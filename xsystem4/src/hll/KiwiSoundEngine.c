/* Copyright (C) 2021 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system4.h"
#include "system4/ain.h"
#include "system4/archive.h"
#include "system4/string.h"

#include "vm.h"
#include "hll.h"
#include "asset_manager.h"
#include "audio.h"
#include "mixer.h"
#include "utfgbk.h"

HLL_QUIET_UNIMPLEMENTED(, void, KiwiSoundEngine, SetGlobalFocus, possibly_unused int nNum);

//int KiwiSoundEngine_Sound_GetGroupNum(int nCh);
//static bool KiwiSoundEngine_Sound_PrepareFromFile(int ch, struct string *filename);

static int KiwiSoundEngine_GetMasterGroup(void)
{
	return 0;
}

static int KiwiSoundEngine_GetBGMGroup(void)
{
	return 0;
}

static int KiwiSoundEngine_GetSEGroup(void)
{
	return 0;
}

static int KiwiSoundEngine_GetVoiceGroup(void)
{
	return 0;
}

static int KiwiSoundEngine_GetGimicSEGroup(void)
{
	return 0;
}

static int KiwiSoundEngine_GetBackVoiceGroup(void)
{
	return 0;
}

static bool rance9_bgm_exists(int num)
{
	char name[16];
	snprintf(name, sizeof(name), "%05d", num);
	return asset_exists_by_name(ASSET_BGM, name, NULL);
}

static int rance9_bgm_prepare(int ch, int num)
{
	char name[16];
	snprintf(name, sizeof(name), "%05d", num);
	struct archive_data *dfile = asset_get_by_name(ASSET_BGM, name, NULL);
	if (!dfile)
		return 0;
	return bgm_prepare_from_archive_data(ch, num, dfile);
}

static bool rance9_wav_exists(int num)
{
	char name[16];
	snprintf(name, sizeof(name), "%05d", num);
	return asset_exists_by_name(ASSET_SOUND, name, NULL);
}

static int rance9_wav_prepare(int ch, int num)
{
	char name[16];
	snprintf(name, sizeof(name), "%05d", num);
	struct archive_data *dfile = asset_get_by_name(ASSET_SOUND, name, NULL);
	if (!dfile)
		return 0;
	return wav_prepare_from_archive_data(ch, num, dfile);
}

static void KiwiSoundEngine_PreLink(void);

// ---------------------------------------------------------------------------
// Name-keyed interface (Rance 03).
//
// Rance 03 declares a different KiwiSoundEngine API: channels are game-managed
// integers, sounds are addressed by file name (no per-game wai/bgi metadata
// exists), and there is no separate BGM archive -- the music lives in
// Rance03Sound.afa alongside the SEs, and voice files in Rance03Voice.afa are
// addressed by bare number ("26743" -> entry "26743.ogg"). File names arrive
// as raw GBK bytes, same as the archive entry names, so plain strcmp matches.
// ---------------------------------------------------------------------------

#define KSE_NR_CHANNELS 256
static int kse_se_volume[KSE_NR_CHANNELS];      // volume preset from SetSeParam, -1 = unset
static char *kse_file_name[KSE_NR_CHANNELS];    // file name per channel, for GetSoundFileName

static void kse_forget_channel(int id)
{
	if (id < 0 || id >= KSE_NR_CHANNELS)
		return;
	kse_se_volume[id] = -1;
	free(kse_file_name[id]);
	kse_file_name[id] = NULL;
}

// Resolve a game file name to an archive entry. Tries the name as-is, then
// with ".ogg" appended; Rance03Sound.afa before Rance03Voice.afa.
static struct archive_data *kse_get_data(struct string *name, enum asset_type *type_out, int *no_out)
{
	static const enum asset_type types[] = { ASSET_SOUND, ASSET_VOICE };
	char buf[512];
	if (name->size >= (int)sizeof(buf) - 4)
		return NULL;
	for (unsigned t = 0; t < sizeof(types)/sizeof(types[0]); t++) {
		for (int sfx = 0; sfx < 2; sfx++) {
			snprintf(buf, sizeof(buf), "%s%s", name->text, sfx ? ".ogg" : "");
			int no;
			struct archive_data *d = asset_get_by_name(types[t], buf, &no);
			if (d) {
				*type_out = types[t];
				*no_out = no;
				return d;
			}
		}
	}
	return NULL;
}

static bool KiwiSoundEngine_IsExistFile(struct string *name)
{
	enum asset_type type;
	int no;
	struct archive_data *d = kse_get_data(name, &type, &no);
	if (!d)
		return false;
	archive_free_data(d);
	return true;
}

static void kse_apply_channel_preset(int id)
{
	if (id < 0 || id >= KSE_NR_CHANNELS)
		return;
	if (kse_se_volume[id] >= 0)
		wav_fade(id, 0, kse_se_volume[id], false);
}

static bool KiwiSoundEngine_Prepare(int id, struct string *filename, bool loop)
{
	if (id < 0)
		return false;
	enum asset_type type;
	int no;
	struct archive_data *d = kse_get_data(filename, &type, &no);
	if (!d) {
		char *u = text2utf(filename->text, filename->size);
		WARNING("KiwiSoundEngine: file not found: %s", u);
		free(u);
		return false;
	}
	// the channel takes ownership of dfile
	if (!wav_prepare_from_archive_data(id, AUDIO_NO_METADATA, d))
		return false;
	if (loop)
		wav_set_loop_count(id, 0);
	kse_apply_channel_preset(id);
	kse_forget_channel(id);
	if (id < KSE_NR_CHANNELS)
		kse_file_name[id] = strdup(filename->text);
	return true;
}

static bool KiwiSoundEngine_Play(int id) { return !!wav_play(id); }
static bool KiwiSoundEngine_Stop(int id) { return !!wav_stop(id); }
static bool KiwiSoundEngine_IsPlay(int id) { return wav_is_playing(id); }
static bool KiwiSoundEngine_Unprepare(int id)
{
	kse_forget_channel(id);
	return !!wav_unprepare(id);
}
static bool KiwiSoundEngine_SetLoopCount(int id, int count) { return !!wav_set_loop_count(id, count); }
static int KiwiSoundEngine_GetLoopCount(int id) { return wav_get_loop_count(id); }
static bool KiwiSoundEngine_Pause(int id) { return !!wav_pause(id); }
static bool KiwiSoundEngine_Restart(int id) { return !!wav_restart(id); }
static bool KiwiSoundEngine_IsPause(int id) { return wav_is_paused(id); }
static bool KiwiSoundEngine_StopFade(int id) { return !!wav_stop_fade(id); }
static bool KiwiSoundEngine_IsFade(int id) { return wav_is_fading(id); }
static bool KiwiSoundEngine_Seek(int id, int pos) { return !!wav_seek(id, pos); }
static int KiwiSoundEngine_GetPos(int id) { return wav_get_pos(id); }
static int KiwiSoundEngine_GetLength(int id) { return wav_get_length(id); }
static int KiwiSoundEngine_GetFreeID(int min_id, possibly_unused int max_id)
{
	int id = wav_get_unused_channel();
	return id < min_id ? min_id : id;
}

// Fade takes the target volume as a float in [0;1]; the mixer scale is 0-100.
static bool KiwiSoundEngine_Fade(int id, int time, float volume, bool stop, possibly_unused int fade_type)
{
	int vol = (int)(volume * 100.0f + 0.5f);
	if (vol < 0) vol = 0;
	if (vol > 100) vol = 100;
	return !!wav_fade(id, time, vol, stop);
}

static bool KiwiSoundEngine_PlaySe(int id, struct string *filename)
{
	return KiwiSoundEngine_Prepare(id, filename, false) && !!wav_play(id);
}
static bool KiwiSoundEngine_StopSe(int id) { return !!wav_stop(id); }
static bool KiwiSoundEngine_IsPlaySe(int id) { return wav_is_playing(id); }

// (channel, volume 0-100, flag). The game presets volumes for its system SE
// channels before preparing them (Ｅ＿システム効果音設定: all (id, 100, 1)).
// Remember the volume and apply it once the channel exists; Fade with time=0
// is how the mixer sets volume on a live channel.
static bool KiwiSoundEngine_SetSeParam(int id, int volume, possibly_unused bool flag)
{
	if (id < 0)
		return false;
	if (id < KSE_NR_CHANNELS)
		kse_se_volume[id] = volume;
	kse_apply_channel_preset(id);
	return true;
}

static int KiwiSoundEngine_GetLengthFromFile(struct string *filename)
{
	enum asset_type type;
	int no;
	struct archive_data *d = kse_get_data(filename, &type, &no);
	if (!d)
		return 0;
	struct channel *ch = channel_open(type, no);
	if (!ch)
		return 0;
	int len = channel_get_time_length(ch);
	channel_close(ch);
	return len;
}

// Rance 03 has no per-file group metadata; a resolvable file is one group.
static int KiwiSoundEngine_GetGroupNumFromFile(struct string *filename)
{
	return KiwiSoundEngine_IsExistFile(filename) ? 1 : 0;
}

static bool KiwiSoundEngine_GetSoundFileName(int id, struct string **out)
{
	if (id < 0 || id >= KSE_NR_CHANNELS || !kse_file_name[id]) {
		*out = string_ref(string_alloc(0));
		return false;
	}
	*out = string_ref(make_string(kse_file_name[id], strlen(kse_file_name[id])));
	return true;
}

// No call sites in Rance 03; unity gain on the 0-1 scale.
static float KiwiSoundEngine_GetGroupVolume(possibly_unused int id)
{
	return 1.0f;
}

static int KiwiSoundEngine_MillisecondsToSamples(int ms, possibly_unused int unused)
{
	return (int)((long long)ms * 44100 / 1000);
}

static void KiwiSoundEngine_PreLink(void);

HLL_LIBRARY(KiwiSoundEngine,
	    HLL_EXPORT(_PreLink, KiwiSoundEngine_PreLink),
	    HLL_EXPORT(SetGlobalFocus, KiwiSoundEngine_SetGlobalFocus),
	    HLL_EXPORT(Music_IsExist, bgm_exists),
	    HLL_EXPORT(Music_Prepare, bgm_prepare),
	    HLL_EXPORT(Music_Unprepare, bgm_unprepare),
	    HLL_EXPORT(Music_Play, bgm_play),
	    HLL_EXPORT(Music_Stop, bgm_stop),
	    HLL_EXPORT(Music_IsPlay, bgm_is_playing),
	    HLL_EXPORT(Music_SetLoopCount, bgm_set_loop_count),
	    HLL_EXPORT(Music_GetLoopCount, bgm_get_loop_count),
	    HLL_EXPORT(Music_SetLoopStartPos, bgm_set_loop_start_pos),
	    HLL_EXPORT(Music_SetLoopEndPos, bgm_set_loop_end_pos),
	    HLL_EXPORT(Music_Fade, bgm_fade),
	    HLL_EXPORT(Music_StopFade, bgm_stop_fade),
	    HLL_EXPORT(Music_IsFade, bgm_is_fading),
	    HLL_EXPORT(Music_Pause, bgm_pause),
	    HLL_EXPORT(Music_Restart, bgm_restart),
	    HLL_EXPORT(Music_IsPause, bgm_is_paused),
	    HLL_EXPORT(Music_GetPos, bgm_get_pos),
	    HLL_EXPORT(Music_GetLength, bgm_get_length),
	    HLL_EXPORT(Music_GetSamplePos, bgm_get_sample_pos),
	    HLL_EXPORT(Music_GetSampleLength, bgm_get_sample_length),
	    HLL_EXPORT(Music_Seek, bgm_seek),
	    //HLL_EXPORT(Music_MillisecondsToSamples, KiwiSoundEngine_Music_MillisecondsToSamples),
	    //HLL_EXPORT(Music_GetFormat, KiwiSoundEngine_Music_GetFormat),
	    HLL_EXPORT(Sound_IsExist, wav_exists),
	    HLL_EXPORT(Sound_Prepare, wav_prepare),
	    HLL_EXPORT(Sound_Unprepare, wav_unprepare),
	    HLL_EXPORT(Sound_Play, wav_play),
	    HLL_EXPORT(Sound_Stop, wav_stop),
	    HLL_EXPORT(Sound_IsPlay, wav_is_playing),
	    HLL_EXPORT(Sound_Fade, wav_fade),
	    HLL_EXPORT(Sound_StopFade, wav_stop_fade),
	    HLL_EXPORT(Sound_IsFade, wav_is_fading),
	    HLL_EXPORT(Sound_GetTimeLength, wav_get_time_length),
	    HLL_TODO_EXPORT(Sound_GetGroupNum, KiwiSoundEngine_Sound_GetGroupNum),
	    HLL_EXPORT(Sound_GetGroupNumFromDataNum, wav_get_group_num_from_data_num),
	    HLL_TODO_EXPORT(Sound_PrepareFromFile, KiwiSoundEngine_Sound_PrepareFromFile),
	    HLL_EXPORT(Sound_GetDataLength, wav_get_data_length),
	    HLL_EXPORT(GetMasterGroup, KiwiSoundEngine_GetMasterGroup),
	    HLL_EXPORT(GetBGMGroup, KiwiSoundEngine_GetBGMGroup),
	    HLL_EXPORT(GetSEGroup, KiwiSoundEngine_GetSEGroup),
	    HLL_EXPORT(GetVoiceGroup, KiwiSoundEngine_GetVoiceGroup),
	    HLL_EXPORT(GetGimicSEGroup, KiwiSoundEngine_GetGimicSEGroup),
	    HLL_EXPORT(GetBackVoiceGroup, KiwiSoundEngine_GetBackVoiceGroup),
	    // Name-keyed interface (Rance 03). The .ain never declares the
	    // legacy names above, so the two sets don't collide.
	    HLL_EXPORT(GetFreeSeID, KiwiSoundEngine_GetFreeID),
	    HLL_EXPORT(IsExistSeID, wav_is_playing),
	    HLL_EXPORT(SetSeParam, KiwiSoundEngine_SetSeParam),
	    HLL_EXPORT(PlaySe, KiwiSoundEngine_PlaySe),
	    HLL_EXPORT(StopSe, KiwiSoundEngine_StopSe),
	    HLL_EXPORT(IsPlaySe, KiwiSoundEngine_IsPlaySe),
	    HLL_EXPORT(GetFreeID, KiwiSoundEngine_GetFreeID),
	    HLL_EXPORT(IsExistID, wav_is_playing),
	    HLL_EXPORT(Prepare, KiwiSoundEngine_Prepare),
	    HLL_EXPORT(Unprepare, KiwiSoundEngine_Unprepare),
	    HLL_EXPORT(Play, KiwiSoundEngine_Play),
	    HLL_EXPORT(Stop, KiwiSoundEngine_Stop),
	    HLL_EXPORT(IsPlay, KiwiSoundEngine_IsPlay),
	    HLL_EXPORT(SetLoopCount, KiwiSoundEngine_SetLoopCount),
	    HLL_EXPORT(GetLoopCount, KiwiSoundEngine_GetLoopCount),
	    HLL_EXPORT(Pause, KiwiSoundEngine_Pause),
	    HLL_EXPORT(Restart, KiwiSoundEngine_Restart),
	    HLL_EXPORT(IsPause, KiwiSoundEngine_IsPause),
	    HLL_EXPORT(Fade, KiwiSoundEngine_Fade),
	    HLL_EXPORT(StopFade, KiwiSoundEngine_StopFade),
	    HLL_EXPORT(IsFade, KiwiSoundEngine_IsFade),
	    HLL_EXPORT(Seek, KiwiSoundEngine_Seek),
	    HLL_EXPORT(GetPos, KiwiSoundEngine_GetPos),
	    HLL_EXPORT(GetLength, KiwiSoundEngine_GetLength),
	    HLL_EXPORT(GetGroupNum, wav_get_group_num),
	    HLL_EXPORT(IsExistFile, KiwiSoundEngine_IsExistFile),
	    HLL_EXPORT(GetLengthFromFile, KiwiSoundEngine_GetLengthFromFile),
	    HLL_EXPORT(GetGroupNumFromFile, KiwiSoundEngine_GetGroupNumFromFile),
	    HLL_EXPORT(GetGroupVolume, KiwiSoundEngine_GetGroupVolume),
	    HLL_EXPORT(MillisecondsToSamples, KiwiSoundEngine_MillisecondsToSamples),
	    HLL_EXPORT(GetSoundFileName, KiwiSoundEngine_GetSoundFileName)
	);

static void KiwiSoundEngine_PreLink(void)
{
	int libno = ain_get_library(ain, "KiwiSoundEngine");
	if (libno < 0)
		return;
	if (ain_get_library_function(ain, libno, "Sound_Seek") >= 0) {
		static_library_replace(&lib_KiwiSoundEngine, "Music_IsExist", rance9_bgm_exists);
		static_library_replace(&lib_KiwiSoundEngine, "Music_Prepare", rance9_bgm_prepare);
		static_library_replace(&lib_KiwiSoundEngine, "Sound_IsExist", rance9_wav_exists);
		static_library_replace(&lib_KiwiSoundEngine, "Sound_Prepare", rance9_wav_prepare);
	}
}
