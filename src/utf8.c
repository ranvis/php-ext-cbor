/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"

/*
Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static const uint8_t utf8d[] = {
	/* The first part of the table maps bytes to character classes that
	   to reduce the size of the transition table and create bitmasks. */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	 8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

	/* The second part is a transition table that maps a combination
	   of a state of the automaton and a character class to a state. */
	 0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
	12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
	12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
	12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
	12,36,12,12,12,12,12,12,12,12,12,12,
};

bool cbor_is_utf8(const uint8_t *str, size_t len)
{
	uint32_t state = 0;
	const uint8_t *end = str + len;
	while (str < end && *str < 0x80) {
		str++;
	}
	while (str < end) {
		uint8_t type = utf8d[*str++];
		state = utf8d[256 + state + type];
#if 0
		if (UNEXPECTED(state == UTF8_REJECT)) {
			return false;
		}
#endif
	}
	return state == UTF8_ACCEPT;
}

uint32_t cbor_next_utf8(const uint8_t **str, const uint8_t *end)
{
	uint32_t state = 0;
	uint32_t codepoint = 0;
	const uint8_t *ptr = *str;
	if (ptr < end && (codepoint = *ptr) < 0x80) {
		*str = ptr + 1;
		return codepoint;
	}
	while (ptr < end) {
		uint8_t byte = *ptr++;
		uint8_t type = utf8d[byte];
		codepoint = (state != UTF8_ACCEPT)
			? (byte & 0x3f) | (codepoint << 6)
			: (0xff >> type) & byte;
		state = utf8d[256 + state + type];
		if (UNEXPECTED(state == UTF8_REJECT)) {
			break;
		}
		if (state == UTF8_ACCEPT) {
			*str = ptr;
			return codepoint;
		}
	}
	return 0xffffffff;
}
