/* Copyright (C) 2026 kichikuou <KichikuouChrome@gmail.com>
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

#include <stdbool.h>
#include <stdlib.h>

#include "system4/flat.h"

#include "parts.h"
#include "parts_internal.h"

void PE_SetComponentMargin(int parts_no, int top, int bottom, int left, int right)
{
	struct parts *parts = parts_get(parts_no);
	parts->margin_top = top;
	parts->margin_bottom = bottom;
	parts->margin_left = left;
	parts->margin_right = right;
}

int PE_GetComponentMarginTop(int parts_no)
{
	return parts_get(parts_no)->margin_top;
}

int PE_GetComponentMarginBottom(int parts_no)
{
	return parts_get(parts_no)->margin_bottom;
}

int PE_GetComponentMarginLeft(int parts_no)
{
	return parts_get(parts_no)->margin_left;
}

int PE_GetComponentMarginRight(int parts_no)
{
	return parts_get(parts_no)->margin_right;
}

void PE_SetLayoutBoxLayoutType(int parts_no, int type)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_layout_box *lb = parts_get_layout_box(parts);
	if (lb->layout_type != type) {
		lb->layout_type = type;
		parts_component_dirty(parts);
	}
}

int PE_GetLayoutBoxLayoutType(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return PARTS_LAYOUT_FREE;
	return parts->states[0].layout_box.layout_type;
}

void PE_SetLayoutBoxReturn(int parts_no, bool return_flag, int return_size)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_layout_box *lb = parts_get_layout_box(parts);
	lb->wrap = return_flag;
	lb->wrap_size = return_size;
	parts_component_dirty(parts);
}

bool PE_IsLayoutBoxReturn(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return false;
	return parts->states[0].layout_box.wrap;
}

int PE_GetLayoutBoxReturnSize(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return 0;
	return parts->states[0].layout_box.wrap_size;
}

void PE_SetLayoutBoxAlign(int parts_no, int align)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_layout_box *lb = parts_get_layout_box(parts);
	if (lb->align != align) {
		lb->align = align;
		parts_component_dirty(parts);
	}
}

int PE_GetLayoutBoxAlign(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return 0;
	return parts->states[0].layout_box.align;
}

void PE_set_layoutbox_padding(int parts_no, int top, int bottom, int left, int right)
{
	struct parts *parts = parts_get(parts_no);
	struct parts_layout_box *lb = parts_get_layout_box(parts);
	lb->padding_top = top;
	lb->padding_bottom = bottom;
	lb->padding_left = left;
	lb->padding_right = right;
	parts_component_dirty(parts);
}

int PE_get_layoutbox_padding_top(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return 0;
	return parts->states[0].layout_box.padding_top;
}

int PE_get_layoutbox_padding_bottom(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return 0;
	return parts->states[0].layout_box.padding_bottom;
}

int PE_get_layoutbox_padding_left(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return 0;
	return parts->states[0].layout_box.padding_left;
}

int PE_get_layoutbox_padding_right(int parts_no)
{
	struct parts *parts = parts_try_get(parts_no);
	if (!parts || parts->states[0].type != PARTS_LAYOUT_BOX)
		return 0;
	return parts->states[0].layout_box.padding_right;
}

static void parts_get_layout_size(struct parts *parts, int *w, int *h)
{
	struct parts_state *state = &parts->states[parts->state];
	if (state->type == PARTS_FLAT && state->flat.flat) {
		// The layout size of Flat is specified in the header, and this may
		// differ from the hitbox size.
		*w = state->flat.flat->hdr.width;
		*h = state->flat.flat->hdr.height;
	} else {
		*w = state->common.w;
		*h = state->common.h;
	}
}

static int align_offset_x(int align, int total_w)
{
	switch (align) {
	case 2: case 5: case 8: return -(total_w / 2);
	case 3: case 6: case 9: return -total_w;
	default: return 0;
	}
}

static int align_offset_y(int align, int total_h)
{
	switch (align) {
	case 4: case 5: case 6: return -(total_h / 2);
	case 7: case 8: case 9: return -total_h;
	default: return 0;
	}
}

// Calculates the total size of a horizontal layout box by summing child widths per row.
static void parts_layout_calc_size_horizontal(struct parts_layout_box *lb, struct parts *parts,
		int *out_w, int *out_h)
{
	int row_w = 0, max_row_w = 0, max_row_h = 0, total_h = 0;
	struct parts *child;
	PARTS_FOREACH_CHILD(child, parts) {
		int child_w, child_h;
		parts_get_layout_size(child, &child_w, &child_h);
		int cw = child->margin_left + child_w + child->margin_right;
		int ch = child->margin_top + child_h + child->margin_bottom;
		if (lb->wrap && row_w + cw > lb->wrap_size && row_w > 0) {
			if (row_w > max_row_w)
				max_row_w = row_w;
			total_h += max_row_h;
			row_w = 0;
			max_row_h = 0;
		}
		row_w += cw;
		if (ch > max_row_h)
			max_row_h = ch;
	}
	if (row_w > max_row_w)
		max_row_w = row_w;
	total_h += max_row_h;
	*out_w = max_row_w + lb->padding_left + lb->padding_right;
	*out_h = total_h + lb->padding_top + lb->padding_bottom;
}

// Calculates the total size of a vertical layout box by summing child heights per column.
static void parts_layout_calc_size_vertical(struct parts_layout_box *lb, struct parts *parts,
		int *out_w, int *out_h)
{
	int col_h = 0, max_col_h = 0, max_col_w = 0, total_w = 0;
	struct parts *child;
	PARTS_FOREACH_CHILD(child, parts) {
		int child_w, child_h;
		parts_get_layout_size(child, &child_w, &child_h);
		int cw = child->margin_left + child_w + child->margin_right;
		int ch = child->margin_top + child_h + child->margin_bottom;
		if (lb->wrap && col_h + ch > lb->wrap_size && col_h > 0) {
			if (col_h > max_col_h)
				max_col_h = col_h;
			total_w += max_col_w;
			col_h = 0;
			max_col_w = 0;
		}
		col_h += ch;
		if (cw > max_col_w)
			max_col_w = cw;
	}
	if (col_h > max_col_h)
		max_col_h = col_h;
	total_w += max_col_w;
	*out_w = total_w + lb->padding_left + lb->padding_right;
	*out_h = max_col_h + lb->padding_top + lb->padding_bottom;
}

/*
 * Content bounds of `parts`, expressed in the coordinate system of its own
 * anchor point (i.e. before its own origin-mode offset is applied).
 *
 * A container carries no graphic and the .pactex has no サイズ field for it, so
 * its size has to be derived from what its children occupy. That size is what
 * lets 原点座標モード move the whole group: parts_child_origin() offsets every
 * child by calculate_offset(origin_mode, w, h).
 *
 * Must be recursive rather than reading the children's stored w/h, because
 * parts_update_component() lays a container out before its children, so a
 * nested container's size is still stale at that point.
 */
static bool parts_content_bbox(struct parts *parts, Rectangle *out)
{
	if (parts->scroll.width > 0 && parts->scroll.height > 0) {
		*out = (Rectangle) { 0, 0, parts->scroll.width, parts->scroll.height };
		return true;
	}

	bool any = false;
	Rectangle acc = { 0, 0, 0, 0 };
	struct parts *child;
	PARTS_FOREACH_CHILD(child, parts) {
		Rectangle cb;
		if (!parts_content_bbox(child, &cb))
			continue;
		int x0 = cb.x + child->local.pos.x;
		int y0 = cb.y + child->local.pos.y;
		int x1 = x0 + cb.w;
		int y1 = y0 + cb.h;
		if (!any) {
			acc = (Rectangle) { x0, y0, cb.w, cb.h };
			any = true;
		} else {
			int ax0 = acc.x < x0 ? acc.x : x0;
			int ay0 = acc.y < y0 ? acc.y : y0;
			int ax1 = acc.x + acc.w > x1 ? acc.x + acc.w : x1;
			int ay1 = acc.y + acc.h > y1 ? acc.y + acc.h : y1;
			acc = (Rectangle) { ax0, ay0, ax1 - ax0, ay1 - ay0 };
		}
	}

	if (any) {
		// The children hang off this node's child-origin, which carries its own
		// origin-mode offset; report the bounds as seen by our parent, i.e.
		// after that shift.
		Point off = calculate_offset(parts->origin_mode, acc.w, acc.h);
		acc.x += off.x;
		acc.y += off.y;
		*out = acc;
		return true;
	}

	struct parts_state *state = &parts->states[0];
	if (state->common.w == 0 && state->common.h == 0)
		return false;
	*out = (Rectangle) {
		state->common.origin_offset.x, state->common.origin_offset.y,
		state->common.w, state->common.h
	};
	return true;
}

void parts_do_layout(struct parts *parts)
{
	if (parts->states[0].type != PARTS_LAYOUT_BOX)
		return;
	struct parts_layout_box *lb = &parts->states[0].layout_box;
	if (lb->layout_type == PARTS_LAYOUT_FREE) {
		// Free layout leaves the children where the .pactex put them and never
		// gives the box a size; derive one from the children so that
		// origin_mode can still offset the whole group. Storing it also
		// refreshes lb->common.origin_offset, which parts_child_origin() and
		// input.c read.
		Rectangle bbox;
		if (parts_content_bbox(parts, &bbox))
			parts_set_dims(parts, &lb->common, bbox.w, bbox.h);
		return;
	}

	int total_w, total_h;
	if (lb->layout_type == PARTS_LAYOUT_HORIZONTAL)
		parts_layout_calc_size_horizontal(lb, parts, &total_w, &total_h);
	else
		parts_layout_calc_size_vertical(lb, parts, &total_w, &total_h);

	int align = lb->align;
	bool is_left   = (align == 1 || align == 4 || align == 7);
	bool is_right  = (align == 3 || align == 6 || align == 9);
	bool is_top    = (align == 1 || align == 2 || align == 3);
	bool is_bottom = (align == 7 || align == 8 || align == 9);

	int origin_x = align_offset_x(parts->origin_mode, total_w);
	int origin_y = align_offset_y(parts->origin_mode, total_h);
	int align_x = align_offset_x(align, total_w);
	int align_y = align_offset_y(align, total_h);

	int start_x = (is_left ? lb->padding_left : -lb->padding_right)
		+ (origin_x - align_x);
	int start_y = (is_top ? lb->padding_top : -lb->padding_bottom)
		+ (origin_y - align_y);

	int x_dir = is_right ? -1 : 1;
	int y_dir = is_bottom ? -1 : 1;

	int cur_x = start_x;
	int cur_y = start_y;
	int max_cross = 0;

	struct parts *child;
	PARTS_FOREACH_CHILD(child, parts) {
		int child_w, child_h;
		parts_get_layout_size(child, &child_w, &child_h);
		int cw = child->margin_left + child_w + child->margin_right;
		int ch = child->margin_top + child_h + child->margin_bottom;

		// The layout box forces the child's origin_mode to match its own align.
		if (child->origin_mode != align) {
			child->origin_mode = align;
			parts_recalculate_hitbox(child);
		}

		if (lb->layout_type == PARTS_LAYOUT_HORIZONTAL) {
			if (lb->wrap && abs((cur_x + cw * x_dir) - start_x) > lb->wrap_size
					&& max_cross > 0) {
				cur_y += max_cross * y_dir;
				max_cross = 0;
				cur_x = start_x;
			}
		} else {
			if (lb->wrap && abs((cur_y + ch * y_dir) - start_y) > lb->wrap_size
					&& max_cross > 0) {
				cur_x += max_cross * x_dir;
				max_cross = 0;
				cur_y = start_y;
			}
		}

		// margin offset depends on alignment direction
		int mx = is_left ? child->margin_left : is_right ? -child->margin_right : 0;
		int my = is_top ? child->margin_top : is_bottom ? -child->margin_bottom : 0;

		int pos_x = cur_x + mx;
		int pos_y = cur_y + my;
		if (child->local.pos.x != pos_x || child->local.pos.y != pos_y) {
			child->local.pos.x = pos_x;
			child->local.pos.y = pos_y;
			parts_recalculate_hitbox(child);
			parts_component_dirty(child);
		}

		if (lb->layout_type == PARTS_LAYOUT_HORIZONTAL) {
			cur_x += cw * x_dir;
			if (ch > max_cross)
				max_cross = ch;
		} else {
			cur_y += ch * y_dir;
			if (cw > max_cross)
				max_cross = cw;
		}
	}
}
