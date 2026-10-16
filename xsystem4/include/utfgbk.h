/* utfgbk.h -- GBK text support for Chinese-patched scripts
 *
 * Copyright (C) 2026 Rance01 Android port
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XSYS4_UTFGBK_H
#define XSYS4_UTFGBK_H

#include <stdbool.h>
#include <stdint.h>

#include "system4/utfsjis.h"

/* GBK lead byte range (0x81-0xFE); wider than SJIS leads. */
#define GBK_2BYTE(b) ((uint8_t)(b) >= 0x81 && (uint8_t)(b) <= 0xfe)

/* Decode the first character of src (GBK or ASCII) to a Unicode codepoint. */
char *gbk_char2unicode(const char *src, int *dst);

/* Unicode codepoint for a GBK double-byte pair, or 0xFFFD if unassigned. */
int gbk_pair_code(uint8_t lead, uint8_t trail);

/* Charset-aware variants of the SJIS helpers; consult config.gbk_text. */
bool text_2byte(uint8_t b);
char *text_skip_char(const char *str);

/* Byte offset of character `index` in a charset-aware string, or -1. */
struct string;
int text_index(const char *src, int index);
struct string *text_string_copy(const struct string *s, int index, int len);

/* Character count, GBK pairs counting as one. Mirrors sjis_count_char(). */
int text_count_char(const char *src);

/* string_get_char() with charset-aware indexing. */
int text_get_char(const struct string *str, int i);

#endif
