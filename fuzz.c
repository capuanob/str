/*
BSD 3-Clause License

Copyright (c) 2020,2021, Maxim Konakov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define _POSIX_C_SOURCE 200809L

#include "str.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>

#define MIN_INPUT_SIZE 1000
#define passed (void)0 // Same reason as above

void assert(int expression) {
    (void)expression;
}

// make sure assert is always enabled
#ifdef NDEBUG
#undef NDEBUG
#endif

static
void test_str_dup(char *data)
{
	str s = str_null;
    str src = str_ref(data);
    str_cpy(&s, src);

	str_free(s); str_free(src);
}

static
void test_str_clear(char *data)
{
	str s = str_null;
    str src = str_ref(data);

	assert(str_cpy(&s, src) == 0);

	assert(str_len(s) == 3);
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_clear(&s);
    str_free(src);

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_str_move(char *data)
{
	str s1 = str_null;
    str src = str_ref(data);

	assert(str_cpy(&s1, src) == 0);
	str s2 = str_move(&s1);

	assert(str_is_empty(s1));
	assert(str_is_ref(s1));

	assert(str_is_owner(s2));
	assert(str_eq(s2, src));

	str_free(s2);
	passed;
}

static
void test_str_pass(char *data)
{
	str s1 = str_null;
    str src = str_ref(data);

	assert(str_cpy(&s1, src) == 0);

	str s2 = str_pass(&s1);

	assert(str_is_ref(s1));
	assert(str_eq(s1, src));

	assert(str_is_owner(s2));
	assert(str_eq(s2, src));

	str_free(s2);
	passed;
}

static
void test_str_ref(char *data)
{
    str s = str_ref(data);

	assert(str_len(s) == 3);
	assert(str_is_ref(s));

	s = str_ref(s);

	assert(str_is_ref(s));
}

static
void test_str_cmp(char *data)
{
    const str s = str_ref(data);

	assert(str_cmp(s, s) == 0);

	passed;
}

static
void test_str_cmp_ci(char *data)
{
    const str s = str_ref(data);

	assert(str_cmp_ci(s, s) == 0);
	assert(str_cmp_ci(s, str_lit("zzz")) == 0);
	assert(str_cmp_ci(s, str_lit("zz")) > 0);
	assert(str_cmp_ci(s, str_lit("zzzz")) < 0);
	assert(str_cmp_ci(s, str_null) > 0);
	assert(str_cmp_ci(str_null, s) < 0);
	assert(str_cmp_ci(str_null, str_null) == 0);
	assert(str_cmp_ci(s, str_lit("ZZZ")) == 0);
	assert(str_cmp_ci(s, str_lit("ZZ")) > 0);
	assert(str_cmp_ci(s, str_lit("ZZZZ")) < 0);
	assert(str_eq_ci(s, str_lit("ZZZ")));

	passed;
}

static
void test_str_acquire(char *data)
{
	str s = str_acquire(data);

	assert(str_is_owner(s));
	assert(str_eq(s, str_lit("ZZZ")));
	assert(*str_end(s) == 0);

	str_free(s);
	passed;
}

static
void test_str_cat(char *data)
{
	str s = str_ref(data);

	assert(str_cat(&s, str_lit("AAA"), str_lit("BBB"), str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("AAABBBCCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_cat(&s, str_null, str_null, str_null) == 0);	// this simply clears the target string

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_str_join(char *data)
{
	str s = str_ref(data);

	assert(str_join(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("AAA_BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_null, str_lit("BBB"), str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("_BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_lit("AAA"), str_null, str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("AAA__CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_null) == 0);

	assert(str_eq(s, str_lit("AAA_BBB_")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_null, str_null, str_null) == 0);

	assert(str_eq(s, str_lit("__")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_null) == 0);	// this simply clears the target string

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_composition(char *data)
{
	str s = str_ref(data);

	assert(str_join(&s, s, str_lit("Here"), str_lit("there"), str_lit("and everywhere")) == 0);
	assert(str_cat(&s, s, str_lit("...")) == 0);

	assert(str_eq(s, str_lit("Here, there, and everywhere...")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_free(s);
	passed;
}

static
void test_sort(char *data)
{
	str src[] = { str_ref(data), str_lit("zzz"), str_lit("aaa"), str_lit("bbb") };

	str_sort_range(str_order_asc, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq(src[0], str_lit("aaa")));
	assert(str_eq(src[1], str_lit("bbb")));
	assert(str_eq(src[2], str_lit("z")));
	assert(str_eq(src[3], str_lit("zzz")));

	str_sort_range(str_order_desc, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq(src[0], str_lit("zzz")));
	assert(str_eq(src[1], str_lit("z")));
	assert(str_eq(src[2], str_lit("bbb")));
	assert(str_eq(src[3], str_lit("aaa")));

	passed;
}

static
void test_sort_ci(char *data)
{
	str src[] = { str_ref(data), str_lit("zzz"), str_lit("aaa"), str_lit("AAA") };

	str_sort_range(str_order_asc_ci, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq_ci(src[0], str_lit("aaa")));
	assert(str_eq_ci(src[1], str_lit("aaa")));
	assert(str_eq_ci(src[2], str_lit("zzz")));
	assert(str_eq_ci(src[3], str_lit("zzz")));

	str_sort_range(str_order_desc_ci, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq_ci(src[0], str_lit("zzz")));
	assert(str_eq_ci(src[1], str_lit("zzz")));
	assert(str_eq_ci(src[2], str_lit("aaa")));
	assert(str_eq_ci(src[3], str_lit("aaa")));

	passed;
}

static
void test_search(char *data)
{
	str src[] = { str_ref(data), str_lit("zzz"), str_lit("aaa"), str_lit("bbb") };
	const size_t count = sizeof(src)/sizeof(src[0]);

	str_sort_range(str_order_asc, src, count);

	assert(str_search_range(src[0], src, count) == &src[0]);
	assert(str_search_range(src[1], src, count) == &src[1]);
	assert(str_search_range(src[2], src, count) == &src[2]);
	assert(str_search_range(src[3], src, count) == &src[3]);
	assert(str_search_range(str_lit("xxx"), src, count) == NULL);

	passed;
}

static
void test_prefix(char *data)
{
	const str s = str_ref(data);

	assert(str_has_prefix(s, str_null));
	assert(str_has_prefix(s, str_lit("a")));
	assert(str_has_prefix(s, str_lit("ab")));
	assert(str_has_prefix(s, str_lit("abc")));
	assert(str_has_prefix(s, str_lit("abcd")));

	assert(!str_has_prefix(s, str_lit("zzz")));
	assert(!str_has_prefix(s, str_lit("abcde")));

	passed;
}

static
void test_suffix(char *data)
{
	const str s = str_ref(data);

	assert(str_has_suffix(s, str_null));
	assert(str_has_suffix(s, str_lit("d")));
	assert(str_has_suffix(s, str_lit("cd")));
	assert(str_has_suffix(s, str_lit("bcd")));
	assert(str_has_suffix(s, str_lit("abcd")));

	assert(!str_has_suffix(s, str_lit("zzz")));
	assert(!str_has_suffix(s, str_lit("_abcd")));

	passed;
}


static
bool part_pred(const str s) { return str_len(s) < 2; }

static
void test_partition_range(char *data)
{
	str src[] = { str_ref(data), str_lit("a"), str_lit("aaaa"), str_lit("z") };

	assert(str_partition_range(part_pred, src, 1) == 0);

	assert(str_partition_range(part_pred, src, sizeof(src)/sizeof(src[0])) == 2);
	assert(str_eq(src[0], str_lit("a")));
	assert(str_eq(src[1], str_lit("z")));
	assert(str_partition_range(part_pred, src, 1) == 1);

	src[0] = str_lit("?");
	src[2] = str_lit("*");

	assert(str_partition_range(part_pred, src, sizeof(src)/sizeof(src[0])) == 3);
	assert(str_eq(src[0], str_lit("?")));
	assert(str_eq(src[1], str_lit("z")));
	assert(str_eq(src[2], str_lit("*")));
	assert(str_eq(src[3], str_lit("aaa")));

	assert(str_partition_range(part_pred, NULL, 42) == 0);
	assert(str_partition_range(part_pred, src, 0) == 0);

	passed;
}

static
void test_unique_range(char *data)
{
	str src[] = {
		str_ref(data),
		str_lit("aaa"),
		str_lit("zzz"),
		str_lit("bbb"),
		str_lit("aaa"),
		str_lit("ccc"),
		str_lit("ccc"),
		str_lit("aaa"),
		str_lit("ccc"),
		str_lit("zzz")
	};

	assert(str_unique_range(src, sizeof(src)/sizeof(src[0])) == 4);
	assert(str_eq(src[0], str_lit("aaa")));
	assert(str_eq(src[1], str_lit("bbb")));
	assert(str_eq(src[2], str_lit("ccc")));
	assert(str_eq(src[3], str_lit("zzz")));

	passed;
}

static
void test_tok(char *data)
{
	typedef struct
	{
		const str src, delim;
		const unsigned n_tok;
		const str tok[3];
	} test_data;

	const test_data t[] =
	{
		{
			str_ref(data),
			str_lit(","),
			3,
			{ str_lit("a"), str_lit("b"), str_lit("c") }
		},
		{
			str_lit(",,a,b,,c,"),
			str_lit(","),
			3,
			{ str_lit("a"), str_lit("b"), str_lit("c") }
		},
		{
			str_lit("aaa;=~bbb~,=ccc="),
			str_lit(",;=~"),
			3,
			{ str_lit("aaa"), str_lit("bbb"), str_lit("ccc") }
		},
		{
			str_lit(""),
			str_lit(","),
			0,
			{ }
		},
		{
			str_lit(""),
			str_lit(""),
			0,
			{ }
		},
		{
			str_lit(",.;,.;;.,;.,"),
			str_lit(",.;"),
			0,
			{ }
		},
		{
			str_lit("aaa,bbb,ccc"),
			str_lit(""),
			1,
			{ str_lit("aaa,bbb,ccc") }
		},
		{
			str_lit("aaa,bbb,ccc"),
			str_lit(";-="),
			1,
			{ str_lit("aaa,bbb,ccc") }
		}
	};

	for(unsigned i = 0; i < sizeof(t)/sizeof(t[0]); ++i)
	{
		unsigned tok_count = 0;

		str tok = str_null;
		str_tok_state state;

		str_tok_init(&state, t[i].src, t[i].delim);

		while(str_tok(&tok, &state))
		{
// 			printf("%u-%u: \"%.*s\" %zu\n",
// 					i, tok_count, (int)str_len(tok), str_ptr(tok), str_len(tok));
// 			fflush(stdout);

			assert(tok_count < t[i].n_tok);
			assert(str_eq(tok, t[i].tok[tok_count]));

			++tok_count;
		}

		assert(tok_count == t[i].n_tok);
	}

	passed;
}

static
void test_partition(char *data)
{
	typedef struct
	{
		const bool res;
		const str src, patt, pref, suff;
	} test_data;

	const test_data t[] =
	{
		{ true, str_ref(data), str_lit("abc"), str_lit("..."), str_lit("...") },
		{ true, str_lit("......abc"), str_lit("abc"), str_lit("......"), str_null },
		{ true, str_lit("abc......"), str_lit("abc"), str_null, str_lit("......") },

		{ true, str_lit("...a..."), str_lit("a"), str_lit("..."), str_lit("...") },
		{ true, str_lit("......a"), str_lit("a"), str_lit("......"), str_null },
		{ true, str_lit("a......"), str_lit("a"), str_null, str_lit("......") },

		{ false, str_lit("zzz"), str_null, str_lit("zzz"), str_null },
		{ false, str_null, str_lit("zzz"), str_null, str_null },
		{ false, str_null, str_null, str_null, str_null },

		{ false, str_lit("...zzz..."), str_lit("xxx"), str_lit("...zzz..."), str_null },
		{ false, str_lit("...xxz..."), str_lit("xxx"), str_lit("...xxz..."), str_null },
		{ true, str_lit("...xxz...xxx."), str_lit("xxx"), str_lit("...xxz..."), str_lit(".") },
		{ true, str_lit(u8"...цифры___"), str_lit(u8"цифры"), str_lit("..."), str_lit("___") }
	};

	for(unsigned i = 0; i < sizeof(t)/sizeof(t[0]); ++i)
	{
		str pref = str_lit("???"), suff = str_lit("???");

		assert(str_partition(t[i].src, t[i].patt, &pref, &suff) == t[i].res);
		assert(str_eq(pref, t[i].pref));
		assert(str_eq(suff, t[i].suff));
	}

	passed;
}


static void (*fuzz_funcs[])(char*) = {
    test_str_dup, test_str_clear, test_str_move, test_str_pass, test_str_ref,
    test_str_cmp, test_str_cmp_ci, test_str_acquire, test_str_cat, test_str_join,
    test_composition, test_sort, test_search, test_prefix, test_suffix,
    test_partition_range, test_unique_range, test_tok, test_partition
};

#define FUZZ_FUNC_SIZE sizeof(fuzz_funcs) / sizeof(fuzz_funcs[0])

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < MIN_INPUT_SIZE) return 0;

    // Create a non-const, null-terminated copy of fuzzer data
    char str_data[size];
    memcpy(str_data, data, size);
    str_data[size - 1] = '\0';

    size_t fnc_idx = 0;
    size_t i = 0;

    while (i < size) {
        fuzz_funcs[fnc_idx](str_data + i);

        // Iterate
        fnc_idx = (fnc_idx + 1) % FUZZ_FUNC_SIZE;
        i += 1 + strlen(str_data + i);
    }
    return 0;

}
