/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system4/ain.h"
#include "system4/archive.h"
#include "system4/ex.h"
#include "system4/string.h"

#include "asset_manager.h"
#include "vm/heap.h"
#include "vm/page.h"
#include "parts.h"
#include "hll.h"
#include "parts_activity_names.h"
#include "xsystem4.h"

static void PartsEngine_ModuleInit(void)
{
	PE_Init();
}

static void PartsEngine_ModuleFini(void)
{
	PE_Reset();
}

static void PartsEngine_Update(int passed_time, bool is_skip, bool message_window_show)
{
	PE_Update(passed_time, message_window_show);
}

static void PartsEngine_Update_Pascha3PC(struct string *xxx1, struct string *xxx2, int passed_time, bool is_skip, bool message_window_show)
{
	PE_Update(passed_time, message_window_show);
}

// Oyako Rankan
static bool PartsEngine_AddDrawCutCGToPartsConstructionProcess_old(int parts_no,
		struct string *cg_name, int dx, int dy, int sx, int sy, int w, int h,
		int state)
{
	return PE_AddDrawCutCGToPartsConstructionProcess(parts_no, cg_name, dx, dy, w, h,
			sx, sy, w, h, 0, state);
}

// Oyako Rankan
static bool PartsEngine_AddCopyCutCGToPartsConstructionProcess_old(int parts_no,
		struct string *cg_name, int dx, int dy, int sx, int sy, int w, int h,
		int state)
{
	return PE_AddCopyCutCGToPartsConstructionProcess(parts_no, cg_name, dx, dy, w, h,
			sx, sy, w, h, 0, state);
}

// Rance 9: argument types changed from int to float
static void PartsEngine_SetComponentPos(int parts_no, float x, float y)
{
	PE_SetPos(parts_no, x, y);
}

// Rance 9: return type changed from int to float
static float PartsEngine_Parts_GetPartsUpperLeftPosX(int parts_no, int state)
{
	return PE_GetPartsUpperLeftPosX(parts_no, state);
}

static float PartsEngine_Parts_GetPartsUpperLeftPosY(int parts_no, int state)
{
	return PE_GetPartsUpperLeftPosY(parts_no, state);
}

// Rance 9: return type changed from int to float
static float PartsEngine_Parts_GetComponentPosX(int parts_no)
{
	return PE_GetPartsX(parts_no);
}

static float PartsEngine_GetComponentPosY(int parts_no)
{
	return PE_GetPartsY(parts_no);
}

// Rance 9: X/Y coordinate types changed from int to float
static void PartsEngine_AddComponentMotionPos(int parts_no, float begin_x, float begin_y,
		float end_x, float end_y, int begin_t, int end_t,
		struct string *curve_name)
{
	PE_AddMotionPos_curve(parts_no, begin_x, begin_y, end_x, end_y,
			begin_t, end_t, curve_name);
}

static void PartsEngine_add_construction_process(union vm_value *ints,
		union vm_value *floats, union vm_value *strings)
{
	int parts_no    = ints[0].i;
	int state       = ints[1].i;
	int command     = ints[2].i;
	int interp_type = ints[3].i;
	int sx          = ints[4].i;
	int sy          = ints[5].i;
	int sw          = ints[6].i;
	int sh          = ints[7].i;
	int dx          = ints[8].i;
	int dy          = ints[9].i;
	int dw          = ints[12].i;
	int dh          = ints[13].i;
	int r           = ints[14].i;
	int g           = ints[15].i;
	int b           = ints[16].i;
	int a           = ints[17].i;
	// int r2       = ints[18].i;
	// int g2       = ints[19].i;
	// int b2       = ints[20].i;
	int char_space  = ints[21].i;
	int line_space  = ints[22].i;
	int font_type   = ints[23].i;
	int font_size   = ints[24].i;
	int font_r      = ints[25].i;
	int font_g      = ints[26].i;
	int font_b      = ints[27].i;
	int edge_r      = ints[28].i;
	int edge_g      = ints[29].i;
	int edge_b      = ints[30].i;
	int full_size   = ints[31].i;
	float bold_weight = floats[0].f;
	float edge_weight = floats[1].f;
	struct string *text    = heap_get_string(strings[0].i);
	struct string *cg_name = heap_get_string(strings[1].i);

	switch (command) {
	case 0:  // CASConstructionProcess::SetCreate
		PE_AddCreateToPartsConstructionProcess(parts_no, dw, dh, state);
		break;
	case 1:  // CASConstructionProcess::SetCreatePixelOnly
		PE_AddCreatePixelOnlyToPartsConstructionProcess(parts_no, dw, dh, state);
		break;
	case 2:  // CASConstructionProcess::SetCreateCG
		PE_AddCreateCGToProcess(parts_no, cg_name, state);
		break;
	case 3:  // CASConstructionProcess::SetFill
		PE_AddFillToPartsConstructionProcess(parts_no,
				dx, dy, dw, dh, r, g, b, state);
		break;
	case 4:  // CASConstructionProcess::SetFillAlphaColor
		PE_AddFillAlphaColorToPartsConstructionProcess(parts_no,
				dx, dy, dw, dh, r, g, b, a, state);
		break;
	case 5:  // CASConstructionProcess::SetFillAMap
		PE_AddFillAMapToPartsConstructionProcess(parts_no,
				dx, dy, dw, dh, a, state);
		break;
	case 6:  // CASConstructionProcess::SetFillWithAlpha
		PE_AddFillWithAlphaToPartsConstructionProcess(parts_no,
				dx, dy, dw, dh, r, g, b, a, state);
		break;
	case 7:  // CASConstructionProcess::SetDrawText
		PE_AddDrawTextToPartsConstructionProcess(parts_no,
				dx, dy, text, font_type, font_size,
				font_r, font_g, font_b, bold_weight,
				edge_r, edge_g, edge_b, edge_weight,
				char_space, line_space, state);
		break;
	case 8:  // CASConstructionProcess::SetCopyText
		PE_AddCopyTextToPartsConstructionProcess(parts_no,
				dx, dy, text, font_type, font_size,
				font_r, font_g, font_b, bold_weight,
				edge_r, edge_g, edge_b, edge_weight,
				char_space, line_space, state);
		break;
	case 9:  // CASConstructionProcess::SetFillGradationHorizon
		WARNING("AddConstructProcess: FillGradationHorizon unimplemented");
		break;
	case 10:  // CASConstructionProcess::SetDrawRect
		PE_AddDrawRectToPartsConstructionProcess(parts_no,
				dx, dy, dw, dh, r, g, b, state);
		break;
	case 11:  // CASConstructionProcess::SetCutCGBlend (equal scale)
		PE_AddDrawCutCGToPartsConstructionProcess(parts_no, cg_name,
				dx, dy, dw, dh, sx, sy, dw, dh,
				interp_type, state);
		break;
	case 12:  // CASConstructionProcess::SetCutCGCopy (equal scale)
		PE_AddCopyCutCGToPartsConstructionProcess(parts_no, cg_name,
				dx, dy, dw, dh, sx, sy, dw, dh,
				interp_type, state);
		break;
	case 13:  // CASConstructionProcess::SetCutCGScaleBlend
		PE_AddDrawCutCGToPartsConstructionProcess(parts_no, cg_name,
				dx, dy, dw, dh, sx, sy, sw, sh,
				interp_type, state);
		break;
	case 14:  // CASConstructionProcess::SetCutCGScaleCopy
		PE_AddCopyCutCGToPartsConstructionProcess(parts_no, cg_name,
				dx, dy, dw, dh, sx, sy, sw, sh,
				interp_type, state);
		break;
	case 15:  // CASConstructionProcess::SetGrayFilter
		PE_AddGrayFilterToPartsConstructionProcess(parts_no,
				dx, dy, dw, dh, full_size, state);
		break;
	case 16:  // CASConstructionProcess::SetAddFilter
		WARNING("AddConstructProcess: AddFilter unimplemented");
		break;
	case 17:  // CASConstructionProcess::SetMulFilter
		WARNING("AddConstructProcess: MulFilter unimplemented");
		break;
	case 18:  // CASConstructionProcess::SetDrawLine
		WARNING("AddConstructProcess: DrawLine unimplemented");
		break;
	default:
		WARNING("AddConstructProcess: unknown command %d", command);
		break;
	}
}

// Rance9 predates the layer functions at func_id 3-5.  Use the numbering from
// later PartsEngine versions as canonical and adjust Rance9's ids to match.
static bool parts_func_uses_rance9_ids;

// Generic dispatch function for PartsEngine operations.
// func_id selects the operation; arguments and return values are passed
// through three typed arrays (int/bool, float, string).
//
// Calling convention (from game script):
//   1. Push input values into the appropriate arrays.
//   2. Push a placeholder (0 or "") for each output slot.
//   3. Call PartsFunc; outputs are written back into the placeholder slots.
static int PartsEngine_PartsFunc(int func_id, struct page **array_int,
		struct page **array_float, struct page **array_string)
{
	int nr_ints = (array_int && *array_int) ? (*array_int)->nr_vars : 0;
	union vm_value *ints = nr_ints ? (*array_int)->values : NULL;
	int nr_floats = (array_float && *array_float) ? (*array_float)->nr_vars : 0;
	union vm_value *floats = nr_floats ? (*array_float)->values : NULL;
	int nr_strings = (array_string && *array_string) ? (*array_string)->nr_vars : 0;
	union vm_value *strings = nr_strings ? (*array_string)->values : NULL;

#define REQUIRE_INTS(n) \
	if (nr_ints != (n)) VM_ERROR("Invalid arguments for PartsFunc %d: expected %d ints, got %d", func_id, (n), nr_ints)
#define REQUIRE_FLOATS(n) \
	if (nr_floats < (n)) VM_ERROR("Invalid arguments for PartsFunc %d: expected %d floats, got %d", func_id, (n), nr_floats)
#define REQUIRE_STRINGS(n) \
	if (nr_strings < (n)) VM_ERROR("Invalid arguments for PartsFunc %d: expected %d strings, got %d", func_id, (n), nr_strings)

	int canonical_func_id = func_id;
	if (parts_func_uses_rance9_ids && canonical_func_id > 2)
		canonical_func_id += 3;

	switch (canonical_func_id) {
	case 0:  // void SetActiveLayer(int layer)
		REQUIRE_INTS(1);
		PE_set_active_controller(ints[0].i);
		return 1;
	case 1:  // int GetActiveLayer()
		REQUIRE_INTS(1);
		ints[0].i = PE_get_active_controller();
		return 1;
	case 2:  // int GetSystemOverlayLayer()
		REQUIRE_INTS(1);
		ints[0].i = PE_get_system_controller();
		return 1;
	// Layers are identified by their index in the controller stack, so the
	// layer id and index conversions are identity functions.
	case 3:  // int GetLayerIndex(int layer_id)
	case 4:  // int GetLayerID(int layer_index)
		REQUIRE_INTS(2);
		ints[1].i = ints[0].i;
		return 1;
	case 5:  // int NumofLayer()
		REQUIRE_INTS(1);
		ints[0].i = PE_get_nr_controllers();
		return 1;
	case 6:  // void PauseMotion(bool pause)
		REQUIRE_INTS(1);
		PE_PauseMotion(!!ints[0].i);
		return 1;
	case 7:  // void SetWantSave(int parts_no, bool want_save)
		REQUIRE_INTS(2);
		PE_parts_set_want_save(ints[0].i, !!ints[1].i);
		return 1;
	case 9:  // bool SaveThumbnail(string filename, int reduction_factor)
		REQUIRE_INTS(2);
		REQUIRE_STRINGS(1);
		ints[1].i = PE_save_thumbnail(heap_get_string(strings[0].i), ints[0].i);
		return 1;
	case 43:  // float PARTS_GetAbsoluteX(int number)
		REQUIRE_INTS(1);
		REQUIRE_FLOATS(1);
		floats[0].f = PE_parts_get_absolute_x(ints[0].i);
		return 1;
	case 44:  // float PARTS_GetAbsoluteY(int number)
		REQUIRE_INTS(1);
		REQUIRE_FLOATS(1);
		floats[0].f = PE_parts_get_absolute_y(ints[0].i);
		return 1;
	case 45:  // int PARTS_GetAbsoluteZ(int number)
		REQUIRE_INTS(2);
		ints[1].i = PE_parts_get_absolute_z(ints[0].i);
		return 1;
	case 48:  // void PARTS_SetLockInputState(int number, bool lock)
		REQUIRE_INTS(2);
		PE_parts_set_lock_input_state(ints[0].i, !!ints[1].i);
		return 1;
	case 60:  // void AppendChild(int number, int child_number)
		REQUIRE_INTS(2);
		PE_SetParentPartsNumber(ints[1].i, ints[0].i);
		return 1;
	case 94:  // void SetLayoutBoxPadding(int parts_no, int top, int bottom, int left, int right)
		REQUIRE_INTS(5);
		PE_set_layoutbox_padding(ints[0].i, ints[1].i, ints[2].i, ints[3].i, ints[4].i);
		return 1;
	case 95:  // int GetLayoutBoxPaddingTop(int parts_no)
		REQUIRE_INTS(2);
		ints[1].i = PE_get_layoutbox_padding_top(ints[0].i);
		return 1;
	case 96:  // int GetLayoutBoxPaddingBottom(int parts_no)
		REQUIRE_INTS(2);
		ints[1].i = PE_get_layoutbox_padding_bottom(ints[0].i);
		return 1;
	case 97:  // int GetLayoutBoxPaddingLeft(int parts_no)
		REQUIRE_INTS(2);
		ints[1].i = PE_get_layoutbox_padding_left(ints[0].i);
		return 1;
	case 98:  // int GetLayoutBoxPaddingRight(int parts_no)
		REQUIRE_INTS(2);
		ints[1].i = PE_get_layoutbox_padding_right(ints[0].i);
		return 1;
	case 106:  // void GetPartsCGSurfaceArea(int parts_no, int *x, int *y, int *w, int *h, int state)
		REQUIRE_INTS(6);
		PE_GetPartsCGSurfaceArea(ints[0].i, &ints[1].i, &ints[2].i, &ints[3].i, &ints[4].i, ints[5].i);
		return 1;
	case 112: {  // string GetText(int parts_no, int state)
		REQUIRE_INTS(2);
		REQUIRE_STRINGS(1);
		struct string *text = PE_GetText(ints[0].i, ints[1].i);
		if (text) {
			heap_string_assign(strings[0].i, text);
			free_string(text);
		}
		return 1;
	}
	case 162:  // AddConstructProcess(ArrayInt[32], ArrayFloat[2], ArrayString[2])
		REQUIRE_INTS(32);
		REQUIRE_FLOATS(2);
		REQUIRE_STRINGS(2);
		PartsEngine_add_construction_process(ints, floats, strings);
		return 1;
	case 165:  // bool InitPartsMovie(int parts_no, int width, int height, int bg_r, int bg_g, int bg_b, int state)
		REQUIRE_INTS(8);
		ints[7].i = PE_init_parts_movie(ints[0].i, ints[1].i, ints[2].i, ints[3].i, ints[4].i, ints[5].i, ints[6].i);
		return 1;
	case 166:  // int GetMovieSprite(int parts_no, int state)
		REQUIRE_INTS(3);
		ints[2].i = PE_get_movie_sprite(ints[0].i, ints[1].i);
		return 1;
	default:
		WARNING("Unknown func_id: %d", func_id);
		return 0;
	}
#undef REQUIRE_INTS
}

HLL_WARN_UNIMPLEMENTED(, void, PartsEngine, StopSoundWithoutSystemSound);

/*
 * 画面保管 ("back scene"): the game marks the parts whose state it wants kept
 * across a scene switch, then asks for just those to be serialised. Without
 * SaveBackScene it treats the missing answer as a failure and raises
 * "画面保管においてパーツライブラリの保存に失敗しました" on every message.
 */
static void PartsEngine_SetWantSaveBackScene(int parts_no, bool want)
{
	PE_parts_set_want_back_scene(parts_no, want);
}

static bool PartsEngine_IsWantSaveBackScene(int parts_no)
{
	return PE_parts_want_back_scene(parts_no);
}

/*
 * Check-box state. Rance 03's config screen polls IsCheckBoxChecked every
 * frame from CConfigView@SetCheckStatForParts while the game writes the new
 * state itself on click (setConfigCheckbox -> Ｐ＿チェックボックス＿チェック状態設定).
 * With both answers stubbed to zero the poll never sees a change, so the
 * bottom buttons of the config page lit up as pressed and did nothing.
 */
static void PartsEngine_CheckBoxChecked(int parts_no, bool checked)
{
	PE_parts_set_checked(parts_no, checked);
}

static bool PartsEngine_IsCheckBoxChecked(int parts_no)
{
	return PE_parts_checked(parts_no);
}

static void PartsEngine_SetCheckBoxColor(int parts_no, int r, int g, int b)
{
	PE_parts_set_checkbox_color(parts_no, r, g, b);
}

static int PartsEngine_GetCheckBoxR(int parts_no)
{
	return PE_parts_checkbox_color(parts_no, 0);
}

static int PartsEngine_GetCheckBoxG(int parts_no)
{
	return PE_parts_checkbox_color(parts_no, 1);
}

static int PartsEngine_GetCheckBoxB(int parts_no)
{
	return PE_parts_checkbox_color(parts_no, 2);
}

static void PartsEngine_ClearBackScene(void)
{
	PE_clear_back_scene_flags();
}

static void PartsEngine_PreLink(void);

/*
 * Controller index/ID conversions (Rance 03's multi-controller interface).
 * Layer ids ARE stack indices in this implementation (see the PartsFunc
 * GetLayerIndex/GetLayerID cases), so both conversions are the identity.
 */
static int PE_GetControllerIndex(int controller_id)
{
	return controller_id;
}

static int PE_GetControllerID(int controller_index)
{
	return controller_index;
}

/*
 * Rance 03 activity system ("CASPartsActivity").
 *
 * Activities are name-keyed registries of parts.  The definitions live in
 * .pactex files (an .ex container with an アクティビティ tree and an optional
 * 終了キー list) inside Rance03Pact.afa; ReadActivityFile parses one and
 * creates the real parts, the rest of the family is bookkeeping over the
 * registry.  The game drives everything else (layers, click wiring, the
 * event pump) from script.
 *
 * Interface note: like MainEXFile (see 08a2084) and KiwiSoundEngine
 * (915c328), this is the name-keyed generation of the API.  Upstream only
 * ever saw the older binary generation (ReadActivityBinary), which is why
 * these were stubs.
 */
#define PE_ACTIVITY_MAX 16
// keep in sync with ACT_NR_STATES in parts/parts_internal.h (private header)
#define ACT_NR_STATES 3

struct activity_part {
	struct string *name;
	int parts_no;
};

struct activity_intent {
	struct string *parts_name;  // string_ref'd
	int type;
	struct string **dests;      // string_ref'd
	int nr_dests;
};

struct pe_activity {
	struct string *name;        // string_ref'd
	struct activity_part *parts;
	int nr_parts;
	struct string **close_parts;
	int nr_close_parts;
	struct activity_intent *intents;
	int nr_intents;
	int *end_keys;
	int nr_end_keys;
};

static struct pe_activity activities[PE_ACTIVITY_MAX];

static struct pe_activity *activity_find(const char *name)
{
	for (int i = 0; i < PE_ACTIVITY_MAX; i++) {
		if (activities[i].name && !strcmp(activities[i].name->text, name))
			return &activities[i];
	}
	return NULL;
}

static struct pe_activity *activity_create(struct string *name)
{
	if (activity_find(name->text))
		return NULL;
	for (int i = 0; i < PE_ACTIVITY_MAX; i++) {
		struct pe_activity *a = &activities[i];
		if (a->name)
			continue;
		memset(a, 0, sizeof(*a));
		a->name = string_ref(name);
		return a;
	}
	WARNING("activity table full");
	return NULL;
}

static void activity_intent_free(struct activity_intent *it)
{
	free_string(it->parts_name);
	for (int i = 0; i < it->nr_dests; i++)
		free_string(it->dests[i]);
	free(it->dests);
}

static void activity_free(struct pe_activity *a)
{
	for (int i = 0; i < a->nr_parts; i++)
		free_string(a->parts[i].name);
	free(a->parts);
	for (int i = 0; i < a->nr_close_parts; i++)
		free_string(a->close_parts[i]);
	free(a->close_parts);
	for (int i = 0; i < a->nr_intents; i++)
		activity_intent_free(&a->intents[i]);
	free(a->intents);
	free(a->end_keys);
	free_string(a->name);
	memset(a, 0, sizeof(*a));
}

static struct activity_intent *activity_intent_get(struct pe_activity *a, struct string *parts_name)
{
	for (int i = 0; i < a->nr_intents; i++) {
		if (!strcmp(a->intents[i].parts_name->text, parts_name->text))
			return &a->intents[i];
	}
	a->intents = realloc(a->intents, (a->nr_intents+1) * sizeof(struct activity_intent));
	struct activity_intent *it = &a->intents[a->nr_intents++];
	it->parts_name = string_ref(parts_name);
	it->type = 0;
	it->dests = NULL;
	it->nr_dests = 0;
	return it;
}

static struct activity_part *activity_part_by_name(struct pe_activity *a, const char *parts_name)
{
	for (int i = 0; i < a->nr_parts; i++) {
		if (!strcmp(a->parts[i].name->text, parts_name))
			return &a->parts[i];
	}
	return NULL;
}

static struct activity_part *activity_part_by_number(struct pe_activity *a, int parts_no)
{
	for (int i = 0; i < a->nr_parts; i++) {
		if (a->parts[i].parts_no == parts_no)
			return &a->parts[i];
	}
	return NULL;
}

static void activity_add_part(struct pe_activity *a, struct string *name, int parts_no)
{
	a->parts = realloc(a->parts, (a->nr_parts+1) * sizeof(struct activity_part));
	a->parts[a->nr_parts].name = string_ref(name);
	a->parts[a->nr_parts].parts_no = parts_no;
	a->nr_parts++;
}

static void activity_add_end_key(struct pe_activity *a, int key)
{
	for (int i = 0; i < a->nr_end_keys; i++) {
		if (a->end_keys[i] == key)
			return;
	}
	a->end_keys = realloc(a->end_keys, (a->nr_end_keys+1) * sizeof(int));
	a->end_keys[a->nr_end_keys++] = key;
}

// --- .pactex loader ---

static struct ex_tree *act_tree_child(struct ex_tree *t, const char *name)
{
	if (!t || t->is_leaf)
		return NULL;
	for (uint32_t i = 0; i < t->nr_children; i++) {
		if (!strcmp(t->children[i].name->text, name))
			return &t->children[i];
	}
	return NULL;
}

static struct ex_value *act_leaf(struct ex_tree *t, const char *name)
{
	struct ex_tree *c = act_tree_child(t, name);
	return c && c->is_leaf ? &c->leaf.value : NULL;
}

static int act_int(struct ex_tree *t, const char *name, int dflt)
{
	struct ex_value *v = act_leaf(t, name);
	return v && v->type == EX_INT ? v->i : dflt;
}

static const char *act_str(struct ex_tree *t, const char *name)
{
	struct ex_value *v = act_leaf(t, name);
	return v && v->type == EX_STRING ? v->s->text : "";
}

static float act_list_num(struct ex_value *v, int i, float dflt)
{
	if (!v || v->type != EX_LIST || i >= (int)v->list->nr_items)
		return dflt;
	struct ex_value *item = &v->list->items[i].value;
	if (item->type == EX_INT)
		return item->i;
	if (item->type == EX_FLOAT)
		return item->f;
	return dflt;
}

// The tab/button CGs ship as one archive entry per state, suffixed
// ／通常, ／オン, ／ダウン (and ／無効 for the disabled variant).  The
// .pactex only names the base, so resolve the suffix per state and fall
// back to the base name when a variant is missing.
static void activity_set_button_cg(int parts_no, struct string *base, bool enabled)
{
	static const char *suffix[ACT_NR_STATES] = { ACT_N_SFX_0, ACT_N_SFX_1, ACT_N_SFX_2 };
	for (int st = 0; st < ACT_NR_STATES; st++) {
		struct string *name = string_dup(base);
		string_append_cstr(&name, suffix[st], strlen(suffix[st]));
		if (enabled && asset_exists_by_name(ASSET_CG, name->text, NULL)) {
			PE_SetPartsCG(parts_no, name, 0, st+1);
			free_string(name);
			continue;
		}
		free_string(name);
		if (!enabled && st == 0) {
			// disabled buttons use the ／無効 variant for every state
			struct string *dis = string_dup(base);
			string_append_cstr(&dis, ACT_N_SFX_OFF, strlen(ACT_N_SFX_OFF));
			if (asset_exists_by_name(ASSET_CG, dis->text, NULL)) {
				for (int st2 = 0; st2 < ACT_NR_STATES; st2++)
					PE_SetPartsCG(parts_no, dis, 0, st2+1);
				free_string(dis);
				return;
			}
			free_string(dis);
		}
		PE_SetPartsCG(parts_no, base, 0, st+1);
	}
}

// Track which unhandled .pactex fields have been complained about already.
static void act_warn_once(const char *field)
{
	static char warned[512];
	if (!field)
		return;
	for (const char *w = warned; *w; w += strlen(w)+1) {
		if (!strcmp(w, field))
			return;
	}
	size_t off = strlen(warned);
	if (off + strlen(field) + 1 < sizeof(warned)) {
		strcpy(warned + off, field);
		warned[off + strlen(field)] = '\0';
	}
	WARNING("activity file: unhandled field %s", field);
}

static void act_warn_once_type(int type)
{
	char label[32];
	snprintf(label, sizeof(label), "<part type %d>", type);
	act_warn_once(label);
}

static void activity_apply_state(int parts_no, struct ex_tree *state, int pe_state)
{
	struct ex_value *cg = act_leaf(state, ACT_N_CG_NAME);
	if (cg && cg->type == EX_STRING && *(cg->s->text)) {
		PE_SetPartsCG(parts_no, cg->s, act_int(state, ACT_N_DEFORM, 0), pe_state);
		struct ex_value *sa = act_leaf(state, ACT_N_SURFACE);
		if (sa && sa->type == EX_LIST && (act_list_num(sa, 0, 0) || act_list_num(sa, 1, 0)
					|| act_list_num(sa, 2, 0) || act_list_num(sa, 3, 0)))
			PE_SetPartsCGSurfaceArea(parts_no, act_list_num(sa,0,0), act_list_num(sa,1,0),
					act_list_num(sa,2,0), act_list_num(sa,3,0), pe_state);
	}
	struct ex_value *text = act_leaf(state, ACT_N_TEXT);
	if (text && text->type == EX_STRING && *(text->s->text)) {
		PE_SetText(parts_no, text->s, pe_state);
		int font_type = act_int(state, ACT_N_FONT_TYPE, 0);
		if (font_type)
			PE_SetPartsFontType(parts_no, font_type, pe_state);
		int font_size = act_int(state, ACT_N_FONT_SIZE, 0);
		if (font_size)
			PE_SetPartsFontSize(parts_no, font_size, pe_state);
		struct ex_value *fc = act_leaf(state, ACT_N_FONT_COLOR);
		if (fc)
			PE_SetPartsFontColor(parts_no, act_list_num(fc,0,0), act_list_num(fc,1,0),
					act_list_num(fc,2,0), pe_state);
		struct ex_value *ec = act_leaf(state, ACT_N_FONT_EDGE_COLOR);
		if (ec)
			PE_SetPartsFontEdgeColor(parts_no, act_list_num(ec,0,0), act_list_num(ec,1,0),
					act_list_num(ec,2,0), pe_state);
		int char_space = act_int(state, ACT_N_CHAR_SPACE, act_int(state, ACT_N_CHAR_SPACE2, 0));
		if (char_space)
			PE_SetTextCharSpace(parts_no, char_space, pe_state);
		int line_space = act_int(state, ACT_N_LINE_SPACE, 0);
		if (line_space)
			PE_SetTextLineSpace(parts_no, line_space, pe_state);
	}
}

static int activity_walk_part(struct pe_activity *act, struct ex_tree *part, int parent_no);

static void activity_walk_children(struct pe_activity *act, struct ex_tree *part, int parts_no)
{
	struct ex_tree *children = act_tree_child(part, ACT_N_CHILDREN);
	if (!children || children->is_leaf)
		return;
	for (uint32_t i = 0; i < children->nr_children; i++) {
		struct ex_tree *child = &children->children[i];
		if (child->is_leaf)
			continue;
		int child_no = activity_walk_part(act, child, parts_no);
		if (child_no >= 0)
			PE_SetParentPartsNumber(child_no, parts_no);
	}
}

static int activity_walk_part(struct pe_activity *act, struct ex_tree *part, int parent_no)
{
	int no = PE_GetFreeNumber();
	activity_add_part(act, part->name, no);

	// common properties
	struct ex_value *pos = act_leaf(part, ACT_N_POS);
	if (pos && pos->type == EX_LIST) {
		PE_SetPos(no, act_list_num(pos, 0, 0), act_list_num(pos, 1, 0));
		PE_SetZ(no, act_list_num(pos, 2, 1));
	}
	int origin_mode = act_int(part, ACT_N_ORIGIN, 0);
	if (origin_mode)
		PE_SetPartsOriginPosMode(no, origin_mode);
	if (act_int(part, ACT_N_SHOW, 1))
		PE_SetShow(no, true);
	PE_SetPartsMessageWindowShowLink(no, act_int(part, ACT_N_MSG_LINK, 0));
	PE_SetAlpha(no, act_int(part, ACT_N_ALPHA, 255));
	struct ex_value *add = act_leaf(part, ACT_N_ADD_COLOR);
	if (add)
		PE_SetAddColor(no, act_list_num(add,0,0), act_list_num(add,1,0), act_list_num(add,2,0));
	struct ex_value *mul = act_leaf(part, ACT_N_MUL_COLOR);
	if (mul)
		PE_SetMultiplyColor(no, act_list_num(mul,0,255), act_list_num(mul,1,255), act_list_num(mul,2,255));
	PE_SetPartsDrawFilter(no, act_int(part, ACT_N_DRAW_FILTER, 0));
	struct ex_value *mag = act_leaf(part, ACT_N_MAG);
	if (mag) {
		PE_SetPartsMagX(no, act_list_num(mag, 0, 1));
		PE_SetPartsMagY(no, act_list_num(mag, 1, 1));
	}
	struct ex_value *rot = act_leaf(part, ACT_N_ROT);
	if (rot) {
		PE_SetPartsRotateX(no, act_list_num(rot, 0, 0));
		PE_SetPartsRotateY(no, act_list_num(rot, 1, 0));
		PE_SetPartsRotateZ(no, act_list_num(rot, 2, 0));
	}
	struct ex_value *margin = act_leaf(part, ACT_N_MARGIN);
	if (margin)
		PE_SetComponentMargin(no, act_list_num(margin,0,0), act_list_num(margin,1,0),
				act_list_num(margin,2,0), act_list_num(margin,3,0));
	struct ex_value *clip = act_leaf(part, ACT_N_ALPHA_CLIP);
	if (clip && clip->type == EX_STRING && *(clip->s->text)) {
		struct activity_part *target = activity_part_by_name(act, clip->s->text);
		if (target)
			PE_SetPartsAlphaClipperPartsNumber(no, target->parts_no);
	}
	struct ex_value *link = act_leaf(part, ACT_N_ONCURSOR_LINK);
	if (link && link->type == EX_STRING && *(link->s->text)) {
		struct activity_part *target = activity_part_by_name(act, link->s->text);
		if (target)
			PE_SetOnCursorShowLinkPartsNumber(no, target->parts_no);
	}
	if (*act_str(part, ACT_N_SCROLL_X))
		act_warn_once(ACT_N_SCROLL_X);
	if (*act_str(part, ACT_N_SCROLL_Y))
		act_warn_once(ACT_N_SCROLL_Y);
	if (*act_str(part, ACT_N_SCROLL_A))
		act_warn_once(ACT_N_SCROLL_A);
	if (*act_str(part, ACT_N_CHECKBOX_LINK))
		act_warn_once(ACT_N_CHECKBOX_LINK);
	int speedup = act_int(part, ACT_N_SPEEDUP, 0);
	if (speedup)
		PE_SetSpeedupRateByMessageSkip(no, speedup);
	/*
	 * クリック許可 is not parts->clickable. Every part in the shipped .pactex
	 * files carries 0 there, and the game only ever touches clickability from
	 * the scene editor and context menus (Ｐ＿クリック可能設定). Mapping the
	 * field literally left the whole config screen unable to raise
	 * PARTS_MSG_MOUSE_CLICK -- the hit test still captures the press because
	 * pass_cursor is 0, so the buttons lit up as pressed and never fired.
	 * What actually decides it is the widget type.
	 */
	struct ex_tree *info = act_tree_child(part, ACT_N_TYPE_INFO);
	int widget_type = (info && !info->is_leaf) ? act_int(info, ACT_N_PART_TYPE, 0) : -1;
	PE_SetClickable(no, widget_type == 0 || widget_type == 1 || widget_type == 3);
	PE_SetPassCursor(no, act_int(part, ACT_N_PASS_CURSOR, 0));
	PE_SetDrag(no, act_int(part, ACT_N_DRAG, 0));
	PE_SetPartsPixelDecide(no, act_int(part, ACT_N_PIXEL_DECIDE, 0));
	if (act_int(part, ACT_N_RATE_POS, 0))
		act_warn_once(ACT_N_RATE_POS);
	if (*act_str(part, ACT_N_SE_ON))
		act_warn_once(ACT_N_SE_ON);
	if (*act_str(part, ACT_N_SE_CLICK))
		act_warn_once(ACT_N_SE_CLICK);
	if (act_int(part, ACT_N_CLOSED, 0))
		PE_SetShow(no, false);

	// click-through intent data (遷移先/遷移方法 at part level)
	struct ex_value *trans = act_leaf(part, ACT_N_TRANS_DST);
	if (trans && trans->type == EX_LIST) {
		struct activity_intent *it = activity_intent_get(act, part->name);
		it->type = act_int(part, ACT_N_TRANS_TYPE, it->type);
		for (uint32_t i = 0; i < trans->list->nr_items; i++) {
			struct ex_value *d = &trans->list->items[i].value;
			if (d->type != EX_STRING || !*(d->s->text))
				continue;
			it->dests = realloc(it->dests, (it->nr_dests+1) * sizeof(struct string*));
			it->dests[it->nr_dests++] = string_ref(d->s);
		}
	}

	// type-specific information
	if (info && !info->is_leaf) {
		int type = act_int(info, ACT_N_PART_TYPE, 0);
		switch (type) {
		case 8: // layout box
			PE_SetLayoutBoxLayoutType(no, act_int(info, ACT_N_LAYOUT_TYPE, 0));
			PE_SetLayoutBoxAlign(no, act_int(info, ACT_N_ALIGN, 0));
			if (act_int(info, ACT_N_WRAP, 0))
				PE_SetLayoutBoxReturn(no, true, act_int(info, ACT_N_WRAP_SIZE, 200));
			{
				struct ex_value *pad = act_leaf(info, ACT_N_PADDING);
				if (pad)
					PE_set_layoutbox_padding(no, act_list_num(pad,0,0), act_list_num(pad,1,0),
							act_list_num(pad,2,0), act_list_num(pad,3,0));
			}
			break;
		case 0:
			// plain container or button: a ＣＧ名 here means a button-like
			// part whose per-state graphics use the ／通常／オン／ダウン
			// archive naming convention
			{
				struct ex_value *bcg = act_leaf(info, ACT_N_CG_NAME);
				if (bcg && bcg->type == EX_STRING && *(bcg->s->text)) {
					bool enabled = !act_int(info, ACT_N_ENABLED, 1);
					activity_set_button_cg(no, bcg->s, enabled);
					struct ex_value *size = act_leaf(info, ACT_N_SIZE);
					if (size)
						PE_SetPartsRectangleDetectionSize(no, act_list_num(size,0,0),
								act_list_num(size,1,0), 1);
					struct ex_value *text = act_leaf(info, ACT_N_TEXT);
					if (text && text->type == EX_STRING && *(text->s->text))
						PE_SetText(no, text->s, 1);
				}
			}
			break;
		case 10:
			// clip container: per-state CGs follow
			break;
		default:
			// checkbox/button widget types the game drives from its own
			// script classes; nothing to do beyond the common properties
			act_warn_once_type(type);
			break;
		}
		// explicit per-state definitions (usually under type 10/11 parts)
		static const char *state_names[ACT_NR_STATES] = { ACT_N_STATE_0, ACT_N_STATE_1, ACT_N_STATE_2 };
		for (int st = 0; st < ACT_NR_STATES; st++) {
			struct ex_tree *s = act_tree_child(info, state_names[st]);
			if (s && !s->is_leaf)
				activity_apply_state(no, s, st+1);
		}
	}

	activity_walk_children(act, part, no);
	return no;
}

static bool activity_load_pact(struct pe_activity *act, struct archive_data *data)
{
	struct ex *ex = ex_read(data->data, data->size);
	if (!ex) {
		WARNING("activity file: ex_read failed for %s", data->name);
		return false;
	}
	struct ex_tree *root = ex_get_tree(ex, ACT_N_ROOT);
	if (!root || root->is_leaf) {
		WARNING("activity file: no activity tree in %s", data->name);
		ex_free(ex);
		return false;
	}
	struct ex_tree *root_parts = act_tree_child(root, ACT_N_ROOT_PARTS);
	if (!root_parts || root_parts->is_leaf) {
		WARNING("activity file: no root parts in %s", data->name);
		ex_free(ex);
		return false;
	}
	int root_no = activity_walk_part(act, root_parts, -1);
	(void)root_no;
	struct ex_list *keys = ex_get_list(ex, ACT_N_END_KEYS);
	if (keys) {
		for (uint32_t i = 0; i < keys->nr_items; i++) {
			if (keys->items[i].value.type == EX_INT)
				activity_add_end_key(act, keys->items[i].value.i);
		}
	}
	ex_free(ex);
	return true;
}

// Activity files are looked up in the Pact archive by basename, so the game's
// ".pact" queries match the archive's ".pactex" entries without any rewriting.
static bool PartsEngine_IsExistActivityFile(struct string *filename)
{
	return asset_exists_by_name(ASSET_PACT, filename->text, NULL);
}

static bool PartsEngine_ReadActivityFile(struct string *name, struct string *filename)
{
	if (activity_find(name->text)) {
		WARNING("activity %s already loaded", display_utf0(name->text));
		return false;
	}
	struct pe_activity *act = activity_create(name);
	if (!act)
		return false;
	int id;
	struct archive_data *data = asset_get_by_name(ASSET_PACT, filename->text, &id);
	if (!data) {
		WARNING("activity file not found: %s", display_utf0(filename->text));
		activity_free(act);
		return false;
	}
	bool ok = activity_load_pact(act, data);
	archive_free_data(data);
	if (!ok) {
		activity_free(act);
		return false;
	}
	return true;
}

static bool PartsEngine_IsExistActivity(struct string *name)
{
	return activity_find(name->text) != NULL;
}

static bool PartsEngine_CreateActivity(struct string *name)
{
	return activity_create(name) != NULL;
}

static bool PartsEngine_ReleaseActivity(struct string *name, struct page ***erase_list)
{
	// The erase list is delegate indexes the game resolves itself after we
	// report success (SYS_Activity_Release then calls method 1450 with it).
	(void)erase_list;
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return false;
	for (int i = 0; i < act->nr_parts; i++) {
		if (PE_IsExist(act->parts[i].parts_no))
			PE_ReleaseParts(act->parts[i].parts_no);
	}
	activity_free(act);
	return true;
}

// Editor-only paths (CActivityEditor/scene edit); the runtime never re-reads
// activities written this way because the shipped .pact archives always win.
static bool PartsEngine_WriteActivityFile(struct string *name, struct string *filename)
{
	WARNING("WriteActivityFile(%s) not implemented (editor path)", display_utf0(filename->text));
	return activity_find(name->text) != NULL;
}

static bool PartsEngine_SaveActivityEXText(struct string **text, struct string *name)
{
	(void)text;
	(void)name;
	WARNING("SaveActivityEXText not implemented (editor path)");
	return false;
}

static bool PartsEngine_LoadActivityEXText(struct string *name, struct string *text)
{
	(void)text;
	WARNING("LoadActivityEXText(%s) not implemented (editor path)", display_utf0(name->text));
	return false;
}

static bool PartsEngine_AddActivityParts(struct string *name, struct string *parts_name, int number)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act || activity_part_by_name(act, parts_name->text))
		return false;
	activity_add_part(act, parts_name, number);
	return true;
}

static bool PartsEngine_RemoveActivityParts(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return false;
	for (int i = 0; i < act->nr_parts; i++) {
		if (strcmp(act->parts[i].name->text, parts_name->text))
			continue;
		free_string(act->parts[i].name);
		memmove(&act->parts[i], &act->parts[i+1], (act->nr_parts-i-1) * sizeof(struct activity_part));
		act->nr_parts--;
		return true;
	}
	return false;
}

static bool PartsEngine_RemoveAllActivityParts(struct string *name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return false;
	for (int i = 0; i < act->nr_parts; i++)
		free_string(act->parts[i].name);
	act->nr_parts = 0;
	return true;
}

static int PartsEngine_NumofActivityParts(struct string *name)
{
	struct pe_activity *act = activity_find(name->text);
	return act ? act->nr_parts : 0;
}

static bool PartsEngine_GetActivityParts(int index, struct string *name,
		struct string **parts_name, int *number)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act || index < 0 || index >= act->nr_parts)
		return false;
	if (*parts_name)
		free_string(*parts_name);
	*parts_name = string_dup(act->parts[index].name);
	*number = act->parts[index].parts_no;
	return true;
}

static bool PartsEngine_IsExistActivityPartsByName(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	return act && activity_part_by_name(act, parts_name->text) != NULL;
}

static bool PartsEngine_IsExistActivityPartsByNumber(struct string *name, int number)
{
	struct pe_activity *act = activity_find(name->text);
	return act && activity_part_by_number(act, number) != NULL;
}

static int PartsEngine_GetActivityPartsNumber(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	struct activity_part *part = act ? activity_part_by_name(act, parts_name->text) : NULL;
	return part ? part->parts_no : -1;
}

static void PartsEngine_GetActivityPartsName(struct string **out, struct string *name, int number)
{
	struct pe_activity *act = activity_find(name->text);
	struct activity_part *part = act ? activity_part_by_number(act, number) : NULL;
	if (!part)
		return;
	if (*out)
		free_string(*out);
	*out = string_dup(part->name);
}

static void PartsEngine_AddActivityCloseParts(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return;
	for (int i = 0; i < act->nr_close_parts; i++) {
		if (!strcmp(act->close_parts[i]->text, parts_name->text))
			return;
	}
	act->close_parts = realloc(act->close_parts, (act->nr_close_parts+1) * sizeof(struct string*));
	act->close_parts[act->nr_close_parts++] = string_ref(parts_name);
}

static void PartsEngine_RemoveActivityCloseParts(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return;
	for (int i = 0; i < act->nr_close_parts; i++) {
		if (strcmp(act->close_parts[i]->text, parts_name->text))
			continue;
		free_string(act->close_parts[i]);
		memmove(&act->close_parts[i], &act->close_parts[i+1], (act->nr_close_parts-i-1) * sizeof(struct string*));
		act->nr_close_parts--;
		return;
	}
}

static void PartsEngine_RemoveAllActivityCloseParts(struct string *name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return;
	for (int i = 0; i < act->nr_close_parts; i++)
		free_string(act->close_parts[i]);
	act->nr_close_parts = 0;
}

static bool PartsEngine_IsExistActivityCloseParts(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return false;
	for (int i = 0; i < act->nr_close_parts; i++) {
		if (!strcmp(act->close_parts[i]->text, parts_name->text))
			return true;
	}
	return false;
}

static void PartsEngine_SetActivityIntentData(struct string *name, struct string *parts_name,
		struct string *destination, int intent_type)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return;
	struct activity_intent *it = activity_intent_get(act, parts_name);
	it->type = intent_type;
	// SetIntentData resets the destination list, AddIntentDataDestination appends
	for (int i = 0; i < it->nr_dests; i++)
		free_string(it->dests[i]);
	it->nr_dests = 0;
	it->dests = realloc(it->dests, sizeof(struct string*));
	it->dests[it->nr_dests++] = string_ref(destination);
}

static void PartsEngine_AddActivityIntentDataDestination(struct string *name,
		struct string *parts_name, struct string *destination)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return;
	struct activity_intent *it = activity_intent_get(act, parts_name);
	it->dests = realloc(it->dests, (it->nr_dests+1) * sizeof(struct string*));
	it->dests[it->nr_dests++] = string_ref(destination);
}

static bool PartsEngine_IsExistActivityIntentData(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return false;
	for (int i = 0; i < act->nr_intents; i++) {
		if (!strcmp(act->intents[i].parts_name->text, parts_name->text))
			return true;
	}
	return false;
}

static int PartsEngine_NumofActivityIntentDataDestination(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return 0;
	for (int i = 0; i < act->nr_intents; i++) {
		if (!strcmp(act->intents[i].parts_name->text, parts_name->text))
			return act->intents[i].nr_dests;
	}
	return 0;
}

static void PartsEngine_GetActivityIntentDataDestination(struct string **out, int index,
		struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return;
	for (int i = 0; i < act->nr_intents; i++) {
		struct activity_intent *it = &act->intents[i];
		if (strcmp(it->parts_name->text, parts_name->text))
			continue;
		if (index < 0 || index >= it->nr_dests)
			return;
		if (*out)
			free_string(*out);
		*out = string_dup(it->dests[index]);
		return;
	}
}

static int PartsEngine_GetActivityIntentDataType(struct string *name, struct string *parts_name)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return 0;
	for (int i = 0; i < act->nr_intents; i++) {
		if (!strcmp(act->intents[i].parts_name->text, parts_name->text))
			return act->intents[i].type;
	}
	return 0;
}

static void PartsEngine_SetActivityEndKey(struct string *name, int key)
{
	struct pe_activity *act = activity_find(name->text);
	if (act)
		activity_add_end_key(act, key);
}

static void PartsEngine_EraseActivityEndKey(struct string *name, int key)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return;
	for (int i = 0; i < act->nr_end_keys; i++) {
		if (act->end_keys[i] != key)
			continue;
		memmove(&act->end_keys[i], &act->end_keys[i+1], (act->nr_end_keys-i-1) * sizeof(int));
		act->nr_end_keys--;
		return;
	}
}

static bool PartsEngine_IsExistActivityEndKey(struct string *name, int key)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act)
		return false;
	for (int i = 0; i < act->nr_end_keys; i++) {
		if (act->end_keys[i] == key)
			return true;
	}
	return false;
}

static int PartsEngine_NumofActivityEndKey(struct string *name)
{
	struct pe_activity *act = activity_find(name->text);
	return act ? act->nr_end_keys : 0;
}

static int PartsEngine_GetActivityEndKey(struct string *name, int index)
{
	struct pe_activity *act = activity_find(name->text);
	if (!act || index < 0 || index >= act->nr_end_keys)
		return 0;
	return act->end_keys[index];
}

HLL_LIBRARY(PartsEngine,
	    HLL_EXPORT(_PreLink, PartsEngine_PreLink),
	    // for versions without PartsEngine.Init
	    HLL_EXPORT(_ModuleInit, PartsEngine_ModuleInit),
	    HLL_EXPORT(_ModuleFini, PartsEngine_ModuleFini),
	    // Oyako Rankan
	    HLL_EXPORT(Init, PE_Init),
	    HLL_EXPORT(Update, PartsEngine_Update),
	    HLL_TODO_EXPORT(UpdateGUIController, PartsEngine_UpdateGUIController),
	    HLL_EXPORT(GetFreeSystemPartsNumber, PE_GetFreeNumber),
	    // FIXME: what is the difference?
	    HLL_EXPORT(GetFreeSystemPartsNumberNotSaved, PE_GetFreeNumber),
	    HLL_EXPORT(IsExistParts, PE_IsExist),
	    HLL_EXPORT(SetPartsCG, PE_SetPartsCG),
	    HLL_EXPORT(GetPartsCGName, PE_GetPartsCGName),
	    HLL_EXPORT(SetPartsCGSurfaceArea, PE_SetPartsCGSurfaceArea),
	    HLL_EXPORT(SetLoopCG, PE_SetLoopCG),
	    HLL_EXPORT(SetLoopCGSurfaceArea, PE_SetLoopCGSurfaceArea),
	    HLL_EXPORT(SetText, PE_SetText),
	    HLL_EXPORT(AddPartsText, PE_AddPartsText),
	    HLL_TODO_EXPORT(DeletePartsTopTextLine, PartsEngine_DeletePartsTopTextLine),
	    HLL_EXPORT(SetPartsTextSurfaceArea, PE_SetPartsTextSurfaceArea),
	    HLL_TODO_EXPORT(SetPartsTextHighlight, PartsEngine_SetPartsTextHighlight),
	    HLL_TODO_EXPORT(AddPartsTextHighlight, PartsEngine_AddPartsTextHighlight),
	    HLL_TODO_EXPORT(ClearPartsTextHighlight, PartsEngine_ClearPartsTextHighlight),
	    HLL_TODO_EXPORT(SetPartsTextCountReturn, PartsEngine_SetPartsTextCountReturn),
	    HLL_TODO_EXPORT(GetPartsTextCountReturn, PartsEngine_GetPartsTextCountReturn),
	    HLL_EXPORT(SetFont, PE_SetFont),
	    HLL_EXPORT(SetPartsFontType, PE_SetPartsFontType),
	    HLL_EXPORT(SetPartsFontSize, PE_SetPartsFontSize),
	    HLL_EXPORT(SetPartsFontColor, PE_SetPartsFontColor),
	    HLL_EXPORT(SetPartsFontBoldWeight, PE_SetPartsFontBoldWeight),
	    HLL_EXPORT(SetPartsFontEdgeColor, PE_SetPartsFontEdgeColor),
	    HLL_EXPORT(SetPartsFontEdgeWeight, PE_SetPartsFontEdgeWeight),
	    HLL_EXPORT(SetTextCharSpace, PE_SetTextCharSpace),
	    HLL_EXPORT(SetTextLineSpace, PE_SetTextLineSpace),
	    HLL_EXPORT(SetHGaugeCG, PE_SetHGaugeCG),
	    HLL_EXPORT(SetHGaugeRate, PE_SetHGaugeRate_int),
	    HLL_EXPORT(SetVGaugeCG, PE_SetVGaugeCG),
	    HLL_EXPORT(SetVGaugeRate, PE_SetVGaugeRate_int),
	    HLL_EXPORT(SetHGaugeSurfaceArea, PE_SetHGaugeSurfaceArea),
	    HLL_EXPORT(SetVGaugeSurfaceArea, PE_SetVGaugeSurfaceArea),
	    HLL_EXPORT(SetNumeralCG, PE_SetNumeralCG),
	    HLL_EXPORT(SetNumeralLinkedCGNumberWidthWidthList, PE_SetNumeralLinkedCGNumberWidthWidthList),
	    HLL_TODO_EXPORT(SetNumeralFont, PartsEngine_SetNumeralFont),
	    HLL_EXPORT(SetNumeralNumber, PE_SetNumeralNumber),
	    HLL_EXPORT(SetNumeralShowComma, PE_SetNumeralShowComma),
	    HLL_EXPORT(SetNumeralSpace, PE_SetNumeralSpace),
	    HLL_EXPORT(SetNumeralLength, PE_SetNumeralLength),
	    HLL_EXPORT(SetNumeralSurfaceArea, PE_SetNumeralSurfaceArea),
	    HLL_EXPORT(SetPartsRectangleDetectionSize, PE_SetPartsRectangleDetectionSize),
	    HLL_TODO_EXPORT(SetPartsRectangleDetectionSurfaceArea, PartsEngine_SetPartsRectangleDetectionSurfaceArea),
	    HLL_EXPORT(SetPartsCGDetectionSize, PE_SetPartsCGDetectionSize),
	    HLL_TODO_EXPORT(SetPartsCGDetectionSurfaceArea, PartsEngine_SetPartsCGDetectionSurfaceArea),
	    HLL_EXPORT(SetPartsFlash, PE_SetPartsFlash),
	    HLL_EXPORT(IsPartsFlashEnd, PE_IsPartsFlashEnd),
	    HLL_EXPORT(GetPartsFlashCurrentFrameNumber, PE_GetPartsFlashCurrentFrameNumber),
	    HLL_EXPORT(BackPartsFlashBeginFrame, PE_BackPartsFlashBeginFrame),
	    HLL_EXPORT(StepPartsFlashFinalFrame, PE_StepPartsFlashFinalFrame),
	    HLL_TODO_EXPORT(SetPartsFlashSurfaceArea, PE_SetPartsFlashSurfaceArea),
	    HLL_EXPORT(SetPartsFlashAndStop, PE_SetPartsFlashAndStop),
	    HLL_EXPORT(StopPartsFlash, PE_StopPartsFlash),
	    HLL_EXPORT(StartPartsFlash, PE_StartPartsFlash),
	    HLL_EXPORT(GoFramePartsFlash, PE_GoFramePartsFlash),
	    HLL_EXPORT(GetPartsFlashEndFrame, PE_GetPartsFlashEndFrame),
	    HLL_EXPORT(ExistsFlashFile, PE_ExistsFlashFile),
	    HLL_EXPORT(ClearPartsConstructionProcess, PE_ClearPartsConstructionProcess),
	    HLL_EXPORT(AddCreateToPartsConstructionProcess, PE_AddCreateToPartsConstructionProcess),
	    HLL_EXPORT(AddCreatePixelOnlyToPartsConstructionProcess, PE_AddCreatePixelOnlyToPartsConstructionProcess),
	    HLL_EXPORT(AddCreateCGToProcess, PE_AddCreateCGToProcess),
	    HLL_EXPORT(AddFillToPartsConstructionProcess, PE_AddFillToPartsConstructionProcess),
	    HLL_EXPORT(AddFillAlphaColorToPartsConstructionProcess, PE_AddFillAlphaColorToPartsConstructionProcess),
	    HLL_EXPORT(AddFillAMapToPartsConstructionProcess, PE_AddFillAMapToPartsConstructionProcess),
	    HLL_EXPORT(AddFillWithAlphaToPartsConstructionProcess, PE_AddFillWithAlphaToPartsConstructionProcess),
	    HLL_TODO_EXPORT(AddFillGradationHorizonToPartsConstructionProcess, PartsEngine_AddFillGradationHorizonToPartsConstructionProcess),
	    HLL_EXPORT(AddDrawRectToPartsConstructionProcess, PE_AddDrawRectToPartsConstructionProcess),
	    HLL_EXPORT(AddDrawCutCGToPartsConstructionProcess, PartsEngine_AddDrawCutCGToPartsConstructionProcess_old),
	    HLL_EXPORT(AddCopyCutCGToPartsConstructionProcess, PartsEngine_AddCopyCutCGToPartsConstructionProcess_old),
	    HLL_EXPORT(AddGrayFilterToPartsConstructionProcess, PE_AddGrayFilterToPartsConstructionProcess),
	    HLL_TODO_EXPORT(AddAddFilterToPartsConstructionProcess, PartsEngine_AddAddFilterToPartsConstructionProcess),
	    HLL_TODO_EXPORT(AddMulFilterToPartsConstructionProcess, PartsEngine_AddMulFilterToPartsConstructionProcess),
	    HLL_EXPORT(BuildPartsConstructionProcess, PE_BuildPartsConstructionProcess),
	    HLL_EXPORT(AddDrawTextToPartsConstructionProcess, PE_AddDrawTextToPartsConstructionProcess),
	    HLL_EXPORT(AddCopyTextToPartsConstructionProcess, PE_AddCopyTextToPartsConstructionProcess),
	    HLL_EXPORT(SetPartsConstructionSurfaceArea, PE_SetPartsConstructionSurfaceArea),
	    HLL_EXPORT(ReleaseParts, PE_ReleaseParts),
	    HLL_EXPORT(ReleaseAllParts, PE_ReleaseAllParts),
	    HLL_EXPORT(ReleaseAllPartsWithoutSystem, PE_ReleaseAllPartsWithoutSystem),
	    HLL_EXPORT(SetPos, PE_SetPos),
	    HLL_EXPORT(SetZ, PE_SetZ),
	    HLL_EXPORT(SetShow, PE_SetShow),
	    HLL_EXPORT(SetAlpha, PE_SetAlpha),
	    HLL_EXPORT(SetPartsDrawFilter, PE_SetPartsDrawFilter),
	    HLL_EXPORT(SetAddColor, PE_SetAddColor),
	    HLL_EXPORT(SetMultiplyColor, PE_SetMultiplyColor),
	    HLL_EXPORT(SetPassCursor, PE_SetPassCursor),
	    HLL_EXPORT(SetClickable, PE_SetClickable),
	    HLL_EXPORT(SetSpeedupRateByMessageSkip, PE_SetSpeedupRateByMessageSkip),
	    HLL_TODO_EXPORT(SetResetTimerByChangeInputStatus, PartsEngine_SetResetTimerByChangeInputStatus),
	    HLL_EXPORT(GetPartsX, PE_GetPartsX),
	    HLL_EXPORT(GetPartsY, PE_GetPartsY),
	    HLL_EXPORT(GetPartsZ, PE_GetPartsZ),
	    HLL_EXPORT(GetPartsShow, PE_GetPartsShow),
	    HLL_EXPORT(GetPartsAlpha, PE_GetPartsAlpha),
	    HLL_TODO_EXPORT(GetAddColor, PartsEngine_GetAddColor),
	    HLL_TODO_EXPORT(GetMultiplyColor, PartsEngine_GetMultiplyColor),
	    HLL_EXPORT(GetPartsPassCursor, PE_GetPartsPassCursor),
	    HLL_EXPORT(GetPartsClickable, PE_GetPartsClickable),
	    HLL_TODO_EXPORT(GetPartsSpeedupRateByMessageSkip, PartsEngine_GetPartsSpeedupRateByMessageSkip),
	    HLL_TODO_EXPORT(GetResetTimerByChangeInputStatus, PartsEngine_GetResetTimerByChangeInputStatus),
	    HLL_EXPORT(GetPartsUpperLeftPosX, PE_GetPartsUpperLeftPosX),
	    HLL_EXPORT(GetPartsUpperLeftPosY, PE_GetPartsUpperLeftPosY),
	    HLL_EXPORT(GetPartsWidth, PE_GetPartsWidth),
	    HLL_EXPORT(GetPartsHeight, PE_GetPartsHeight),
	    HLL_EXPORT(SetInputState, PE_SetInputState),
	    HLL_EXPORT(GetInputState, PE_GetInputState),
	    HLL_EXPORT(SetPartsOriginPosMode, PE_SetPartsOriginPosMode),
	    HLL_EXPORT(GetPartsOriginPosMode, PE_GetPartsOriginPosMode),
	    HLL_EXPORT(SetParentPartsNumber, PE_SetParentPartsNumber),
	    HLL_EXPORT(SetPartsGroupNumber, PE_SetPartsGroupNumber),
	    HLL_EXPORT(SetPartsGroupDecideOnCursor, PE_SetPartsGroupDecideOnCursor),
	    HLL_EXPORT(SetPartsGroupDecideClick, PE_SetPartsGroupDecideClick),
	    HLL_EXPORT(SetOnCursorShowLinkPartsNumber, PE_SetOnCursorShowLinkPartsNumber),
	    HLL_EXPORT(SetPartsMessageWindowShowLink, PE_SetPartsMessageWindowShowLink),
	    HLL_EXPORT(GetPartsMessageWindowShowLink, PE_GetPartsMessageWindowShowLink),
	    HLL_EXPORT(AddMotionPos, PE_AddMotionPos_curve),
	    HLL_EXPORT(AddMotionAlpha, PE_AddMotionAlpha_curve),
	    HLL_TODO_EXPORT(AddMotionCG, PartsEngine_AddMotionCG),
	    HLL_EXPORT(AddMotionHGaugeRate, PE_AddMotionHGaugeRate_curve),
	    HLL_EXPORT(AddMotionVGaugeRate, PE_AddMotionVGaugeRate_curve),
	    HLL_EXPORT(AddMotionNumeralNumber, PE_AddMotionNumeralNumber_curve),
	    HLL_EXPORT(AddMotionMagX, PE_AddMotionMagX_curve),
	    HLL_EXPORT(AddMotionMagY, PE_AddMotionMagY_curve),
	    HLL_EXPORT(AddMotionRotateX, PE_AddMotionRotateX_curve),
	    HLL_EXPORT(AddMotionRotateY, PE_AddMotionRotateY_curve),
	    HLL_EXPORT(AddMotionRotateZ, PE_AddMotionRotateZ_curve),
	    HLL_EXPORT(AddMotionVibrationSize, PE_AddMotionVibrationSize),
	    HLL_EXPORT(AddWholeMotionVibrationSize, PE_AddWholeMotionVibrationSize),
	    HLL_EXPORT(AddMotionSound, PE_AddMotionSound),
	    HLL_TODO_EXPORT(SetSoundNumber, PartsEngine_SetSoundNumber),
	    HLL_TODO_EXPORT(GetSoundNumber, PartsEngine_GetSoundNumber),
	    HLL_EXPORT(SetClickMissSoundNumber, PE_SetClickMissSoundNumber),
	    HLL_TODO_EXPORT(GetClickMissSoundNumber, PartsEngine_GetClickMissSoundNumber),
	    HLL_EXPORT(BeginMotion, PE_BeginMotion),
	    HLL_EXPORT(EndMotion, PE_EndMotion),
	    HLL_EXPORT(IsMotion, PE_IsMotion),
	    HLL_EXPORT(SeekEndMotion, PE_SeekEndMotion),
	    HLL_EXPORT(UpdateMotionTime, PE_UpdateMotionTime),
	    HLL_EXPORT(BeginInput, PE_BeginInput),
	    HLL_EXPORT(EndInput, PE_EndInput),
	    HLL_EXPORT(GetClickPartsNumber, PE_GetClickPartsNumber),
	    HLL_TODO_EXPORT(GetFocusPartsNumber, PartsEngine_GetFocusPartsNumber),
	    HLL_TODO_EXPORT(SetFocusPartsNumber, PartsEngine_SetFocusPartsNumber),
	    HLL_TODO_EXPORT(PushGUIController, PartsEngine_PushGUIController),
	    HLL_TODO_EXPORT(PopGUIController, PartsEngine_PopGUIController),
	    HLL_EXPORT(SetPartsMagX, PE_SetPartsMagX),
	    HLL_EXPORT(SetPartsMagY, PE_SetPartsMagY),
	    HLL_EXPORT(SetPartsRotateX, PE_SetPartsRotateX),
	    HLL_EXPORT(SetPartsRotateY, PE_SetPartsRotateY),
	    HLL_EXPORT(SetPartsRotateZ, PE_SetPartsRotateZ),
	    HLL_EXPORT(SetPartsAlphaClipperPartsNumber, PE_SetPartsAlphaClipperPartsNumber),
	    HLL_EXPORT(SetPartsPixelDecide, PE_SetPartsPixelDecide),
	    HLL_EXPORT(IsCursorIn, PE_IsCursorIn),
	    HLL_EXPORT(SetThumbnailReductionSize, PE_SetThumbnailReductionSize),
	    HLL_EXPORT(SetThumbnailMode, PE_SetThumbnailMode),
	    HLL_EXPORT(Save, PE_Save),
	    HLL_EXPORT(SaveWithoutHideParts, PE_SaveWithoutHideParts),
	    HLL_EXPORT(Load, PE_Load),
	    HLL_EXPORT(SaveBackScene, PE_SaveBackScene),
	    HLL_EXPORT(LoadBackScene, PE_LoadBackScene),
	    HLL_EXPORT(SetWantSaveBackScene, PartsEngine_SetWantSaveBackScene),
	    HLL_EXPORT(IsWantSaveBackScene, PartsEngine_IsWantSaveBackScene),
	    HLL_EXPORT(ClearBackScene, PartsEngine_ClearBackScene),
	    // Rance 9
	    HLL_EXPORT(PartsFunc, PartsEngine_PartsFunc),
	    HLL_EXPORT(Release, PE_ReleaseParts),
	    HLL_TODO_EXPORT(ReleaseAll, PartsEngine_ReleaseAll),
	    HLL_EXPORT(ReleaseAllWithoutSystem, PE_ReleaseAllWithoutSystem),
	    HLL_EXPORT(GetFreeNumber, PE_GetFreeNumber),
	    HLL_EXPORT(IsExist, PE_IsExist),
	    HLL_EXPORT(AddController, PE_AddController),
	    HLL_EXPORT(RemoveController, PE_RemoveController),
	    HLL_EXPORT(SetActiveController, PE_set_active_controller),
	    HLL_EXPORT(GetActiveController, PE_get_active_controller),
	    HLL_EXPORT(GetControllerIndex, PE_GetControllerIndex),
	    HLL_EXPORT(GetControllerID, PE_GetControllerID),
	    HLL_EXPORT(GetControllerLength, PE_get_nr_controllers),
	    HLL_EXPORT(GetSystemOverlayController, PE_get_system_controller),
	    HLL_EXPORT(UpdateComponent, PartsEngine_Update),
	    HLL_EXPORT(Parts_SetThumbnailReductionSize, PE_SetThumbnailReductionSize),
	    HLL_EXPORT(Parts_SetThumbnailMode, PE_SetThumbnailMode),
	    HLL_EXPORT(GetClickNumber, PE_GetClickPartsNumber),
	    HLL_EXPORT(StopSoundWithoutSystemSound, PartsEngine_StopSoundWithoutSystemSound),
	    // Rance 03 activity system (name-keyed; see the comment above
	    // activity_find for the interface-generation background)
	    HLL_EXPORT(IsExistActivity, PartsEngine_IsExistActivity),
	    HLL_EXPORT(CreateActivity, PartsEngine_CreateActivity),
	    HLL_EXPORT(ReleaseActivity, PartsEngine_ReleaseActivity),
	    HLL_EXPORT(SaveActivityEXText, PartsEngine_SaveActivityEXText),
	    HLL_EXPORT(LoadActivityEXText, PartsEngine_LoadActivityEXText),
	    HLL_EXPORT(ReadActivityFile, PartsEngine_ReadActivityFile),
	    HLL_EXPORT(WriteActivityFile, PartsEngine_WriteActivityFile),
	    HLL_EXPORT(IsExistActivityFile, PartsEngine_IsExistActivityFile),
	    HLL_EXPORT(AddActivityParts, PartsEngine_AddActivityParts),
	    HLL_EXPORT(RemoveActivityParts, PartsEngine_RemoveActivityParts),
	    HLL_EXPORT(RemoveAllActivityParts, PartsEngine_RemoveAllActivityParts),
	    HLL_EXPORT(NumofActivityParts, PartsEngine_NumofActivityParts),
	    HLL_EXPORT(GetActivityParts, PartsEngine_GetActivityParts),
	    HLL_EXPORT(IsExistActivityPartsByName, PartsEngine_IsExistActivityPartsByName),
	    HLL_EXPORT(IsExistActivityPartsByNumber, PartsEngine_IsExistActivityPartsByNumber),
	    HLL_EXPORT(GetActivityPartsNumber, PartsEngine_GetActivityPartsNumber),
	    HLL_EXPORT(GetActivityPartsName, PartsEngine_GetActivityPartsName),
	    HLL_EXPORT(AddActivityCloseParts, PartsEngine_AddActivityCloseParts),
	    HLL_EXPORT(RemoveActivityCloseParts, PartsEngine_RemoveActivityCloseParts),
	    HLL_EXPORT(RemoveAllActivityCloseParts, PartsEngine_RemoveAllActivityCloseParts),
	    HLL_EXPORT(IsExistActivityCloseParts, PartsEngine_IsExistActivityCloseParts),
	    HLL_EXPORT(SetActivityIntentData, PartsEngine_SetActivityIntentData),
	    HLL_EXPORT(AddActivityIntentDataDestination, PartsEngine_AddActivityIntentDataDestination),
	    HLL_EXPORT(IsExistActivityIntentData, PartsEngine_IsExistActivityIntentData),
	    HLL_EXPORT(NumofActivityIntentDataDestination, PartsEngine_NumofActivityIntentDataDestination),
	    HLL_EXPORT(GetActivityIntentDataDestination, PartsEngine_GetActivityIntentDataDestination),
	    HLL_EXPORT(GetActivityIntentDataType, PartsEngine_GetActivityIntentDataType),
	    HLL_EXPORT(SetActivityEndKey, PartsEngine_SetActivityEndKey),
	    HLL_EXPORT(EraseActivityEndKey, PartsEngine_EraseActivityEndKey),
	    HLL_EXPORT(IsExistActivityEndKey, PartsEngine_IsExistActivityEndKey),
	    HLL_EXPORT(NumofActivityEndKey, PartsEngine_NumofActivityEndKey),
	    HLL_EXPORT(GetActivityEndKey, PartsEngine_GetActivityEndKey),
	    HLL_TODO_EXPORT(CrateActivityBinary, PartsEngine_CrateActivityBinary),
	    HLL_TODO_EXPORT(ReadActivityBinary, PartsEngine_ReadActivityBinary),
	    HLL_EXPORT(ReleaseMessage, PE_ReleaseMessage),
	    HLL_EXPORT(PopMessage, PE_PopMessage),
	    HLL_EXPORT(GetMessagePartsNumber, PE_GetMessagePartsNumber),
	    HLL_EXPORT(GetMessageDelegateIndex, PE_GetMessageDelegateIndex),
	    HLL_EXPORT(GetDelegateIndex, PE_GetDelegateIndex),
	    HLL_EXPORT(GetMessageType, PE_GetMessageType),
	    HLL_EXPORT(GetMessageVariableCount, PE_GetMessageVariableCount),
	    HLL_EXPORT(GetMessageVariableType, PE_GetMessageVariableType),
	    HLL_EXPORT(GetMessageVariableInt, PE_GetMessageVariableInt),
	    HLL_EXPORT(GetMessageVariableFloat, PE_GetMessageVariableFloat),
	    HLL_EXPORT(GetMessageVariableBool, PE_GetMessageVariableBool),
	    HLL_EXPORT(GetMessageVariableString, PE_GetMessageVariableString),
	    HLL_EXPORT(SetDelegateIndex, PE_SetDelegateIndex),
	    HLL_TODO_EXPORT(SetFocus, PartsEngine_SetFocus),
	    HLL_TODO_EXPORT(IsFocus, PartsEngine_IsFocus),
	    HLL_EXPORT(SetComponentType, PE_SetComponentType),
	    HLL_EXPORT(GetComponentType, PE_GetComponentType),
	    HLL_EXPORT(SetComponentPos, PartsEngine_SetComponentPos),
	    HLL_EXPORT(SetComponentPosZ, PE_SetZ),
	    HLL_EXPORT(GetComponentPosX, PartsEngine_Parts_GetComponentPosX),
	    HLL_EXPORT(GetComponentPosY, PartsEngine_GetComponentPosY),
	    HLL_EXPORT(GetComponentPosZ, PE_GetPartsZ),
	    HLL_EXPORT(Parts_GetPartsUpperLeftPosX, PartsEngine_Parts_GetPartsUpperLeftPosX),
	    HLL_EXPORT(Parts_GetPartsUpperLeftPosY, PartsEngine_Parts_GetPartsUpperLeftPosY),
	    HLL_EXPORT(SetComponentOriginPosMode, PE_SetPartsOriginPosMode),
	    HLL_EXPORT(GetComponentOriginPosMode, PE_GetPartsOriginPosMode),
	    HLL_TODO_EXPORT(GetComponentWidth, PartsEngine_GetComponentWidth),
	    HLL_TODO_EXPORT(GetComponentHeight, PartsEngine_GetComponentHeight),
	    HLL_EXPORT(Parts_GetPartsWidth, PE_GetPartsWidth),
	    HLL_EXPORT(Parts_GetPartsHeight, PE_GetPartsHeight),
	    HLL_EXPORT(SetComponentShow, PE_SetShow),
	    HLL_EXPORT(IsComponentShow, PE_GetPartsShow),
	    HLL_EXPORT(SetComponentMessageWindowShowLink, PE_SetPartsMessageWindowShowLink),
	    HLL_EXPORT(IsComponentMessageWindowShowLink, PE_GetPartsMessageWindowShowLink),
	    HLL_EXPORT(SetComponentAlpha, PE_SetAlpha),
	    HLL_EXPORT(GetComponentAlpha, PE_GetPartsAlpha),
	    HLL_EXPORT(SetComponentAddColor, PE_SetAddColor),
	    HLL_TODO_EXPORT(GetComponentAddColorR, PartsEngine_GetComponentAddColorR),
	    HLL_TODO_EXPORT(GetComponentAddColorG, PartsEngine_GetComponentAddColorG),
	    HLL_TODO_EXPORT(GetComponentAddColorB, PartsEngine_GetComponentAddColorB),
	    HLL_EXPORT(SetComponentMulColor, PE_SetMultiplyColor),
	    HLL_TODO_EXPORT(GetComponentMulColorR, PartsEngine_GetComponentMulColorR),
	    HLL_TODO_EXPORT(GetComponentMulColorG, PartsEngine_GetComponentMulColorG),
	    HLL_TODO_EXPORT(GetComponentMulColorB, PartsEngine_GetComponentMulColorB),
	    HLL_EXPORT(SetComponentDrawFilter, PE_SetPartsDrawFilter),
	    HLL_TODO_EXPORT(GetComponentDrawFilter, PartsEngine_GetComponentDrawFilter),
	    HLL_EXPORT(SetComponentMagX, PE_SetPartsMagX),
	    HLL_EXPORT(SetComponentMagY, PE_SetPartsMagY),
	    HLL_TODO_EXPORT(GetComponentMagX, PartsEngine_GetComponentMagX),
	    HLL_TODO_EXPORT(GetComponentMagY, PartsEngine_GetComponentMagY),
	    HLL_EXPORT(SetComponentRotateX, PE_SetPartsRotateX),
	    HLL_EXPORT(SetComponentRotateY, PE_SetPartsRotateY),
	    HLL_EXPORT(SetComponentRotateZ, PE_SetPartsRotateZ),
	    HLL_TODO_EXPORT(GetComponentRotateX, PartsEngine_GetComponentRotateX),
	    HLL_TODO_EXPORT(GetComponentRotateY, PartsEngine_GetComponentRotateY),
	    HLL_EXPORT(GetComponentRotateZ, PE_GetPartsRotateZ),
	    HLL_EXPORT(SetComponentMargin, PE_SetComponentMargin),
	    HLL_EXPORT(GetComponentMarginTop, PE_GetComponentMarginTop),
	    HLL_EXPORT(GetComponentMarginBottom, PE_GetComponentMarginBottom),
	    HLL_EXPORT(GetComponentMarginLeft, PE_GetComponentMarginLeft),
	    HLL_EXPORT(GetComponentMarginRight, PE_GetComponentMarginRight),
	    HLL_EXPORT(SetComponentAlphaClipper, PE_SetPartsAlphaClipperPartsNumber),
	    HLL_TODO_EXPORT(GetComponentAlphaClipper, PartsEngine_GetComponentAlphaClipper),
	    HLL_TODO_EXPORT(SetComponentTextureFilterType, PartsEngine_SetComponentTextureFilterType),
	    HLL_TODO_EXPORT(GetComponentTextureFilterType, PartsEngine_GetComponentTextureFilterType),
	    HLL_TODO_EXPORT(SetComponentMipmap, PartsEngine_SetComponentMipmap),
	    HLL_TODO_EXPORT(IsComponentMipmap, PartsEngine_IsComponentMipmap),
	    HLL_EXPORT(SetComponentSpeedupRateByMessageSkip, PE_SetSpeedupRateByMessageSkip),
	    HLL_EXPORT(GetComponentSpeedupRateByMessageSkip, PE_GetSpeedupRateByMessageSkip),
	    HLL_EXPORT(AddComponentMotionPos, PartsEngine_AddComponentMotionPos),
	    HLL_EXPORT(AddComponentMotionAlpha, PE_AddMotionAlpha_curve),
	    HLL_TODO_EXPORT(AddComponentMotionCG, PartsEngine_AddComponentMotionCG),
	    HLL_TODO_EXPORT(AddComponentMotionCGTermination, PartsEngine_AddComponentMotionCGTermination),
	    HLL_EXPORT(AddComponentMotionHGaugeRate, PE_AddMotionHGaugeRate_curve),
	    HLL_EXPORT(AddComponentMotionVGaugeRate, PE_AddMotionVGaugeRate_curve),
	    HLL_EXPORT(AddComponentMotionNumeralNumber, PE_AddMotionNumeralNumber_curve),
	    HLL_EXPORT(AddComponentMotionMagX, PE_AddMotionMagX_curve),
	    HLL_EXPORT(AddComponentMotionMagY, PE_AddMotionMagY_curve),
	    HLL_EXPORT(AddComponentMotionRotateX, PE_AddMotionRotateX_curve),
	    HLL_EXPORT(AddComponentMotionRotateY, PE_AddMotionRotateY_curve),
	    HLL_EXPORT(AddComponentMotionRotateZ, PE_AddMotionRotateZ_curve),
	    HLL_EXPORT(AddComponentMotionVibrationSize, PE_AddMotionVibrationSize),
	    HLL_TODO_EXPORT(SuspendBuildView, PartsEngine_SuspendBuildView),
	    HLL_TODO_EXPORT(SuspendBuildViewAt, PartsEngine_SuspendBuildViewAt),
	    HLL_TODO_EXPORT(ResumeBuildView, PartsEngine_ResumeBuildView),
	    HLL_TODO_EXPORT(SetButtonSize, PartsEngine_SetButtonSize),
	    HLL_TODO_EXPORT(SetButtonDrag, PartsEngine_SetButtonDrag),
	    HLL_TODO_EXPORT(IsButtonDrag, PartsEngine_IsButtonDrag),
	    HLL_TODO_EXPORT(SetButtonEnable, PartsEngine_SetButtonEnable),
	    HLL_TODO_EXPORT(IsButtonEnable, PartsEngine_IsButtonEnable),
	    HLL_TODO_EXPORT(SetButtonPixelDecide, PartsEngine_SetButtonPixelDecide),
	    HLL_TODO_EXPORT(IsButtonPixelDecide, PartsEngine_IsButtonPixelDecide),
	    HLL_TODO_EXPORT(SetButtonColor, PartsEngine_SetButtonColor),
	    HLL_TODO_EXPORT(GetButtonR, PartsEngine_GetButtonR),
	    HLL_TODO_EXPORT(GetButtonG, PartsEngine_GetButtonG),
	    HLL_TODO_EXPORT(GetButtonB, PartsEngine_GetButtonB),
	    HLL_TODO_EXPORT(SetButtonFontProperty, PartsEngine_SetButtonFontProperty),
	    HLL_TODO_EXPORT(GetButtonFontProperty, PartsEngine_GetButtonFontProperty),
	    HLL_TODO_EXPORT(SetButtonOnCursorSoundNumber, PartsEngine_SetButtonOnCursorSoundNumber),
	    HLL_TODO_EXPORT(SetButtonClickSoundNumber, PartsEngine_SetButtonClickSoundNumber),
	    HLL_TODO_EXPORT(GetButtonOnCursorSoundNumber, PartsEngine_GetButtonOnCursorSoundNumber),
	    HLL_TODO_EXPORT(GetButtonClickSoundNumber, PartsEngine_GetButtonClickSoundNumber),
	    HLL_TODO_EXPORT(SetButtonCGName, PartsEngine_SetButtonCGName),
	    HLL_TODO_EXPORT(GetButtonCGName, PartsEngine_GetButtonCGName),
	    HLL_TODO_EXPORT(SetButtonText, PartsEngine_SetButtonText),
	    HLL_TODO_EXPORT(GetButtonText, PartsEngine_GetButtonText),
	    HLL_TODO_EXPORT(SetCheckBoxSize, PartsEngine_SetCheckBoxSize),
	    HLL_TODO_EXPORT(SetCheckBoxDrag, PartsEngine_SetCheckBoxDrag),
	    HLL_TODO_EXPORT(IsCheckBoxDrag, PartsEngine_IsCheckBoxDrag),
	    HLL_EXPORT(CheckBoxChecked, PartsEngine_CheckBoxChecked),
	    HLL_EXPORT(IsCheckBoxChecked, PartsEngine_IsCheckBoxChecked),
	    HLL_EXPORT(SetCheckBoxColor, PartsEngine_SetCheckBoxColor),
	    HLL_EXPORT(GetCheckBoxR, PartsEngine_GetCheckBoxR),
	    HLL_EXPORT(GetCheckBoxG, PartsEngine_GetCheckBoxG),
	    HLL_EXPORT(GetCheckBoxB, PartsEngine_GetCheckBoxB),
	    HLL_TODO_EXPORT(SetCheckBoxFontProperty, PartsEngine_SetCheckBoxFontProperty),
	    HLL_TODO_EXPORT(GetCheckBoxFontProperty, PartsEngine_GetCheckBoxFontProperty),
	    HLL_TODO_EXPORT(SetCheckBoxOnCursorSoundNumber, PartsEngine_SetCheckBoxOnCursorSoundNumber),
	    HLL_TODO_EXPORT(SetCheckBoxClickSoundNumber, PartsEngine_SetCheckBoxClickSoundNumber),
	    HLL_TODO_EXPORT(GetCheckBoxOnCursorSoundNumber, PartsEngine_GetCheckBoxOnCursorSoundNumber),
	    HLL_TODO_EXPORT(GetCheckBoxClickSoundNumber, PartsEngine_GetCheckBoxClickSoundNumber),
	    HLL_TODO_EXPORT(SetCheckBoxCGName, PartsEngine_SetCheckBoxCGName),
	    HLL_TODO_EXPORT(GetCheckBoxCGName, PartsEngine_GetCheckBoxCGName),
	    HLL_TODO_EXPORT(SetCheckBoxText, PartsEngine_SetCheckBoxText),
	    HLL_TODO_EXPORT(GetCheckBoxText, PartsEngine_GetCheckBoxText),
	    HLL_TODO_EXPORT(SetVScrollbarOnCursorSoundNumber, PartsEngine_SetVScrollbarOnCursorSoundNumber),
	    HLL_TODO_EXPORT(SetVScrollbarClickSoundNumber, PartsEngine_SetVScrollbarClickSoundNumber),
	    HLL_TODO_EXPORT(GetVScrollbarOnCursorSoundNumber, PartsEngine_GetVScrollbarOnCursorSoundNumber),
	    HLL_TODO_EXPORT(GetVScrollbarClickSoundNumber, PartsEngine_GetVScrollbarClickSoundNumber),
	    HLL_TODO_EXPORT(SetVScrollbarSize, PartsEngine_SetVScrollbarSize),
	    HLL_TODO_EXPORT(SetVScrollbarUpHeight, PartsEngine_SetVScrollbarUpHeight),
	    HLL_TODO_EXPORT(SetVScrollbarDownHeight, PartsEngine_SetVScrollbarDownHeight),
	    HLL_TODO_EXPORT(GetVScrollbarUpHeight, PartsEngine_GetVScrollbarUpHeight),
	    HLL_TODO_EXPORT(GetVScrollbarDownHeight, PartsEngine_GetVScrollbarDownHeight),
	    HLL_TODO_EXPORT(SetVScrollbarTotalSize, PartsEngine_SetVScrollbarTotalSize),
	    HLL_TODO_EXPORT(SetVScrollbarViewSize, PartsEngine_SetVScrollbarViewSize),
	    HLL_TODO_EXPORT(SetVScrollbarScrollPos, PartsEngine_SetVScrollbarScrollPos),
	    HLL_TODO_EXPORT(SetVScrollbarScrollRate, PartsEngine_SetVScrollbarScrollRate),
	    HLL_TODO_EXPORT(SetVScrollbarMoveSizeByButton, PartsEngine_SetVScrollbarMoveSizeByButton),
	    HLL_TODO_EXPORT(GetVScrollbarTotalSize, PartsEngine_GetVScrollbarTotalSize),
	    HLL_TODO_EXPORT(GetVScrollbarViewSize, PartsEngine_GetVScrollbarViewSize),
	    HLL_TODO_EXPORT(GetVScrollbarScrollPos, PartsEngine_GetVScrollbarScrollPos),
	    HLL_TODO_EXPORT(GetVScrollbarScrollRate, PartsEngine_GetVScrollbarScrollRate),
	    HLL_TODO_EXPORT(GetVScrollbarMoveSizeByButton, PartsEngine_GetVScrollbarMoveSizeByButton),
	    HLL_TODO_EXPORT(SetVScrollbarCGName, PartsEngine_SetVScrollbarCGName),
	    HLL_TODO_EXPORT(GetVScrollbarCGName, PartsEngine_GetVScrollbarCGName),
	    HLL_TODO_EXPORT(SetHScrollbarOnCursorSoundNumber, PartsEngine_SetHScrollbarOnCursorSoundNumber),
	    HLL_TODO_EXPORT(SetHScrollbarClickSoundNumber, PartsEngine_SetHScrollbarClickSoundNumber),
	    HLL_TODO_EXPORT(GetHScrollbarOnCursorSoundNumber, PartsEngine_GetHScrollbarOnCursorSoundNumber),
	    HLL_TODO_EXPORT(GetHScrollbarClickSoundNumber, PartsEngine_GetHScrollbarClickSoundNumber),
	    HLL_TODO_EXPORT(SetHScrollbarSize, PartsEngine_SetHScrollbarSize),
	    HLL_TODO_EXPORT(SetHScrollbarLeftWidth, PartsEngine_SetHScrollbarLeftWidth),
	    HLL_TODO_EXPORT(SetHScrollbarRightWidth, PartsEngine_SetHScrollbarRightWidth),
	    HLL_TODO_EXPORT(GetHScrollbarLeftWidth, PartsEngine_GetHScrollbarLeftWidth),
	    HLL_TODO_EXPORT(GetHScrollbarRightWidth, PartsEngine_GetHScrollbarRightWidth),
	    HLL_TODO_EXPORT(SetHScrollbarTotalSize, PartsEngine_SetHScrollbarTotalSize),
	    HLL_TODO_EXPORT(SetHScrollbarViewSize, PartsEngine_SetHScrollbarViewSize),
	    HLL_TODO_EXPORT(SetHScrollbarScrollPos, PartsEngine_SetHScrollbarScrollPos),
	    HLL_TODO_EXPORT(SetHScrollbarScrollRate, PartsEngine_SetHScrollbarScrollRate),
	    HLL_TODO_EXPORT(SetHScrollbarMoveSizeByButton, PartsEngine_SetHScrollbarMoveSizeByButton),
	    HLL_TODO_EXPORT(GetHScrollbarTotalSize, PartsEngine_GetHScrollbarTotalSize),
	    HLL_TODO_EXPORT(GetHScrollbarViewSize, PartsEngine_GetHScrollbarViewSize),
	    HLL_TODO_EXPORT(GetHScrollbarScrollPos, PartsEngine_GetHScrollbarScrollPos),
	    HLL_TODO_EXPORT(GetHScrollbarScrollRate, PartsEngine_GetHScrollbarScrollRate),
	    HLL_TODO_EXPORT(GetHScrollbarMoveSizeByButton, PartsEngine_GetHScrollbarMoveSizeByButton),
	    HLL_TODO_EXPORT(SetHScrollbarCGName, PartsEngine_SetHScrollbarCGName),
	    HLL_TODO_EXPORT(GetHScrollbarCGName, PartsEngine_GetHScrollbarCGName),
	    HLL_TODO_EXPORT(SetTextBoxSize, PartsEngine_SetTextBoxSize),
	    HLL_TODO_EXPORT(SetTextBoxFontProperty, PartsEngine_SetTextBoxFontProperty),
	    HLL_TODO_EXPORT(GetTextBoxFontProperty, PartsEngine_GetTextBoxFontProperty),
	    HLL_TODO_EXPORT(SetTextBoxText, PartsEngine_SetTextBoxText),
	    HLL_TODO_EXPORT(GetTextBoxText, PartsEngine_GetTextBoxText),
	    HLL_TODO_EXPORT(SetTextBoxMaxTextLength, PartsEngine_SetTextBoxMaxTextLength),
	    HLL_TODO_EXPORT(GetTextBoxMaxTextLength, PartsEngine_GetTextBoxMaxTextLength),
	    HLL_TODO_EXPORT(SetTextBoxSelectColor, PartsEngine_SetTextBoxSelectColor),
	    HLL_TODO_EXPORT(GetTextBoxSelectR, PartsEngine_GetTextBoxSelectR),
	    HLL_TODO_EXPORT(GetTextBoxSelectG, PartsEngine_GetTextBoxSelectG),
	    HLL_TODO_EXPORT(GetTextBoxSelectB, PartsEngine_GetTextBoxSelectB),
	    HLL_TODO_EXPORT(SetTextBoxCGName, PartsEngine_SetTextBoxCGName),
	    HLL_TODO_EXPORT(GetTextBoxCGName, PartsEngine_GetTextBoxCGName),
	    HLL_TODO_EXPORT(OpenTextBoxIME, PartsEngine_OpenTextBoxIME),
	    HLL_TODO_EXPORT(CloseTextBoxIME, PartsEngine_CloseTextBoxIME),
	    HLL_TODO_EXPORT(SetListBoxSize, PartsEngine_SetListBoxSize),
	    HLL_TODO_EXPORT(SetListBoxLineHeight, PartsEngine_SetListBoxLineHeight),
	    HLL_TODO_EXPORT(GetListBoxLineHeight, PartsEngine_GetListBoxLineHeight),
	    HLL_TODO_EXPORT(SetListBoxMargin, PartsEngine_SetListBoxMargin),
	    HLL_TODO_EXPORT(GetListBoxWidthMargin, PartsEngine_GetListBoxWidthMargin),
	    HLL_TODO_EXPORT(GetListBoxHeightMargin, PartsEngine_GetListBoxHeightMargin),
	    HLL_TODO_EXPORT(SetListBoxCGName, PartsEngine_SetListBoxCGName),
	    HLL_TODO_EXPORT(GetListBoxCGName, PartsEngine_GetListBoxCGName),
	    HLL_TODO_EXPORT(SetListBoxScrollPos, PartsEngine_SetListBoxScrollPos),
	    HLL_TODO_EXPORT(GetListBoxScrollPos, PartsEngine_GetListBoxScrollPos),
	    HLL_TODO_EXPORT(AddListBoxItem, PartsEngine_AddListBoxItem),
	    HLL_TODO_EXPORT(InsertListBoxItem, PartsEngine_InsertListBoxItem),
	    HLL_TODO_EXPORT(SetListBoxItem, PartsEngine_SetListBoxItem),
	    HLL_TODO_EXPORT(GetListBoxItemCount, PartsEngine_GetListBoxItemCount),
	    HLL_TODO_EXPORT(GetListBoxItem, PartsEngine_GetListBoxItem),
	    HLL_TODO_EXPORT(EraseListBoxItem, PartsEngine_EraseListBoxItem),
	    HLL_TODO_EXPORT(ClearListBoxItem, PartsEngine_ClearListBoxItem),
	    HLL_TODO_EXPORT(GetListBoxOnCursorItemIndex, PartsEngine_GetListBoxOnCursorItemIndex),
	    HLL_TODO_EXPORT(GetListBoxOnCursorItem, PartsEngine_GetListBoxOnCursorItem),
	    HLL_TODO_EXPORT(SetListBoxFontProperty, PartsEngine_SetListBoxFontProperty),
	    HLL_TODO_EXPORT(GetListBoxFontProperty, PartsEngine_GetListBoxFontProperty),
	    HLL_TODO_EXPORT(SetListBoxSelectIndex, PartsEngine_SetListBoxSelectIndex),
	    HLL_TODO_EXPORT(GetListBoxSelectIndex, PartsEngine_GetListBoxSelectIndex),
	    HLL_TODO_EXPORT(SetComboBoxSize, PartsEngine_SetComboBoxSize),
	    HLL_TODO_EXPORT(SetComboBoxTextMargin, PartsEngine_SetComboBoxTextMargin),
	    HLL_TODO_EXPORT(GetComboBoxTextWidthMargin, PartsEngine_GetComboBoxTextWidthMargin),
	    HLL_TODO_EXPORT(GetComboBoxTextHeightMargin, PartsEngine_GetComboBoxTextHeightMargin),
	    HLL_TODO_EXPORT(SetComboBoxCGName, PartsEngine_SetComboBoxCGName),
	    HLL_TODO_EXPORT(GetComboBoxCGName, PartsEngine_GetComboBoxCGName),
	    HLL_TODO_EXPORT(SetComboBoxText, PartsEngine_SetComboBoxText),
	    HLL_TODO_EXPORT(GetComboBoxText, PartsEngine_GetComboBoxText),
	    HLL_TODO_EXPORT(SetComboBoxFontProperty, PartsEngine_SetComboBoxFontProperty),
	    HLL_TODO_EXPORT(GetComboBoxFontProperty, PartsEngine_GetComboBoxFontProperty),
	    HLL_TODO_EXPORT(SetMultiTextBoxSize, PartsEngine_SetMultiTextBoxSize),
	    HLL_TODO_EXPORT(SetMultiTextBoxFontProperty, PartsEngine_SetMultiTextBoxFontProperty),
	    HLL_TODO_EXPORT(GetMultiTextBoxFontProperty, PartsEngine_GetMultiTextBoxFontProperty),
	    HLL_TODO_EXPORT(SetMultiTextBoxText, PartsEngine_SetMultiTextBoxText),
	    HLL_TODO_EXPORT(GetMultiTextBoxText, PartsEngine_GetMultiTextBoxText),
	    HLL_TODO_EXPORT(SetMultiTextBoxMaxTextLength, PartsEngine_SetMultiTextBoxMaxTextLength),
	    HLL_TODO_EXPORT(GetMultiTextBoxMaxTextLength, PartsEngine_GetMultiTextBoxMaxTextLength),
	    HLL_TODO_EXPORT(SetMultiTextBoxSelectColor, PartsEngine_SetMultiTextBoxSelectColor),
	    HLL_TODO_EXPORT(GetMultiTextBoxSelectR, PartsEngine_GetMultiTextBoxSelectR),
	    HLL_TODO_EXPORT(GetMultiTextBoxSelectG, PartsEngine_GetMultiTextBoxSelectG),
	    HLL_TODO_EXPORT(GetMultiTextBoxSelectB, PartsEngine_GetMultiTextBoxSelectB),
	    HLL_TODO_EXPORT(SetMultiTextBoxCGName, PartsEngine_SetMultiTextBoxCGName),
	    HLL_TODO_EXPORT(GetMultiTextBoxCGName, PartsEngine_GetMultiTextBoxCGName),
	    HLL_EXPORT(SetLayoutBoxLayoutType, PE_SetLayoutBoxLayoutType),
	    HLL_EXPORT(GetLayoutBoxLayoutType, PE_GetLayoutBoxLayoutType),
	    HLL_EXPORT(SetLayoutBoxReturn, PE_SetLayoutBoxReturn),
	    HLL_EXPORT(IsLayoutBoxReturn, PE_IsLayoutBoxReturn),
	    HLL_EXPORT(GetLayoutBoxReturnSize, PE_GetLayoutBoxReturnSize),
	    HLL_EXPORT(SetLayoutBoxAlign, PE_SetLayoutBoxAlign),
	    HLL_EXPORT(GetLayoutBoxAlign, PE_GetLayoutBoxAlign),
	    HLL_EXPORT(Parts_SetPartsCG, PE_SetPartsCG),
	    HLL_EXPORT(Parts_GetPartsCGName, PE_GetPartsCGName),
	    HLL_EXPORT(Parts_SetPartsCGSurfaceArea, PE_SetPartsCGSurfaceArea),
	    HLL_EXPORT(Parts_SetLoopCG, PE_SetLoopCG),
	    HLL_EXPORT(Parts_SetLoopCGSurfaceArea, PE_SetLoopCGSurfaceArea),
	    HLL_EXPORT(Parts_SetText, PE_SetText),
	    HLL_EXPORT(Parts_AddPartsText, PE_AddPartsText),
	    HLL_TODO_EXPORT(Parts_DeletePartsTopTextLine, PartsEngine_Parts_DeletePartsTopTextLine),
	    HLL_EXPORT(Parts_SetPartsTextSurfaceArea, PE_SetPartsTextSurfaceArea),
	    HLL_TODO_EXPORT(Parts_SetPartsTextHighlight, PartsEngine_Parts_SetPartsTextHighlight),
	    HLL_TODO_EXPORT(Parts_AddPartsTextHighlight, PartsEngine_Parts_AddPartsTextHighlight),
	    HLL_TODO_EXPORT(Parts_ClearPartsTextHighlight, PartsEngine_Parts_ClearPartsTextHighlight),
	    HLL_TODO_EXPORT(Parts_SetPartsTextCountReturn, PartsEngine_Parts_SetPartsTextCountReturn),
	    HLL_TODO_EXPORT(Parts_GetPartsTextCountReturn, PartsEngine_Parts_GetPartsTextCountReturn),
	    HLL_EXPORT(Parts_SetFont, PE_SetFont),
	    HLL_EXPORT(Parts_SetPartsFontType, PE_SetPartsFontType),
	    HLL_EXPORT(Parts_SetPartsFontSize, PE_SetPartsFontSize),
	    HLL_EXPORT(Parts_SetPartsFontColor, PE_SetPartsFontColor),
	    HLL_EXPORT(Parts_SetPartsFontBoldWeight, PE_SetPartsFontBoldWeight),
	    HLL_EXPORT(Parts_SetPartsFontEdgeColor, PE_SetPartsFontEdgeColor),
	    HLL_EXPORT(Parts_SetPartsFontEdgeWeight, PE_SetPartsFontEdgeWeight),
	    HLL_EXPORT(Parts_SetTextCharSpace, PE_SetTextCharSpace),
	    HLL_EXPORT(Parts_SetTextLineSpace, PE_SetTextLineSpace),
	    HLL_EXPORT(Parts_SetHGaugeCG, PE_SetHGaugeCG),
	    HLL_EXPORT(Parts_SetHGaugeRate, PE_SetHGaugeRate),
	    HLL_EXPORT(Parts_SetVGaugeCG, PE_SetVGaugeCG),
	    HLL_EXPORT(Parts_SetVGaugeRate, PE_SetVGaugeRate),
	    HLL_EXPORT(Parts_SetHGaugeSurfaceArea, PE_SetHGaugeSurfaceArea),
	    HLL_EXPORT(Parts_SetVGaugeSurfaceArea, PE_SetVGaugeSurfaceArea),
	    HLL_EXPORT(Parts_SetNumeralCG, PE_SetNumeralCG),
	    HLL_EXPORT(Parts_SetNumeralLinkedCGNumberWidthWidthList, PE_SetNumeralLinkedCGNumberWidthWidthList),
	    HLL_TODO_EXPORT(Parts_SetNumeralFont, PartsEngine_Parts_SetNumeralFont),
	    HLL_EXPORT(Parts_SetNumeralNumber, PE_SetNumeralNumber),
	    HLL_EXPORT(Parts_SetNumeralShowComma, PE_SetNumeralShowComma),
	    HLL_EXPORT(Parts_SetNumeralSpace, PE_SetNumeralSpace),
	    HLL_EXPORT(Parts_SetNumeralLength, PE_SetNumeralLength),
	    HLL_EXPORT(Parts_SetNumeralSurfaceArea, PE_SetNumeralSurfaceArea),
	    HLL_EXPORT(Parts_SetPartsRectangleDetectionSize, PE_SetPartsRectangleDetectionSize),
	    HLL_TODO_EXPORT(Parts_SetPartsRectangleDetectionSurfaceArea, PartsEngine_Parts_SetPartsRectangleDetectionSurfaceArea),
	    HLL_EXPORT(Parts_SetPartsCGDetectionSize, PE_SetPartsCGDetectionSize),
	    HLL_TODO_EXPORT(Parts_SetPartsCGDetectionSurfaceArea, PartsEngine_Parts_SetPartsCGDetectionSurfaceArea),
	    HLL_EXPORT(Parts_SetPartsFlat, PE_SetPartsFlat),
	    HLL_EXPORT(Parts_IsPartsFlatEnd, PE_IsPartsFlatEnd),
	    HLL_EXPORT(Parts_GetPartsFlatCurrentFrameNumber, PE_GetPartsFlatCurrentFrameNumber),
	    HLL_EXPORT(Parts_BackPartsFlatBeginFrame, PE_BackPartsFlatBeginFrame),
	    HLL_EXPORT(Parts_StepPartsFlatFinalFrame, PE_StepPartsFlatFinalFrame),
	    HLL_EXPORT(Parts_SetPartsFlatSurfaceArea, PE_SetPartsFlatSurfaceArea),
	    HLL_EXPORT(Parts_SetPartsFlatAndStop, PE_SetPartsFlatAndStop),
	    HLL_EXPORT(Parts_StopPartsFlat, PE_StopPartsFlat),
	    HLL_EXPORT(Parts_StartPartsFlat, PE_StartPartsFlat),
	    HLL_EXPORT(Parts_GoFramePartsFlat, PE_GoFramePartsFlat),
	    HLL_EXPORT(Parts_GetPartsFlatEndFrame, PE_GetPartsFlatEndFrame),
	    HLL_EXPORT(Parts_ExistsFlatFile, PE_ExistsFlatFile),
	    HLL_TODO_EXPORT(Parts_GetPartsFlatDataInfo, PartsEngine_Parts_GetPartsFlatDataInfo),
	    HLL_TODO_EXPORT(Parts_ChangeFlatCG, PartsEngine_Parts_ChangeFlatCG),
	    HLL_TODO_EXPORT(Parts_ChangeFlatSound, PartsEngine_Parts_ChangeFlatSound),
	    HLL_EXPORT(Parts_ClearPartsConstructionProcess, PE_ClearPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddCreateToPartsConstructionProcess, PE_AddCreateToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddCreatePixelOnlyToPartsConstructionProcess, PE_AddCreatePixelOnlyToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddCreateCGToProcess, PE_AddCreateCGToProcess),
	    HLL_EXPORT(Parts_AddFillToPartsConstructionProcess, PE_AddFillToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddFillAlphaColorToPartsConstructionProcess, PE_AddFillAlphaColorToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddFillAMapToPartsConstructionProcess, PE_AddFillAMapToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddFillWithAlphaToPartsConstructionProcess, PE_AddFillWithAlphaToPartsConstructionProcess),
	    HLL_TODO_EXPORT(Parts_AddFillGradationHorizonToPartsConstructionProcess, PartsEngine_Parts_AddFillGradationHorizonToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddDrawRectToPartsConstructionProcess, PE_AddDrawRectToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddDrawCutCGToPartsConstructionProcess, PE_AddDrawCutCGToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddCopyCutCGToPartsConstructionProcess, PE_AddCopyCutCGToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddGrayFilterToPartsConstructionProcess, PE_AddGrayFilterToPartsConstructionProcess),
	    HLL_TODO_EXPORT(Parts_AddAddFilterToPartsConstructionProcess, PartsEngine_Parts_AddAddFilterToPartsConstructionProcess),
	    HLL_TODO_EXPORT(Parts_AddMulFilterToPartsConstructionProcess, PartsEngine_Parts_AddMulFilterToPartsConstructionProcess),
	    HLL_TODO_EXPORT(Parts_AddDrawLineToPartsConstructionProcess, PartsEngine_Parts_AddDrawLineToPartsConstructionProcess),
	    HLL_EXPORT(Parts_BuildPartsConstructionProcess, PE_BuildPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddDrawTextToPartsConstructionProcess, PE_AddDrawTextToPartsConstructionProcess),
	    HLL_EXPORT(Parts_AddCopyTextToPartsConstructionProcess, PE_AddCopyTextToPartsConstructionProcess),
	    HLL_EXPORT(Parts_SetPartsConstructionSurfaceArea, PE_SetPartsConstructionSurfaceArea),
	    HLL_EXPORT(Parts_CreateParts3DLayerPluginID, PE_CreateParts3DLayerPluginID),
	    HLL_EXPORT(Parts_GetParts3DLayerPluginID, PE_GetParts3DLayerPluginID),
	    HLL_EXPORT(Parts_ReleaseParts3DLayerPluginID, PE_ReleaseParts3DLayerPluginID),
	    HLL_EXPORT(Parts_SetPassCursor, PE_SetPassCursor),
	    HLL_EXPORT(Parts_SetClickable, PE_SetClickable),
	    HLL_TODO_EXPORT(Parts_SetResetTimerByChangeInputStatus, PartsEngine_Parts_SetResetTimerByChangeInputStatus),
	    HLL_EXPORT(Parts_SetDrag, PE_SetDrag),
	    HLL_EXPORT(Parts_SetParentPartsNumber, PE_SetParentPartsNumber),
	    HLL_EXPORT(Parts_SetInputState, PE_SetInputState),
	    HLL_EXPORT(Parts_SetOnCursorShowLinkPartsNumber, PE_SetOnCursorShowLinkPartsNumber),
	    HLL_TODO_EXPORT(Parts_SetSoundNumber, PartsEngine_Parts_SetSoundNumber),
	    HLL_EXPORT(Parts_SetPartsPixelDecide, PE_SetPartsPixelDecide),
	    HLL_EXPORT(Parts_GetPartsPassCursor, PE_GetPartsPassCursor),
	    HLL_EXPORT(Parts_GetPartsClickable, PE_GetPartsClickable),
	    HLL_TODO_EXPORT(Parts_GetResetTimerByChangeInputStatus, PartsEngine_Parts_GetResetTimerByChangeInputStatus),
	    HLL_TODO_EXPORT(Parts_GetPartsDrag, PartsEngine_Parts_GetPartsDrag),
	    HLL_TODO_EXPORT(Parts_GetParentPartsNumber, PartsEngine_Parts_GetParentPartsNumber),
	    HLL_EXPORT(Parts_GetInputState, PE_GetInputState),
	    HLL_TODO_EXPORT(Parts_GetOnCursorShowLinkPartsNumber, PartsEngine_Parts_GetOnCursorShowLinkPartsNumber),
	    HLL_TODO_EXPORT(Parts_GetSoundNumber, PartsEngine_Parts_GetSoundNumber),
	    HLL_TODO_EXPORT(Parts_IsPartsPixelDecide, PartsEngine_Parts_IsPartsPixelDecide),
	    HLL_EXPORT(Parts_IsCursorIn, PE_IsCursorIn),
	    HLL_TODO_EXPORT(SaveParts, PartsEngine_SaveParts),
	    HLL_TODO_EXPORT(LoadParts, PartsEngine_LoadParts)
	    );

static struct ain_hll_function *get_fun(int libno, const char *name)
{
	int fno = ain_get_library_function(ain, libno, name);
	return fno >= 0 ? &ain->libraries[libno].functions[fno] : NULL;
}

static void PartsEngine_PreLink(void)
{
	struct ain_hll_function *fun;
	int libno = ain_get_library(ain, "PartsEngine");
	assert(libno >= 0);

	fun = get_fun(libno, "AddDrawCutCGToPartsConstructionProcess");
	if (fun && fun->nr_arguments == 12) {
		static_library_replace(&lib_PartsEngine, "AddDrawCutCGToPartsConstructionProcess",
				PE_AddDrawCutCGToPartsConstructionProcess);
	}
	fun = get_fun(libno, "AddCopyCutCGToPartsConstructionProcess");
	if (fun && fun->nr_arguments == 12) {
		static_library_replace(&lib_PartsEngine, "AddCopyCutCGToPartsConstructionProcess",
				PE_AddCopyCutCGToPartsConstructionProcess);
	}
	fun = get_fun(libno, "Update");
	if (fun && fun->nr_arguments == 5) {
		static_library_replace(&lib_PartsEngine, "Update",
				PartsEngine_Update_Pascha3PC);
	}
	if (get_fun(libno, "AddController")) {
		PE_enable_multi_controller();
	}
	parts_func_uses_rance9_ids = ain_get_function(ain, "PARTS_GetLayerIndex") < 0;
}
