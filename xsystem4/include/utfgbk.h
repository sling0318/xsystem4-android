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

/* Charset-aware variants of the SJIS helpers; consult config.gbk_text. */
bool text_2byte(uint8_t b);
char *text_skip_char(const char *str);

#endif
