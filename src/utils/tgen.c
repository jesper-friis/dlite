/* tgen.c -- simple templated text generator
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "compat.h"
#include "err.h"
#include "tgen.h"


/* Chunk size when allocating output buffer */
#define CHUNKSIZE  512

/** Convenient macros for failing */
#define FAIL(errnum, msg) do {                          \
    retval = err(errnum, msg); goto fail; } while (0)
#define FAIL1(errnum, msg, a1) do {                             \
    retval = err(errnum, msg, a1); goto fail; } while (0)
#define FAIL2(errnum, msg, a1, a2) do {                         \
    retval = err(1, msg, a1, a2); goto fail; } while (0)


/* Whether to convert standard escape sequences. */
int tgen_convert_escape_sequences = 1;



/***************************************************************
 * Help functions
 ***************************************************************/

/* Returns non-zero if the format specifier `fmt` is valid.  It should
   have length `len`. */
static int validate_fmt(const char *fmt, int len)
{
  const char *p=fmt;
  if (*(p++) != '%') return 0;
  if (*p == '-') p++;           /* ALIGN */
  while (isdigit(*p)) p++;      /* WIDTH */
  if (*p == '.') {              /* PREC */
    p++;
    if (!isdigit(*(p++))) return 0;
    while (isdigit(*p)) p++;
  }
  if (!strchr("slUT", *(p++)))  /* CASE */
      return 0;
  if (p != fmt + len)           /* check length */
    return 0;
  return 1;
}

/* Returns the number of bytes from the position in a template that is
   pointed to by `p` to the first unpaired end brace.  Returns -1 if
   no unpaired end brace can be found. */
static int length_to_endbrace(const char *p)
{
  const char *q = p;
  int depth = 0;
  while (*q && *q != '}') {
    int n = strcspn(q, "{}");
    q += n;
    switch (*(q++)) {
    case '\0':
      return -1;
    case '{':
      if (*q == '{')
        q++;
      else if (*q != '}')
        depth++;
      break;
    case '}':
      if (*q == '}')
        q++;
      else if (depth-- <= 0)
        q--;
      break;
    default:
      abort();  /* should never be reached */
    }
  }
  return q - p;
}

/* Returns the number of bytes from the position in a template that is
   pointed to by `p` to variable `var`.  Only the number of bytes
   until the starting brace are counted.  If `maxlen` is equal or larger
   than zero, at most `maxlen` bytes are considered.

   Returns -1 if variable `var` was not found. */
static int length_to_var(const char *p, const char *var, int maxlen)
{
  const char *q = p;
  while (1) {
    size_t n = strcspn(q, "{");
    if (!q[n]) return -1;
    q += n + 1;
    if (maxlen >= 0 && q > p + maxlen) {
      return -1;
    } else if (*q == '{') {
      q++;
    } else {
      size_t m = strcspn(q, "%:}");
      if (strncmp(q, var, m) == 0)
        return q - p - 1;
      else
        q += length_to_endbrace(q);
    }
  }
}


/* Evaluates condition `cond`.

   Returns 1 if conds evaluates to true, 0 if cond is false or -1 on error. */
static int evaluate_cond(const char *cond, int len, const TGenSubs *subs,
                         void *context)
{
  int retval=-1;
  char *p, *endp;
  TGenBuf s;
  tgen_buf_init(&s);
  if (tgen_append(&s, cond, len, subs, context)) goto fail;
  if (!s.buf || !*s.buf) {
    retval = 0;
  } else if ((p = strstr(s.buf, "=="))) {
    *p = '\0';
    retval = (strcmp(s.buf, p+2) == 0) ? 1 : 0;
  } else if ((p = strstr(s.buf, "!="))) {
    *p = '\0';
    retval = (strcmp(s.buf, p+2) != 0) ? 1 : 0;
  } else {
    retval = (strtol(s.buf, &endp, 0) == 0 && !*endp) ? 0 : 1;
  }
 fail:
  tgen_buf_deinit(&s);
  return retval;
}


/* Implements template conditional:

       {@if:COND}...{@elif:COND}...{@else}...{@endif}

   Returns the number of bytes consumed or -1 on error.
*/
static int builtin_if(TGenBuf *s, const char *template, const TGenSubs *subs,
                      void *context)
{
  const char *endp, *t = template;
  int cond, m, n = strcspn(t, ":");
  if (strncmp(t, "@if", n) || !t[n]) return -1;
  t += n + 1;
  if ((n = length_to_endbrace(t)) < 0 || !t[n]) return -1;
  if ((cond = evaluate_cond(t, n, subs, context)) < 0) return -1;
  t += n + 1;
  if ((n = length_to_var(t, "@endif", -1)) < 0) return -1;
  if ((m = length_to_endbrace(t+n+1)) < 0) return -1;
  endp = t + n + m + 2;

  while ((n = length_to_var(t, "@elif", endp - t)) >= 0) {
    if (cond) {
      if (tgen_append(s, t, n, subs, context)) return -1;
      return endp - template;
    }
    if ((t += n + strcspn(t+n, ":")) && !*t) return -1;
    if (!*(t++)) return -1;
    if ((n = length_to_endbrace(t)) < 0) return -1;
    if ((cond = evaluate_cond(t, n, subs, context)) < 0) return -1;
    t += n + 1;
  }
  if ((n = length_to_var(t, "@else", endp - t)) >= 0) {
    if (cond) {
      if (tgen_append(s, t, n, subs, context)) return -1;
      return endp - template;
    } else {
      if ((m = length_to_endbrace(t+n+1)) < 0) return -1;
      t += n + m + 2;
      if ((n = length_to_var(t, "@endif", -1)) < 0) return -1;
      if (tgen_append(s, t, n, subs, context)) return -1;
      return endp - template;
    }
  }
  if ((n = length_to_var(t, "@endif", endp - t)) >= 0) {
    if (cond) {
      if (tgen_append(s, t, n, subs, context)) return -1;
      return endp - template;
    }
  }
  return endp - template;
}



/***************************************************************
 * Utility functions
 ***************************************************************/

/*
  Copies at most `n` bytes from `src` and writing them to `dest`.
  If `n` is negative, all of `src` is copied.

  The following standard escape sequences are converted:

      \a, \b, \f, \n, \r, \t, \v \\

  in addition to escaped newlines.

  Returns the number of characters written to `dest`.
 */
int tgen_escaped_copy(char *dest, const char *src, int n)
{
  char *q=dest;
  const char *p=src, *pend=src+n;
  if (n < 0) n = strlen(src);
  pend = src + n;
  while(p < pend) {
    if (*p == '\\') {
      if (p+1 < pend) {
        switch (*(++p)) {
        case 'a':  *(q++) = '\a'; break;
        case 'b':  *(q++) = '\b'; break;
        case 'f':  *(q++) = '\f'; break;
        case 'n':  *(q++) = '\n'; break;
        case 'r':  *(q++) = '\r'; break;
        case 't':  *(q++) = '\t'; break;
        case 'v':  *(q++) = '\v'; break;
        case '\\': *(q++) = '\\'; break;
        case '.':  break;                 /* escaped ".", just consume */
        case '\n': break;                 /* escaped newline, just consume */
        case '\r':
	  if (p[1] == '\n') p++;          /* escaped newline, Windows or Mac */
	  break;
        default:   *(q++) = *p;   break;
        }
      } else {
        *(q++) = '\\';  /* last character is a backslash */
      }
    } else {
      *(q++) = *p;
    }
    p++;
  }
  return q - dest;
}

/*
  Sets the case of the (sub)string `s` according to `casemode`.  `len`
  is the of length of the substring.  If `len` is negative, the case
  is applied to the whole string.

  Valid values for `casemode` are:
    - "s": no change in case
    - "l": convert to lower case
    - "U": convert to upper case
    - "T": convert to title case (convert first character to upper case
           and the rest to lower case)

  Returns non-zero on error.
 */
int tgen_setcase(char *s, int len, int casemode)
{
  int i;
  if (len < 0)
    len = strlen(s);
  switch (casemode) {
  case 's':
    return 0;
  case 'l':
    for (i=0; i<len; i++) s[i] = tolower(s[i]);
    return 0;
  case 'U':
    for (i=0; i<len; i++) s[i] = toupper(s[i]);
    return 0;
  case 'T':
    s[0] = toupper(s[0]);
    for (i=1; i<len; i++) s[i] = tolower(s[i]);
    return 0;
  }
  return 1;
}

/*
  Converts camel case to lower case and underscores.

  Returns a newly allocated string based on the substring of `s` with
  length `len`. Camel case in `s` is in the returned string converted
  to lower case and underscores.  On error, NULL is returned.

  If `len` is negative, all of `s` is used.

  Examples:
    "CamelCaseWord" -> "camel_case_word"
    "A sentence with CamelCase" -> "a sentence with camel_case"
 */
char *tgen_camel_to_underscore(const char *s, int len)
{
  TGenBuf buf;
  int i, n=0;
  tgen_buf_init(&buf);
  if (len < 0) len = strlen(s);
  for (i=0; i<len; i++) {
    if (isupper(s[i]) && n++) tgen_buf_append(&buf, "_", -1);
    tgen_buf_append_fmt(&buf, "%c", tolower(s[i]));
    if (isspace(s[i])) n = 0;
  }
  return tgen_buf_steal(&buf);
}

/*
  Initiates output buffer.
 */
void tgen_buf_init(TGenBuf *s)
{
  memset(s, 0, sizeof(TGenBuf));
}

/*
  Clears output buffer and free's up all memory.
 */
void tgen_buf_deinit(TGenBuf *s)
{
  if (s->buf) free(s->buf);
  memset(s, 0, sizeof(TGenBuf));
}

/*
  Like tgen_buf_deinit(), but instead of free up the internal buffer, it
  is returned.  The returned string is owned by the caller and should be
  free'ed with free().
 */
char *tgen_buf_steal(TGenBuf *s)
{
  char *p = s->buf;
  memset(s, 0, sizeof(TGenBuf));
  return p;
}

/*
  Returns the length of buffer `s`.
 */
size_t tgen_buf_length(const TGenBuf *s)
{
  return s->pos;
}

/*
  Returns a pointer to the content of the output buffer.
 */
const char *tgen_buf_get(const TGenBuf *s)
{
  return s->buf;
}

/*
  Appends `n` bytes from string `src` to end of `s`.  If `n` is
  negative, all of `str` (NUL-terminated) is appended.

  If the global variable `convert_escape_sequences` is non-zero
  (default), then standard escape sequences are converted during
  copying.

  Returns number of characters appended or -1 on error.
 */
int tgen_buf_append(TGenBuf *s, const char *src, int n)
{
  size_t len = (n < 0) ? strlen(src) : (size_t)n;
  size_t needed = s->pos + len;
  assert(!s->size || s->buf);

  if (needed >= s->size) {
    s->size = (needed / CHUNKSIZE + 1) * CHUNKSIZE;
    if (!(s->buf = realloc(s->buf, s->size)))
      return err(TGenAllocationError, "allocation failure");
  }

  if (tgen_convert_escape_sequences) {
    s->pos += tgen_escaped_copy(s->buf + s->pos, src, len);
  } else {
    memcpy(s->buf + s->pos, src, len);
    s->pos += len;
  }
  s->buf[s->pos] = '\0';
  return len;
}

/*
  Like tgen_buf_append() but allows printf() formatting of the input.
 */
int tgen_buf_append_fmt(TGenBuf *s, const char *fmt, ...)
{
  int retval;
  va_list ap;
  va_start(ap, fmt);
  retval = tgen_buf_append_vfmt(s, fmt, ap);
  va_end(ap);
  return retval;
}

/*
  Like tgen_buf_append_fmt(), but takes a `va_list` instead of a
  variable number of arguments.
 */
int tgen_buf_append_vfmt(TGenBuf *s, const char *fmt, va_list ap)
{
  int n, retval;
  char buf[128], *src=buf;
  va_list ap2;
  va_copy(ap2, ap);

  /* First try to write to a stack-allocated buffer instead of allocating
     with malloc().  Resort to malloc if this fails... */
  if ((n = vsnprintf(buf, sizeof(buf), fmt, ap)) < 1)
    FAIL1(TGenFormatError, "invalid format string \"%s\"", fmt);
  if (n <= (int)sizeof(buf)) {
    src = malloc(n + 1);
    if ((vsnprintf(src, n+1, fmt, ap2)) != n)
      FAIL1(TGenFormatError, "invalid format string \"%s\"", fmt);
  }
  retval = tgen_buf_append(s, src, n);

 fail:
  if (src != buf) free(src);
  va_end(ap2);
  return retval;
}

/*
  Removes the last `n` characters appended to buffer `s`.  If `n` is larger
  than the buffer length, it is truncated.

  Returns the number of characters removed.
 */
int tgen_buf_unappend(TGenBuf *s, size_t n)
{
  int removed = (n < s->pos) ? n : s->pos;
  s->pos -= removed;
  s->buf[s->pos] = '\0';
  return removed;
}

/*
  Pad buffer with character `c` until `n` characters has been written since
  the last newline.  If more than `n` characters has already been written
  since the last newline, nothing is added.

  Returns number of padding added or -1 on error.
*/
int tgen_buf_calign(TGenBuf *s, int c, int n)
{
  char str[] = {c, '\0'};
  int retval, i = 0;
  while (i<n && i <= (int)s->pos && s->buf[s->pos-i] != '\n') i++;
  retval = n - i + 1;
  while (i++ <= n) tgen_buf_append(s, str, 1);
  return retval;
}

/*
  Like tgen_buf_calign() but pads with space.
*/
int tgen_buf_align(TGenBuf *s, int n)
{
  return tgen_buf_calign(s, ' ', n);
}


/*
  Returns the line number of position `t` in `template`.
*/
int tgen_lineno(const char *template, const char *t)
{
  int lineno = 1;
  const char *tt = template;
  while (tt < t) {
    assert(*tt);  /* t must point within template */
    if (*tt == '\n') lineno++;
    tt++;
  }
  return lineno;
}


/*
  Reads a file and returns a newly allocated buffer with its content.
  If `filename` is NULL, read from stdin.

  Returns NULL on error.
 */
char *tgen_readfile(const char *filename)
{
  int retval=0;
  char buf[4096], *p=NULL;
  size_t n, pos, size=0;
  FILE *fp=stdin;

  if (filename && !(fp = fopen(filename, "rb")))
    FAIL1(TGenIOError, "cannot open file \"%s\"", filename);

  /* read in chunks of sizeof(buf) and copy to allocated memory... */
  while ((n = fread(buf, 1, sizeof(buf), fp)) == sizeof(buf)) {
    if (ferror(fp)) FAIL1(TGenIOError, "error reading file \"%s\"", filename);
    size_t pos = size;
    size += n;
    p = realloc(p, size);
    memcpy(p + pos, buf, n);
  }

  /* copy last chunk to allocated memory and add space for NUL-termination */
  pos = size;
  size += n + 1;
  p = realloc(p, size);
  memcpy(p + pos, buf, n);
  p[size-1] = '\0';

 fail:
  if (fp && fp != stdin) fclose(fp);
  if (retval && p) free(p);
  return p;
}


/***************************************************************
 * Functions for managing substitutions
 ***************************************************************/

/*
   Initiates memory used by `subs`.
*/
void tgen_subs_init(TGenSubs *subs)
{
  memset(subs, 0, sizeof(TGenSubs));
  map_init(&subs->map);
}

/*
   Deinitiates memory used by `subs`.
*/
void tgen_subs_deinit(TGenSubs *subs)
{
  int i;
  map_deinit(&subs->map);
  for (i=0; i < subs->nsubs; i++) {
    TGenSub *s = subs->subs + i;
    free(s->var);
    if (s->repl) free(s->repl);
  }
  if (subs->subs) free(subs->subs);
  memset(subs, 0, sizeof(TGenSubs));
}

/*
  Returns substitution corresponding to `var` or NULL if there are no
  matching substitution.
*/
const TGenSub *tgen_subs_get(const TGenSubs *subs, const char *var)
{
  return tgen_subs_getn(subs, var, -1);
}


/*
  Like tgen_subs_get(), but allows `var` to not be NUL-terminated by
  specifying its length with `len`. If `len` is negative, this is equivalent
  to calling tgen_subs_get().
*/
const TGenSub *tgen_subs_getn(const TGenSubs *subs, const char *var, int len)
{
  TGenSub *s = NULL;
  int *ip;
  char *name = (len < 0) ? (char *)var : strndup(var, len);

  if ((ip = map_get((map_int_t *)&subs->map, name)))
    s = subs->subs + *ip;

  if (len >= 0) free(name);
  return s;
}

/*
  Adds variable `var` to list of substitutions `subs`.  `repl` and
  `func` are the corresponding replacement string and generator
  function, respectively.

  Returns non-zero on error.
*/
int tgen_subs_set(TGenSubs *subs, const char *var, const char *repl,
                  TGenFun func)
{
  return tgen_subs_setn(subs, var, -1, repl, func);
}

/*
  Like tgen_subs_set(), but allows `var` to not be NUL-terminated by
  specifying its length with `len`.  If `len` is negative, this is
  equivalent to calling tgen_subs_set().

  Returns non-zero on error.
*/
int tgen_subs_setn(TGenSubs *subs, const char *var, int len,
                   const char *repl, TGenFun func)
{
  TGenSub *s;
  int *ip;
  char *name = (len < 0) ? strdup(var) : strndup(var, len);
  assert(name);

  if ((ip = map_get((map_int_t *)&subs->map, name))) {
    s = subs->subs + *ip;
    if (s->repl) free(s->repl);
    if (repl) s->repl = strdup(repl);
    s->func = func;
    free(name);
  } else {
    if (map_set((map_int_t *)&subs->map, name, subs->nsubs)) {
      char msg[80];
      snprintf(msg, sizeof(msg), "cannot add substitution for '%s'", name);
      free(name);
      return err(TGenMapError, "%s", msg);
    }
    if (subs->nsubs >= subs->size) {
      subs->size += 128;
      subs->subs = realloc(subs->subs, subs->size*sizeof(TGenSub));
    }
    s = subs->subs + subs->nsubs;
    s->var = name;
    s->repl = (repl) ? strdup(repl) : NULL;
    s->func = func;
    subs->nsubs++;
  }
  return 0;
}

/*
  Like tgen_subs_set(), but allows printf() formatting of the
  replacement string.

  Returns non-zero on error.
*/
int tgen_subs_set_fmt(TGenSubs *subs, const char *var, TGenFun func,
                      const char *repl_fmt, ...)
{
  int retval;
  va_list ap;
  va_start(ap, repl_fmt);
  retval = tgen_subs_setn_vfmt(subs, var, -1, func, repl_fmt, ap);
  va_end(ap);
  return retval;
}

/*
  Like tgen_subs_setn(), but allows printf() formatting of the
  replacement string.

  Returns non-zero on error.
*/
int tgen_subs_setn_fmt(TGenSubs *subs, const char *var, int len,
                       TGenFun func, const char *repl_fmt, ...)
{
  int retval;
  va_list ap;
  va_start(ap, repl_fmt);
  retval = tgen_subs_setn_vfmt(subs, var, len, func, repl_fmt, ap);
  va_end(ap);
  return retval;
}


/*
  Like tgen_subs_setn(), but allows printf() formatting of the
  replacement string.

  Returns non-zero on error.
*/
int tgen_subs_setn_vfmt(TGenSubs *subs, const char *var, int len,
                        TGenFun func, const char *repl_fmt, va_list ap)
{
  int n, retval=0;
  char buf[64], *repl=buf;
  va_list ap2;
  va_copy(ap2, ap);

  /* First try to write to a stack-allocated buffer instead of allocating
     with malloc().  Resort to malloc if this fails... */
  if ((n = vsnprintf(buf, sizeof(buf), repl_fmt, ap)) < 1)
    FAIL1(TGenFormatError, "error formatting replacement string \"%s\"",
          repl_fmt);
  if (n <= (int)sizeof(buf)) {
    repl = malloc(n + 1);
    if ((vsnprintf(repl, n+1, repl_fmt, ap2)) != n)
      FAIL1(TGenFormatError, "error formatting replacement string \"%s\"",
            repl_fmt);
  }
  retval = tgen_subs_setn(subs, var, len, repl, func);

 fail:
  if (repl != buf) free(repl);
  va_end(ap2);
  return retval;
}


/*
  Initiates `dest` and copies substitutions from `src` to it.  `dest`
  should not be initiated in beforehand.

  Returns non-zero on error.  In this case, `dest` will be left in a
  non-initialised state.
 */
int tgen_subs_copy(TGenSubs *dest, const TGenSubs *src)
{
  int i, stat;
  tgen_subs_init(dest);
  for (i=0; i<src->nsubs; i++) {
    TGenSub *s = src->subs + i;
    stat = tgen_subs_set(dest, s->var, s->repl, s->func);
    if (stat) {
      tgen_subs_deinit(dest);
      return stat;
    }
  }
  return 0;
}



/***************************************************************
 * Functions for text generations
 ***************************************************************/

/*
  Returns a newly malloc'ed string based on `template`, where all
  occurences of ``{VAR}`` are replaced according to substitution `VAR`
  in the array `subs`, which has length `nsubs`.

  The template may also refer to a substitution as ``{VAR:TEMPL}``.
  If the substitution corresponding to `VAR` provide a substitution
  function (via its `subs` member), `TEMPL` will be passed as
  subtemplate to the substitution function.  If `TEMPL` is not given,
  then the subtemplate will be taken from the `repl` member of the
  corresponding substitution.

  Returns NULL, on error.
 */
char *tgen(const char *template, const TGenSubs *subs, void *context)
{
  TGenBuf s;
  tgen_buf_init(&s);
  if (tgen_append(&s, template, -1, subs, context)) {
    if (s.buf) free(s.buf);
    return NULL;
  }
  return s.buf;
}


/*
  Like tgen(), but appends to `s` instead of returning the substituted
  template.  `tlen` is the length of `template`.  If it is negative, the
  full content of `template` will be used.

  Returns non-zero on error.
 */
int tgen_append(TGenBuf *s, const char *template, int tlen,
                const TGenSubs *subs, void *context)
{
  const TGenSub *sub;
  const char *templ, *t = template;
  int templ_len, nchars, stat;

  if (tlen < 0) tlen = strlen(template);
  while (*t && t < template + tlen) {
    int len = strcspn(t, "{}");
    char *fmt = NULL;
    int casemode = 's';
    tgen_buf_append(s, t, len);
    t += len;
    if (t - template == (long)tlen) return 0;
    assert(t < template + tlen);

    switch (*(t++)) {

    case '\0':
      return 0;

    case '{':
      switch (*t) {
      case '\0':  /* unmatched opening brace */
        return err(TGenSyntaxError, "line %d: template ends with "
                   "unmatched '{'", tgen_lineno(template, t));
      case '{':  /* escaped opening brace */
        tgen_buf_append(s, "{", 1);
        t++;
        break;
      case '}':
        break;
      default:  /* substitution */
        len = strcspn(t, "%:{}");
        if (t[len] == '\0')
          return err(TGenSyntaxError, "line %d: template ends with "
                     "unmatched '{'", tgen_lineno(template, t));
        if (t[len] == '{')
          return err(TGenSyntaxError, "line %d: unexpected '{' within a "
                     "substitution", tgen_lineno(template, t));

        /* parse special constructs */
        if (strncmp(t, "@if", len) == 0) {  /* conditional */
          if ((len = builtin_if(s, t, subs, context)) < 0)
            return err(TGenSyntaxError, "line %d: invalid @if conditional",
                       tgen_lineno(template, t));
          t += len;
          continue;
        } else if (t[0] == '@' && isdigit(t[1])) {  /* alignment */
          char *endp;
          long n = strtol(t+1, &endp, 0);
          if (endp != t+len)
            return err(TGenSyntaxError, "line %d: invalid alignment tag {%.*s",
                       tgen_lineno(template, t), len, t);
          tgen_buf_align(s, n);
          t += len+1;
          continue;
        }

        /* parse VAR */
        if (!(sub = tgen_subs_getn(subs, t, len)))
          return err(TGenVariableError, "line %d: unknown var '%.*s'",
                     tgen_lineno(template, t), len, t);

        /* parse FMT */
        if (t[len] == '%') {
          char buf[10];
          const char *tt = t + len;
          int m = strcspn(tt, ":}");
          if (m >= (int)sizeof(buf))
            return err(TGenSyntaxError, "line %d: format specifier \"%.*s\" "
                       "must not exceed %zd characters",
                       tgen_lineno(template, t), m, tt, sizeof(buf)-1);
          if (tt[m] == '\0')
            return err(TGenSyntaxError, "line %d: template ends with "
                       "unmatched '{'", tgen_lineno(template, t));
          if (!validate_fmt(tt, m))
            return err(TGenSyntaxError, "line %d: invalid format specifier "
                       "\"%.*s\"", tgen_lineno(template, t), m, tt);
          len += m;
          casemode = tt[m-1];
          strncpy(buf, tt, m);
          buf[m-1] = 's';
          buf[m] = '\0';
          fmt = buf;
        }

        /* parse TEMPL */
        templ = sub->repl;
        templ_len = -1;
        if (t[len] == ':') {
          int depth = 0;
          const char *tt = t + len + 1;
          templ = tt;
          while (*tt && *tt != '}') {
            int m = strcspn(tt, "{}");
            tt += m;
            switch (*(tt++)) {
            case '\0':
              return err(TGenSyntaxError, "line %d: unterminated "
                         "subtemplate in substitution for '%s'",
                         tgen_lineno(template, t), sub->var);
            case '{':
              if (*tt == '{')
                tt++;
              else if (*tt != '}')
                depth++;
              break;

            case '}':
              if (*tt == '}')
                tt++;
              else if (depth-- <= 0)
                tt--;
              break;

            default:
              abort();
            }
          }
          t = tt;
          templ_len = tt - templ;
        }

        if (sub->func) {
          if (!templ)
            return err(TGenSubtemplateError, "line %d: subtemplate must "
                       "be provided for var '%s'", tgen_lineno(template, t),
                       sub->var);
          if ((stat = sub->func(s, templ, templ_len, subs, context)))
            return stat;
        } else if (fmt) {
          char *p = s->buf + s->pos;
          if ((nchars = tgen_buf_append_fmt(s, fmt, sub->repl)) < 0)
            return nchars;
          tgen_setcase(p, nchars, casemode);
        } else {
          if ((nchars = tgen_buf_append(s, sub->repl, -1)) < 0)
            return -1;
        }
        len = strcspn(t, "}");
        assert(t[len]);
        t += len + 1;
      }
      break;

    case '}':
      if (*t != '}')
        return err(TGenSyntaxError, "line %d: unescaped terminating brace",
                   tgen_lineno(template, t));
      tgen_buf_append(s, "}", 1);
      t++;
      break;

    default:
      abort();  /* should never be reached*/
    }
  }

  return 0;
}
