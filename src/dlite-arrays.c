#include "config.h"

#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "utils/err.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-type.h"
#include "dlite-arrays.h"


/*
  Creates a new array object.

  `data` is a pointer to array data. No copy is done.
  `type` is the type of each element.
  `size` is the size of each element.
  `ndims` is the number of dimensions.
  `dims` is the size of each dimensions.  Length: `ndims`.

  Returns the new array or NULL on error.
 */
DLiteArray *dlite_array_create(void *data, DLiteType type, size_t size,
                               int ndims, const int *dims)
{
  DLiteArray *arr;
  int i, asize = sizeof(DLiteArray) + 2*ndims*sizeof(int);
  assert(ndims >= 0);

  /* allocate the array object (except the data) in one chunk */
  if (!(arr = calloc(1, asize))) return err(1, "allocation failure"), NULL;
  arr->dims = (int *)((char *)arr + sizeof(DLiteArray));
  arr->strides = (int *)((char *)arr->dims + ndims*sizeof(int));

  arr->data = data;
  arr->type = type;
  arr->size = size;
  arr->ndims = ndims;
  memcpy(arr->dims, dims, ndims*sizeof(int));
  for (i=ndims-1; i>=0; i--) {
    arr->strides[i] = size;
    size *= dims[i];
  }
  return arr;
}


/*
  Free an array object, but not the associated data.
*/
void dlite_array_free(DLiteArray *arr)
{
  free(arr);
}


/*
  Returns the memory size in bytes of array `arr`.
 */
size_t dlite_array_size(const DLiteArray *arr)
{
  int n, size, maxsize=0;
  for (n=0; n < arr->ndims; n++)
    if ((size = arr->strides[n]*arr->dims[n]) > maxsize) maxsize = size;
  return maxsize;
}

/*
  Returns a pointer to data at index `ind`, where `ind` is an array
  of length `arr->ndims`.
*/
void *dlite_array_index(const DLiteArray *arr, int *ind)
{
  int i, offset=0;
  for (i=0; i<arr->ndims; i++) offset += ind[i] * arr->strides[i];
  return (char *)arr->data + offset;
}


#ifdef HAVE_STDARG_H
/*
  Like dlite_array_index(), but the index is provided as `arr->ndims`
  number of variable arguments of type int.
*/
void *dlite_array_vindex(const DLiteArray *arr, ...)
{
  int i, offset=0;
  va_list ap;
  va_start(ap, arr);
  for (i=0; i<arr->ndims; i++) offset += va_arg(ap, int) * arr->strides[i];
  va_end(ap);
  return (char *)arr->data + offset;
}
#endif /* HAVE_STDARG_H */


/*
  Initialise array iterator object `iter`, for iteration over array `arr`.

  Returns non-zero on error.
*/
int dlite_array_iter_init(DLiteArrayIter *iter, const DLiteArray *arr)
{
  memset(iter, 0, sizeof(DLiteArrayIter));
  iter->arr = arr;
  if (!(iter->ind = calloc(arr->ndims, sizeof(int))))
    return err(1, "allocation failure");
  iter->ind[arr->ndims-1]--;
  return 0;
}

/*
  Deinitialise array iterator object `iter`.
*/
void dlite_array_iter_deinit(DLiteArrayIter *iter)
{
  free(iter->ind);
}

/*
  Returns the next element of from array iterator, or NULL if all elements
  has been visited.
*/
void *dlite_array_iter_next(DLiteArrayIter *iter)
{
  int n;
  DLiteArray *arr = (DLiteArray *)iter->arr;
  if (iter->ind[0] < 0) return NULL;
  for (n=arr->ndims-1; n>=0; n--) {
    if (++iter->ind[n] < arr->dims[n]) break;
    iter->ind[n] = 0;
  }
  if (n < 0) {
    iter->ind[0] = -1;
    return NULL;
  }
  return dlite_array_index(arr, iter->ind);
}


/*
  Returns 1 is arrays `a` and `b` are equal, zero otherwise.
 */
int dlite_array_compare(const DLiteArray *a, const DLiteArray *b)
{
  /* check whether the array structures are equal */
  if (a->ndims != b->ndims) return 0;
  if (memcmp(a, b, sizeof(DLiteArray) + 2*a->ndims*sizeof(int))) return 0;

  /* check whether the array data are equal */
  if (memcmp(a, b, dlite_array_size(a))) return 0;
  return 1;
}


/*
  Returns a new array object representing a slice of `arr`.  `start`,
  `stop` and `step` has the same meaning as in Python and should be
  either NULL or arrays of length `arr->ndims`.

  If `start` is NULL, it will default to zero for dimensions with
  positive `step` and `arr->ndims-1` for dimensions with negative
  `step`.

  If `stop` is NULL, it will default to `arr->ndims-1` for dimensions
  with positive `step` and zero for dimensions with negative `step`.

  If `step` is NULL, it defaults to one.

  Returns NULL on error.
 */
DLiteArray *dlite_array_slice(const DLiteArray *arr,
			      int *start, int *stop, int *step)
{
  int n, offset=0;
  DLiteArray *new;

  if (!(new = dlite_array_create(arr->data, arr->type, arr->size,
				 arr->ndims, arr->dims))) return NULL;
  for (n = arr->ndims-1; n >= 0; n--) {
    int s1, s2, m;
    int d = (step) ? step[n] : 1;
    if (d == 0) return err(1, "dim %d: slice step cannot be zero", n), NULL;
    s1 = (start) ? start[n] % arr->ndims : (d > 0) ? 0 : arr->ndims-1;
    s2 = (stop) ? stop[n] % arr->ndims : (d > 0) ? arr->ndims-1 : -1;
    offset += s1 * arr->strides[n];
    m = (s2 - s1) / d;
    new->dims[n] = (m >= 0) ? m : -m;
    new->strides[n] *= d;
  }
  new->data = ((char *)arr->data) + offset;
  return new;
}


/*
  Print array `arr` to stream `fp`.  Returns non-zero on error.
 */
int dlite_array_printf(FILE *fp, const DLiteArray *arr)
{
  void *p;
  DLiteArrayIter iter;
  char buf[80];
  dlite_array_iter_init(&iter, arr);
  for (n=arr->ndims-1; n>=0; n--) {
    if (++iter->ind[n] < arr->dims[n]) break;
    iter->ind[n] = 0;
  }



  while ((p = dlite_array_iter_next(&iter))) {
    dlite_type_snprintf(p, arr->type, arr->size, buf, sizeof(buf));
    fprintf(fp, "%s : %d %d\n", buf, iter.ind[0], iter.ind[1]);
  }
  dlite_array_iter_deinit(&iter);
  return 0;
}
