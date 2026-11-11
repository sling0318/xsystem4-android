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

#include <stdlib.h>
#include <string.h>
#include "cheat.h"
#include "system4/ain.h"
#include "system4/utfsjis.h"
#include "vm.h"
#include "vm/heap.h"
#include "vm/page.h"
#include "xsystem4.h"

#ifdef __ANDROID__

// Addressed variables are verified against Rance01's ain file
// (tools/ain_dump.py); see device-out/cheat_paths.md. Other games simply fail
// to resolve these paths and the overlay reports no entries.
static const struct cheat_def {
	const char *label;  // UTF-8, shown in the overlay UI
	const char *path;   // UTF-8 ain path: NAME{'.'NAME|'['INT']'}*
} cheat_defs[] = {
	{ "金币",   "g_player.gold_"    },
	{ "经验",   "g_player.exp_"     },
	{ "等级",   "g_player.level_"   },
	{ "HP",     "g_player.hp_"      },
	{ "最大HP", "g_player.maxHp_"   },
	{ "SP",     "g_player.sp_"      },
	{ "最大SP", "g_player.maxSp_"   },
	{ "攻击",   "g_player.atk_"     },
	{ "防御",   "g_player.def_"     },
	{ "幸运",   "g_player.luck_"    },
};
#define NR_CHEAT_DEFS (sizeof(cheat_defs)/sizeof(cheat_defs[0]))

struct cheat_op {
	bool subscript;  // false: struct member, true: array subscript
	int index;
};

struct cheat_entry {
	int global_index;    // -1 if unresolved
	struct cheat_op *ops;
	int nr_ops;
};

static struct cheat_entry entries[NR_CHEAT_DEFS];
static int nr_resolved = -1;  // -1: not initialized yet

static bool cheat_scalar(enum ain_data_type type)
{
	switch (type) {
	case AIN_INT:
	case AIN_BOOL:
	case AIN_LONG_INT:
	case AIN_FLOAT:
		return true;
	default:
		return false;
	}
}

// Look up a struct member by (SJIS) name.
static int cheat_member_index(int struct_no, const char *name)
{
	if (struct_no < 0 || struct_no >= ain->nr_structures)
		return -1;
	struct ain_struct *struc = &ain->structures[struct_no];
	for (int i = 0; i < struc->nr_members; i++) {
		if (!strcmp(struc->members[i].name, name))
			return i;
	}
	return -1;
}

static bool cheat_resolve_entry(const struct cheat_def *def, struct cheat_entry *e)
{
	e->global_index = -1;
	e->ops = NULL;
	e->nr_ops = 0;
	if (!ain)
		return false;

	// global variable name: up to the first '.' or '['
	const char *p = def->path;
	const char *end = p;
	while (*end && *end != '.' && *end != '[')
		end++;
	if (end == p)
		return false;
	char name[256];
	size_t len = end - p;
	if (len >= sizeof(name))
		return false;
	memcpy(name, p, len);
	name[len] = '\0';
	char *sjis_name = utf2sjis(name, 0);
	e->global_index = ain_get_global(ain, sjis_name);
	free(sjis_name);
	if (e->global_index < 0 || e->global_index >= ain->nr_globals) {
		WARNING("cheat: global '%s' not found in %d globals (first: %s)",
				name, ain->nr_globals,
				ain->nr_globals > 0 ? ain->globals[0].name : "(none)");
		return false;
	}

	struct ain_type type = ain->globals[e->global_index].type;
	int ops_cap = 0;
	while (*p) {
		if (*p == '.') {
			p++;
			const char *ms = p;
			while (*p && *p != '.' && *p != '[')
				p++;
			size_t mlen = p - ms;
			if (!mlen || mlen >= sizeof(name))
				return false;
			memcpy(name, ms, mlen);
			name[mlen] = '\0';
			if (type.data != AIN_STRUCT || type.rank != 0) {
				WARNING("cheat: '%s' member %s: not a struct (type %d)",
						def->path, name, type.data);
				return false;
			}
			char *sjis_member = utf2sjis(name, 0);
			int mi = cheat_member_index(type.struc, sjis_member);
			free(sjis_member);
			if (mi < 0) {
				WARNING("cheat: member '%s' not found in struct %d", name, type.struc);
				return false;
			}
			type = ain->structures[type.struc].members[mi].type;
			if (e->nr_ops >= ops_cap) {
				ops_cap = ops_cap ? ops_cap * 2 : 4;
				e->ops = xrealloc(e->ops, ops_cap * sizeof(struct cheat_op));
			}
			e->ops[e->nr_ops++] = (struct cheat_op){ .subscript = false, .index = mi };
		} else if (*p == '[') {
			p++;
			char *endp;
			long idx = strtol(p, &endp, 10);
			if (endp == p || *endp != ']' || idx < 0)
				return false;
			p = endp + 1;
			switch (type.data) {
			case AIN_ARRAY_INT:
			case AIN_ARRAY_FLOAT:
			case AIN_ARRAY_STRING:
			case AIN_ARRAY_STRUCT:
			case AIN_ARRAY_BOOL:
			case AIN_ARRAY_LONG_INT:
			case AIN_ARRAY:
				break;
			default:
				return false;
			}
			if (e->nr_ops >= ops_cap) {
				ops_cap = ops_cap ? ops_cap * 2 : 4;
				e->ops = xrealloc(e->ops, ops_cap * sizeof(struct cheat_op));
			}
			e->ops[e->nr_ops++] = (struct cheat_op){ .subscript = true, .index = (int)idx };
		} else {
			return false;
		}
	}
	if (!cheat_scalar(type.data)) {
		free(e->ops);
		e->ops = NULL;
		e->nr_ops = 0;
		return false;
	}
	return true;
}

void cheat_init(void)
{
	// Retry until the game program is loaded: callers may fire before the
	// VM thread has run vm_execute_ain.
	if (nr_resolved >= 0 || !ain)
		return;
	NOTICE("cheat: resolving %d paths (ain has %d globals)",
			(int)NR_CHEAT_DEFS, ain->nr_globals);
	nr_resolved = 0;
	for (size_t i = 0; i < NR_CHEAT_DEFS; i++) {
		if (cheat_resolve_entry(&cheat_defs[i], &entries[i])) {
			nr_resolved++;
		} else {
			WARNING("cheat: path unresolved: %s (%s)", cheat_defs[i].path, cheat_defs[i].label);
		}
	}
	NOTICE("cheat: %d/%d resolved", nr_resolved, (int)NR_CHEAT_DEFS);
}

int cheat_entry_count(void)
{
	// With a partial match (some paths unresolved) still expose every
	// entry: unresolved ones fail get/set and the UI shows them as
	// unavailable.
	if (nr_resolved <= 0)
		return 0;
	return (int)NR_CHEAT_DEFS;
}

const char *cheat_entry_label(int i)
{
	if (i < 0 || (size_t)i >= NR_CHEAT_DEFS)
		return NULL;
	return cheat_defs[i].label;
}

// Locate the addressed variable. Must be called with the VM state lock held.
static bool cheat_locate(const struct cheat_entry *e, struct page **page_out,
		int *var_out, enum ain_data_type *type_out)
{
	if (!ain || !heap || heap_size == 0)
		return false;
	if (e->global_index < 0 || e->global_index >= ain->nr_globals)
		return false;
	// The global page lives in heap slot 0. ref==0 means the VM hasn't
	// initialized it yet or is mid-teardown (load/reset).
	if (heap[0].ref <= 0 || !heap[0].page || heap[0].page->type != GLOBAL_PAGE)
		return false;
	if (e->global_index >= heap[0].page->nr_vars)
		return false;

	struct page *page = heap[0].page;
	int var = e->global_index;
	struct ain_type type = ain->globals[var].type;

	for (int i = 0; i < e->nr_ops; i++) {
		const struct cheat_op *op = &e->ops[i];
		union vm_value ptr = page->values[var];
		if (!page_index_valid(ptr.i))
			return false;
		struct page *next = heap[ptr.i].page;
		if (!next)
			return false;
		if (op->subscript) {
			if (next->type != ARRAY_PAGE || op->index >= next->nr_vars)
				return false;
			int struct_no, rank;
			type.data = variable_type(next, op->index, &struct_no, &rank);
			type.struc = struct_no;
			type.rank = rank;
		} else {
			if (next->type != STRUCT_PAGE || next->index != type.struc)
				return false;
			if (op->index >= next->nr_vars)
				return false;
			type = ain->structures[next->index].members[op->index].type;
		}
		page = next;
		var = op->index;
	}
	if (!cheat_scalar(type.data))
		return false;
	*page_out = page;
	*var_out = var;
	*type_out = type.data;
	return true;
}

bool cheat_entry_get(int i, int32_t *value_out)
{
	if (i < 0 || (size_t)i >= NR_CHEAT_DEFS || entries[i].global_index < 0)
		return false;
	vm_state_lock();
	struct page *page;
	int var;
	enum ain_data_type type;
	bool ok = cheat_locate(&entries[i], &page, &var, &type);
	if (ok) {
		if (type == AIN_FLOAT)
			*value_out = (int32_t)page->values[var].f;
		else
			*value_out = page->values[var].i;
	}
	vm_state_unlock();
	return ok;
}

bool cheat_entry_set(int i, int32_t value)
{
	if (i < 0 || (size_t)i >= NR_CHEAT_DEFS || entries[i].global_index < 0)
		return false;
	vm_state_lock();
	struct page *page;
	int var;
	enum ain_data_type type;
	bool ok = cheat_locate(&entries[i], &page, &var, &type);
	if (ok) {
		// Scalar write: no reference counting involved.
		if (type == AIN_FLOAT)
			page->values[var].f = (float)value;
		else
			page->values[var].i = value;
	}
	vm_state_unlock();
	return ok;
}

#else /* !__ANDROID__ */

void cheat_init(void) {}
int cheat_entry_count(void) { return 0; }
const char *cheat_entry_label(int i) { return NULL; }
bool cheat_entry_get(int i, int32_t *value_out) { return false; }
bool cheat_entry_set(int i, int32_t value) { return false; }

#endif
