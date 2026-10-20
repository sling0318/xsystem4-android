/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
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
#include <stddef.h>
#include <stdio.h>

#include "system4.h"
#include "system4/cg.h"

#include "asset_manager.h"
#include "cJSON.h"
#include "gfx/gfx.h"
#include "json.h"
#include "queue.h"
#include "scene.h"
#include "xsystem4.h"

static int id_counter = 0;

bool scene_is_dirty = true;

static TAILQ_HEAD(listhead, sprite) sprite_list = TAILQ_HEAD_INITIALIZER(sprite_list);

static Texture wp = {0};
static bool wallpaper_hidden;
static struct sprite **blackout_sprites;
static int blackout_count;
static struct sprite blackout_sprite;
static struct texture blackout_tex;
static bool blackout_sprite_registered;

static void blackout_sprite_render(struct sprite *sp)
{
	(void)sp;
	Rectangle r = RECT(0, 0, config.view_width, config.view_height);
	gfx_render_texture(&blackout_tex, &r);
}

void scene_blackout_begin(void)
{
	scene_blackout_end();
	int n = 0;
	struct sprite *sp;
	TAILQ_FOREACH(sp, &sprite_list, entry)
		n++;
	blackout_sprites = xmalloc(n * sizeof(*blackout_sprites));
	TAILQ_FOREACH(sp, &sprite_list, entry)
		blackout_sprites[blackout_count++] = sp;

	// The save/load UI is drawn above the game scene by the same renderer,
	// but some game content (background, battle cards) is not part of the
	// scene sprite list. A full-screen black sprite at z=999 covers everything
	// below the save UI parts (group z=1000), including stale framebuffer
	// content, without touching any save state.
	if (!blackout_tex.handle)
		gfx_init_texture_rgb(&blackout_tex, 1, 1, (SDL_Color){0, 0, 0, 255});
	blackout_sprite.z = 999;
	blackout_sprite.has_pixel = true;
	blackout_sprite.render = blackout_sprite_render;
	scene_register_sprite(&blackout_sprite);
	blackout_sprite_registered = true;
	scene_dirty();
}

void scene_blackout_end(void)
{
	if (blackout_sprite_registered) {
		scene_unregister_sprite(&blackout_sprite);
		blackout_sprite_registered = false;
	}
	free(blackout_sprites);
	blackout_sprites = NULL;
	blackout_count = 0;
	scene_dirty();
}

bool scene_blackout_skip(struct sprite *sp)
{
	for (int i = 0; i < blackout_count; i++) {
		if (blackout_sprites[i] == sp)
			return true;
	}
	return false;
}

void scene_register_sprite(struct sprite *sp)
{
	if (sp->in_scene)
		return;

	if (!sp->id)
		sp->id = ++id_counter;

	struct sprite *p;
	TAILQ_FOREACH(p, &sprite_list, entry) {
		if (p->z == sp->z) {
			if (p->z2 > sp->z2) {
				TAILQ_INSERT_BEFORE(p, sp, entry);
				sp->in_scene = true;
				return;
			}
		} else if (p->z > sp->z) {
			TAILQ_INSERT_BEFORE(p, sp, entry);
			sp->in_scene = true;
			return;
		}
	}
	TAILQ_INSERT_TAIL(&sprite_list, sp, entry);
	sp->in_scene = true;
	scene_dirty();
}

void scene_unregister_sprite(struct sprite *sp)
{
	if (!sp->in_scene)
		return;
	TAILQ_REMOVE(&sprite_list, sp, entry);
	sp->in_scene = false;
	scene_dirty();
}

void scene_render(void)
{
	gfx_clear();
	if (wp.handle && !wallpaper_hidden) {
		Rectangle r = RECT(0, 0, wp.w, wp.h);
		gfx_render_texture(&wp, &r);
	}

	struct sprite *sp;
	TAILQ_FOREACH(sp, &sprite_list, entry) {
		if (scene_blackout_skip(sp))
			continue;
		if (sp->render)
			sp->render(sp);
		else
			WARNING("sprite in scene without render function");
	}
}

int scene_set_wp(int cg_no) {
	if (!cg_no) {
		gfx_delete_texture(&wp);
		scene_dirty();
		return 1;
	}

	struct cg *cg = asset_cg_load(cg_no);
	if (!cg)
		return 0;

	gfx_delete_texture(&wp);
	gfx_init_texture_with_cg(&wp, cg);
	cg_free(cg);
	scene_dirty();
	return 1;
}

int scene_set_wp_color(int r, int g, int b)
{
	gfx_set_clear_color(r, g, b, 255);
	scene_dirty();
	return 1;
}

void scene_set_wallpaper_visible(bool visible)
{
	wallpaper_hidden = !visible;
	scene_dirty();
}

int scene_set_wp_texture(Texture *tex)
{
	gfx_delete_texture(&wp);
	gfx_init_texture_rgb(&wp, tex->w, tex->h, (SDL_Color){ 0, 0, 0, 255 });
	gfx_copy(&wp, 0, 0, tex, 0, 0, tex->w, tex->h);

	scene_dirty();
	return 1;
}

void scene_set_sprite_z(struct sprite *sp, int z)
{
	sp->z = z;
	if (sp->in_scene) {
		scene_unregister_sprite(sp);
		scene_register_sprite(sp);
	}
}

void scene_set_sprite_z2(struct sprite *sp, int z, int z2)
{
	sp->z = z;
	sp->z2 = z2;
	if (sp->in_scene) {
		scene_unregister_sprite(sp);
		scene_register_sprite(sp);
	}
}

void scene_print(void)
{
	struct sprite *p;
	TAILQ_FOREACH(p, &sprite_list, entry) {
		cJSON *json = p->to_json ? p->to_json(p, false) : scene_sprite_to_json(p, false);
		char *text = cJSON_Print(json);
		sys_message("%s\n", text);
		free(text);
		cJSON_Delete(json);
	}
}

cJSON *scene_to_json(bool verbose)
{
	cJSON *ar = cJSON_CreateArray();
	struct sprite *p;
	TAILQ_FOREACH(p, &sprite_list, entry) {
		cJSON *sprite = p->to_json ? p->to_json(p, verbose) : scene_sprite_to_json(p, verbose);
		cJSON_AddItemToArray(ar, sprite);
	}
	return ar;
}

struct sprite *scene_get(int id)
{
	struct sprite *p;
	TAILQ_FOREACH(p, &sprite_list, entry) {
		if (p->id == id)
			return p;
	}
	return NULL;
}
