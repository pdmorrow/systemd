/* SPDX-License-Identifier: LGPL-2.1+ */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "alloc-util.h"
#include "macro.h"

/* What is interpreted as whitespace? */
#define WHITESPACE        " \t\n\r"
#define NEWLINE           "\n\r"
#define QUOTES            "\"\'"
#define COMMENTS          "#;"
#define GLOB_CHARS        "*?["
#define DIGITS            "0123456789"
#define LOWERCASE_LETTERS "abcdefghijklmnopqrstuvwxyz"
#define UPPERCASE_LETTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define LETTERS           LOWERCASE_LETTERS UPPERCASE_LETTERS
#define ALPHANUMERICAL    LETTERS DIGITS
#define HEXDIGITS         DIGITS "abcdefABCDEF"

#define streq(a,b) (strcmp((a),(b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)
#define strcaseeq(a,b) (strcasecmp((a),(b)) == 0)
#define strncaseeq(a, b, n) (strncasecmp((a), (b), (n)) == 0)

int strcmp_ptr(const char *a, const char *b) _pure_;

static inline bool streq_ptr(const char *a, const char *b) {
        return strcmp_ptr(a, b) == 0;
}

static inline const char* strempty(const char *s) {
        return s ?: "";
}

static inline const char* strnull(const char *s) {
        return s ?: "(null)";
}

static inline const char *strna(const char *s) {
        return s ?: "n/a";
}

static inline const char* yes_no(bool b) {
        return b ? "yes" : "no";
}

static inline const char* true_false(bool b) {
        return b ? "true" : "false";
}

static inline const char* one_zero(bool b) {
        return b ? "1" : "0";
}

static inline const char* enable_disable(bool b) {
        return b ? "enable" : "disable";
}

static inline bool isempty(const char *p) {
        return !p || !p[0];
}

static inline const char *empty_to_null(const char *p) {
        return isempty(p) ? NULL : p;
}

static inline const char *empty_to_dash(const char *str) {
        return isempty(str) ? "-" : str;
}

static inline bool empty_or_dash(const char *str) {
        return !str ||
                str[0] == 0 ||
                (str[0] == '-' && str[1] == 0);
}

static inline const char *empty_or_dash_to_null(const char *p) {
        return empty_or_dash(p) ? NULL : p;
}

static inline char *startswith(const char *s, const char *prefix) {
        size_t l;

        l = strlen(prefix);
        if (strncmp(s, prefix, l) == 0)
                return (char*) s + l;

        return NULL;
}

static inline char *startswith_no_case(const char *s, const char *prefix) {
        size_t l;

        l = strlen(prefix);
        if (strncasecmp(s, prefix, l) == 0)
                return (char*) s + l;

        return NULL;
}

char *endswith(const char *s, const char *postfix) _pure_;
char *endswith_no_case(const char *s, const char *postfix) _pure_;

char *first_word(const char *s, const char *word) _pure_;

typedef enum SplitFlags {
        SPLIT_QUOTES                     = 0x01 << 0,
        SPLIT_RELAX                      = 0x01 << 1,
} SplitFlags;

const char* split(const char **state, size_t *l, const char *separator, SplitFlags flags);

#define FOREACH_WORD(word, length, s, state)                            \
        _FOREACH_WORD(word, length, s, WHITESPACE, 0, state)

#define FOREACH_WORD_SEPARATOR(word, length, s, separator, state)       \
        _FOREACH_WORD(word, length, s, separator, 0, state)

#define _FOREACH_WORD(word, length, s, separator, flags, state)         \
        for ((state) = (s), (word) = split(&(state), &(length), (separator), (flags)); (word); (word) = split(&(state), &(length), (separator), (flags)))

char *strnappend(const char *s, const char *suffix, size_t length);

char *strjoin_real(const char *x, ...) _sentinel_;
#define strjoin(a, ...) strjoin_real((a), __VA_ARGS__, NULL)

#define strjoina(a, ...)                                                \
        ({                                                              \
                const char *_appendees_[] = { a, __VA_ARGS__ };         \
                char *_d_, *_p_;                                        \
                size_t _len_ = 0;                                          \
                size_t _i_;                                           \
                for (_i_ = 0; _i_ < ELEMENTSOF(_appendees_) && _appendees_[_i_]; _i_++) \
                        _len_ += strlen(_appendees_[_i_]);              \
                _p_ = _d_ = newa(char, _len_ + 1);                      \
                for (_i_ = 0; _i_ < ELEMENTSOF(_appendees_) && _appendees_[_i_]; _i_++) \
                        _p_ = stpcpy(_p_, _appendees_[_i_]);            \
                *_p_ = 0;                                               \
                _d_;                                                    \
        })

char *strstrip(char *s);
char *delete_chars(char *s, const char *bad);
char *delete_trailing_chars(char *s, const char *bad);
char *truncate_nl(char *s);

static inline char *skip_leading_chars(const char *s, const char *bad) {

        if (!s)
                return NULL;

        if (!bad)
                bad = WHITESPACE;

        return (char*) s + strspn(s, bad);
}

char ascii_tolower(char x);
char *ascii_strlower(char *s);
char *ascii_strlower_n(char *s, size_t n);

char ascii_toupper(char x);
char *ascii_strupper(char *s);

int ascii_strcasecmp_n(const char *a, const char *b, size_t n);
int ascii_strcasecmp_nn(const char *a, size_t n, const char *b, size_t m);

bool chars_intersect(const char *a, const char *b) _pure_;

static inline bool _pure_ in_charset(const char *s, const char* charset) {
        assert(s);
        assert(charset);
        return s[strspn(s, charset)] == '\0';
}

bool string_has_cc(const char *p, const char *ok) _pure_;

char *ellipsize_mem(const char *s, size_t old_length_bytes, size_t new_length_columns, unsigned percent);
static inline char *ellipsize(const char *s, size_t length, unsigned percent) {
        return ellipsize_mem(s, strlen(s), length, percent);
}

char *cellescape(char *buf, size_t len, const char *s);

/* This limit is arbitrary, enough to give some idea what the string contains */
#define CELLESCAPE_DEFAULT_LENGTH 64

char* strshorten(char *s, size_t l);

char *strreplace(const char *text, const char *old_string, const char *new_string);

char *strip_tab_ansi(char **ibuf, size_t *_isz, size_t highlight[2]);

char *strextend_with_separator(char **x, const char *separator, ...) _sentinel_;

#define strextend(x, ...) strextend_with_separator(x, NULL, __VA_ARGS__)

char *strrep(const char *s, unsigned n);

int string_prefix(const char *s, const char *sep, char **l);

int split_pair(const char *s, const char *sep, char **l, char **r);

int free_and_strdup(char **p, const char *s);
static inline int free_and_strdup_warn(char **p, const char *s) {
        if (free_and_strdup(p, s) < 0)
                return log_oom();
        return 0;
}
int free_and_strndup(char **p, const char *s, size_t l);

bool string_is_safe(const char *p) _pure_;

static inline size_t strlen_ptr(const char *s) {
        if (!s)
                return 0;

        return strlen(s);
}

DISABLE_WARNING_STRINGOP_TRUNCATION;
static inline void strncpy_exact(char *buf, const char *src, size_t buf_len) {
        strncpy(buf, src, buf_len);
}
REENABLE_WARNING;

/* Like startswith(), but operates on arbitrary memory blocks */
static inline void *memory_startswith(const void *p, size_t sz, const char *token) {
        size_t n;

        assert(token);

        n = strlen(token);
        if (sz < n)
                return NULL;

        assert(p);

        if (memcmp(p, token, n) != 0)
                return NULL;

        return (uint8_t*) p + n;
}

/* Like startswith_no_case(), but operates on arbitrary memory blocks.
 * It works only for ASCII strings.
 */
static inline void *memory_startswith_no_case(const void *p, size_t sz, const char *token) {
        size_t n, i;

        assert(token);

        n = strlen(token);
        if (sz < n)
                return NULL;

        assert(p);

        for (i = 0; i < n; i++) {
                if (ascii_tolower(((char *)p)[i]) != ascii_tolower(token[i]))
                        return NULL;
        }

        return (uint8_t*) p + n;
}

static inline char* str_realloc(char **p) {
        /* Reallocate *p to actual size */

        if (!*p)
                return NULL;

        char *t = realloc(*p, strlen(*p) + 1);
        if (!t)
                return NULL;

        return (*p = t);
}
