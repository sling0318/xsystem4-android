/* Copyright (C) 2024 Nunuhara Cabbage <nunuhara@haniwa.technology>
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

#include <math.h>

#include "gfx/font.h"
#include "sact.h"
#include "sprite.h"
#include "vm/page.h"
#include "xsystem4.h"
#include "utfgbk.h"
#include "hll.h"

static Point NewFont_cursor = { 0, 0 };
static struct text_render_metrics NewFont_metrics = {0};
static struct text_render_metrics NewFont_shadow_metrics = {0};

// The script splits message text into per-char codes using SJIS rules, so a
// GBK double-byte char can arrive as two separate single-byte codes. The
// emitted bytes always reproduce the original stream in order, so reassemble
// lead/trail pairs here before decoding. Draw, shadow and measure each see
// the code sequence independently and keep their own pending byte.
static uint8_t NewFont_pending = 0;
static uint8_t NewFont_shadow_pending = 0;
static uint8_t NewFont_measure_pending = 0;

static void NewFont_render_char(struct sact_sprite *sp, const char *text, bool shadow)
{
	struct texture *t = sprite_get_texture(sp);
	struct text_render_metrics *m = shadow ? &NewFont_shadow_metrics : &NewFont_metrics;
	m->x = NewFont_cursor.x;
	m->y = NewFont_cursor.y;
	_gfx_render_text(t, text, m);
}

static void NewFont_feed(struct sact_sprite *sp, int c, bool shadow)
{
	if (!config.gbk_text) {
		char text[3] = { c & 0xff, c >> 8, 0 };
		NewFont_render_char(sp, text, shadow);
		return;
	}
	uint8_t bytes[2] = { c & 0xff, (c >> 8) & 0xff };
	int n = bytes[1] ? 2 : 1;
	uint8_t *pending = shadow ? &NewFont_shadow_pending : &NewFont_pending;
	for (int i = 0; i < n; i++) {
		if (*pending) {
			char text[3] = { *pending, bytes[i], 0 };
			*pending = 0;
			NewFont_render_char(sp, text, shadow);
		} else if (config.gbk_text && bytes[i] >= 0x81) {
			*pending = bytes[i];
		} else {
			char text[2] = { bytes[i], 0 };
			NewFont_render_char(sp, text, shadow);
		}
	}
}

// XXX: not used
//static void NewFont_AlphaComposite(int dst_sp, int dst_x, int dst_y, int src_sp, int src_x, int src_y, int w, int h)

static int NewFont_SetTextMetrics(struct page **_tm)
{
	union vm_value *tm = (*_tm)->values;
	float edge_width = max(tm[6].i, max(tm[7].i, max(tm[8].i, tm[9].i)));
	enum font_weight weight = gfx_int_to_font_weight(tm[4].i);
	struct font_size *font_size = gfx_font_get_size(tm[5].i, tm[3].i);
	NewFont_metrics = (struct text_render_metrics) {
		.color = {
			.r = tm[0].i,
			.g = tm[1].i,
			.b = tm[2].i,
			.a = 255
		},
		.weight = weight,
		.edge_width = 0,
		.scale_x = 1.f,
		.space_scale_x = 1.f,
		.font_spacing = 0.f,
		.edge_spacing = 0.f,
		.mode = edge_width < 0.1 ? RENDER_COPY : RENDER_BLENDED,
		.font_size = font_size
	};
	NewFont_shadow_metrics = (struct text_render_metrics) {
		.color = {
			.r = tm[10].i,
			.g = tm[11].i,
			.b = tm[12].i,
			.a = 255
		},
		.weight = weight,
		.edge_width = edge_width,
		.scale_x = 1.f,
		.space_scale_x = 1.f,
		.font_spacing = 0.f,
		.edge_spacing = 0.f,
		.mode = RENDER_COPY,
		.font_size = font_size
	};

	return 1;
}

static void NewFont_SetTextX(int x)
{
	NewFont_cursor.x = x;
}

static void NewFont_SetTextY(int y)
{
	NewFont_cursor.y = y;
}

static int NewFont_GetTextX(void)
{
	return NewFont_cursor.x;
}

static int NewFont_GetTextY(void)
{
	return NewFont_cursor.y;
}

static void NewFont_SetTextPos(int x, int y)
{
	NewFont_cursor.x = x;
	NewFont_cursor.y = y;
}

static void NewFont_GetTextPos(int *x, int *y)
{
	*x = NewFont_cursor.x;
	*y = NewFont_cursor.y;
}

static int NewFont_DrawChar(int sp_no, int c)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp)
		return 0;

	NewFont_feed(sp, c, false);
	return 1;
}

static int NewFont_DrawShadowChar(int sp_no, int c)
{
	struct sact_sprite *sp = sact_get_sprite(sp_no);
	if (!sp)
		return 0;

	NewFont_feed(sp, c, true);
	return 1;
}

static int NewFont_MeasureChar(int c1, int c2)
{
	struct font_size *size = NewFont_metrics.font_size;
	if (!config.gbk_text)
		return ceilf(size->font->size_char_kerning(size, c1, c2));

	// Mirror NewFont_feed's grouping for c2 without touching draw state.
	int code;
	uint8_t b0 = c2 & 0xff, b1 = (c2 >> 8) & 0xff;
	if (NewFont_measure_pending) {
		char text[3] = { NewFont_measure_pending, b0, 0 };
		int u;
		gbk_char2unicode(text, &u);
		code = u;
		NewFont_measure_pending = (b1 >= 0x81) ? b1 : 0;
	} else if (!b1 && b0 >= 0x81) {
		NewFont_measure_pending = b0;
		return 0; // advance is charged when the trail byte is measured
	} else {
		char text[3] = { b0, b1, 0 };
		int u;
		gbk_char2unicode(text, &u);
		code = u;
	}
	return ceilf(size->font->size_char_kerning(size, code, code));
}

// XXX: not used
//static void NewFont_MultiplyAlphaByOriginalImageAlpha(int dst_sp, int dst_x, int dst_y, int src_sp, int src_x, int src_y, int w, int h)

HLL_LIBRARY(NewFont,
	HLL_TODO_EXPORT(AlphaComposite, NewFont_AlphaComposite),
	HLL_EXPORT(SetTextMetrics, NewFont_SetTextMetrics),
	HLL_EXPORT(SetTextX, NewFont_SetTextX),
	HLL_EXPORT(SetTextY, NewFont_SetTextY),
	HLL_EXPORT(GetTextX, NewFont_GetTextX),
	HLL_EXPORT(GetTextY, NewFont_GetTextY),
	HLL_EXPORT(SetTextPos, NewFont_SetTextPos),
	HLL_EXPORT(GetTextPos, NewFont_GetTextPos),
	HLL_EXPORT(DrawChar, NewFont_DrawChar),
	HLL_EXPORT(DrawShadowChar, NewFont_DrawShadowChar),
	HLL_EXPORT(MeasureChar, NewFont_MeasureChar),
	HLL_TODO_EXPORT(MultiplyAlphaByOriginalImageAlpha, NewFont_MultiplyAlphaByOriginalImageAlpha)
);
