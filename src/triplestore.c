#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "compat.h"
#include "err.h"
#include "sha1.h"
#include "triplestore.h"


/* Allocate triplestore memory in chunks of TRIPLESTORE_BUFFSIZE */
#define TRIPLESTORE_BUFFSIZE 1024



/* ==============================================================
 * Private functions
 * ============================================================== */

static void xtriplet_clean(XTriplet *t)
{
  triplet_clean((Triplet *)t);
  free(t->id);
}


/* ==============================================================
 * Functions defined in the header
 * ============================================================== */

/*
  Frees up memory used by the s-p-o strings, but not the triplet itself.
*/
void triplet_clean(Triplet *t)
{
  free(t->s);
  free(t->p);
  free(t->o);
}

/*
  Convinient function to assign a triplet.
 */
int triplet_set(Triplet *t, const char *s, const char *p, const char *o)
{
  t->s = strdup(s);
  t->p = strdup(p);
  t->o = strdup(o);
  return 0;
}

/*
  Returns an newly malloc'ed hash string calculated from triplet.
*/
char *triplet_get_id(const Triplet *t)
{
  SHA1_CTX context;
  unsigned char digest[20];
  char *id;
  int i;
  SHA1Init(&context);
  SHA1Update(&context, (unsigned char *)t->s, strlen(t->s));
  SHA1Update(&context, (unsigned char *)t->p, strlen(t->p));
  SHA1Update(&context, (unsigned char *)t->o, strlen(t->o));
  SHA1Final(digest, &context);
  if (!(id = malloc(41))) return NULL;
  for (i=0; i<20; i++)
    sprintf(id + 2*i, "%02x", digest[i]);
  return id;
}

/*
  Returns a new empty triplestore or NULL on error.
 */
Triplestore *triplestore_create()
{
  Triplestore *store = calloc(1, sizeof(Triplestore));
  return store;
}


/*
  Frees triplestore `ts`.
 */
void triplestore_free(Triplestore *store)
{
  size_t i;
  for (i=0; i<store->length; i++)
    xtriplet_clean(store->triplets + i);
  if (store->triplets) free(store->triplets);
  free(store);
}


/*
  Returns the number of triplets in the store.
*/
size_t triplestore_length(Triplestore *store)
{
  return store->length;
}


/* Compare triplets (in s-o-p order) */
static int compar(const void *p1, const void *p2)
{
  int v;
  Triplet *t1 = (Triplet *)p1;
  Triplet *t2 = (Triplet *)p2;
  if ((v = strcmp(t1->s, t2->s))) return v;
  if ((v = strcmp(t1->o, t2->o))) return v;
  return strcmp(t1->p, t2->p);
}


/*
  Adds a single triplet to store.  Returns non-zero on error.
 */
int triplestore_add(Triplestore *store, const char *s, const char *p,
                    const char *o)
{
  Triplet t;
  t.s = (char *)s;
  t.p = (char *)p;
  t.o = (char *)o;
  return triplestore_add_triplets(store, &t, 1);
}


/*
  Adds `n` triplets to store.  Returns non-zero on error.
 */
int triplestore_add_triplets(Triplestore *store, const Triplet *triplets,
                             size_t n)
{
  size_t i;
  XTriplet *t;

  if (store->size < store->length + n) {
    size_t m = (store->length + n - store->size) / TRIPLESTORE_BUFFSIZE;
    store->size += (m + 1) * TRIPLESTORE_BUFFSIZE;
    assert(store->size >= store->length + n);
    store->triplets = realloc(store->triplets, store->size * sizeof(XTriplet));
    memset(store->triplets + store->length, 0,
           (store->size - store->length)*sizeof(XTriplet));
  }

  /* append triplets */
  t = store->triplets + store->length;
  for (i=0; i<n; i++, t++) {
    t->s = strdup(triplets[i].s);
    t->p = strdup(triplets[i].p);
    t->o = strdup(triplets[i].o);
    t->id = triplet_get_id(triplets + i);
  }
  store->length += n;

  /* sort */
  t = store->triplets;
  qsort(t, store->length, sizeof(XTriplet), compar);

  /* remove dublicates */
  for (i=1; i<store->length; i++) {
    if (compar(&t[i], &t[i-1]) == 0) {
      while (i < store->length && compar(&t[store->length - 1], &t[i-1]) == 0)
        xtriplet_clean(t + --store->length);
      if (i < store->length) {
        xtriplet_clean(t + i);
        t[i] = t[--store->length];
      }
    }
  }

  return 0;
}


/* Removes triplet number n. */
static int _remove(Triplestore *store, size_t n)
{
  XTriplet *t = store->triplets;
  if (n >= store->length)
    return err(1, "invalid triplet index: %lu", n);
  xtriplet_clean(t + n);
  memcpy(t + n, t + --store->length, sizeof(XTriplet));
  return 0;
}

/*
  Removes a triplet identified by it's id.  Returns non-zero if no such
  triplet can be found.
*/
int triplestore_remove_by_id(Triplestore *store, const char *id)
{
  size_t i;
  for (i=0; i<store->length; i++)
    if (strcmp(store->triplets[i].id, id) == 0)
      return _remove(store, i);
  return 1;
}

/*
  Removes a triplet identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triplets removed.
*/
int triplestore_remove(Triplestore *store, const char *s,
                       const char *p, const char *o)
{
  size_t i=0, n=0;
  while (i < store->length) {
    const XTriplet *t = store->triplets + i;
    if ((!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0)) {
      if (_remove(store, i) == 0) n++;
    }
    i++;
  }
  return n;
}


/*
  Returns a pointer to triplet with given id or NULL if no match can be found.
*/
const Triplet *triplestore_get_id(const Triplestore *store, const char *id)
{
  size_t i;
  for (i=0; i<store->length; i++)
    if (strcmp(store->triplets[i].id, id) == 0)
      return (Triplet *)&store->triplets[i];
  return NULL;
}


/*
  Returns a pointer to first triplet matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triplet *triplestore_find_first(const Triplestore *store, const char *s,
                                      const char *p, const char *o)
{
  size_t i;
  for (i=0; i<store->length; i++) {
    const XTriplet *t = store->triplets + i;
    if ((!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0))
      return (Triplet *)t;
  }
  return NULL;
}


/*
  Initiates a TripleState for triplestore_find().
*/
void triplestore_init_state(const Triplestore *store, TripleState *state)
{
  (void)store;  /* unused */
  state->pos = 0;
}


/*
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triplet matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.
 */
const Triplet *triplestore_find(const Triplestore *store, TripleState *state,
                       const char *s, const char *p, const char *o)
{
  while (state->pos < store->length) {
    const XTriplet *t = store->triplets + state->pos++;
    if ((!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0))
      return (Triplet *)t;
  }
  return NULL;
}