/* Copyright (C) 2024 sling0318
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

#ifndef XSYSTEM4_CHEAT_H
#define XSYSTEM4_CHEAT_H

#include <stdbool.h>
#include <stdint.h>

// Built-in cheat table for the Android overlay UI. Each entry addresses a
// global variable (or a member/subscript path into one) which is read/written
// live while the game runs.
//
// Resolve the entry paths against the loaded .ain program. Safe to call
// multiple times; does nothing until the program is loaded and is a no-op on
// non-Android builds.
void cheat_init(void);

int cheat_entry_count(void);
const char *cheat_entry_label(int i);

// Read the current value of entry i. Returns false if the game isn't running
// or the addressed value can't be located (e.g. mid-load).
bool cheat_entry_get(int i, int32_t *value_out);

// Write entry i. Only scalar (int/bool/long int/float) destinations are
// accepted. Returns false on failure; the game state is left untouched.
bool cheat_entry_set(int i, int32_t value);

#endif /* XSYSTEM4_CHEAT_H */
