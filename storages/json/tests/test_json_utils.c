#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "json-utils.h"
#include "str.h"

#include "config.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

/* Default path to json input file. */
char *jsonfile1 = STRINGIFY(DLITE_ROOT) "/src/tests/array.json";
char *jsonfile2 = STRINGIFY(DLITE_ROOT) "/tools/tests/Chemistry-0.1.json";

MU_TEST(test_vector)
{
  ivec_t *v = ivec();
  ivec_add(v, 1);
  ivec_add(v, 2);
  ivec_add(v, 3);
  mu_check(ivec_size(v) == 3);
  mu_check(v->capacity == 10);
  mu_check(v->data[0] == 1);
  mu_check(v->data[1] == 2);
  mu_check(v->data[2] == 3);
  ivec_fill(v, 5);
  mu_check(v->data[0] == 5);
  mu_check(v->data[1] == 5);
  mu_check(v->data[2] == 5);
  ivec_free(v);
}


MU_TEST(test_json_array)
{
  json_error_t error;
  ivec_t *dims = NULL;
  json_data_t *data = NULL;

  json_t *root = json_load_file(jsonfile1, 0, &error);
  mu_check(json_char_type(root) == 'o');
  mu_check(json_array_type(json_object_get(root, "i1")) == 'i');
  mu_check(json_array_type(json_object_get(root, "i2")) == 'i');

  mu_check(json_array_type(json_object_get(root, "s1")) == 's');

  mu_check(json_array_type(json_object_get(root, "r1")) == 'm');
  mu_check(json_array_type(json_object_get(root, "r2")) == 'r');
  mu_check(json_array_type(json_object_get(root, "r3")) == 'r');
  mu_check(json_array_type(json_object_get(root, "r4")) == 'r');
  mu_check(json_array_type(json_object_get(root, "r5")) == 'x');

  mu_check(json_array_type(json_object_get(root, "o1")) == 'o');

  dims = json_array_dimensions(json_object_get(root, "r4"));
  mu_check(dims->size == 2);
  mu_check(dims->data[0] == 3);
  mu_check(dims->data[1] == 3);
  ivec_free(dims);

  dims = json_array_dimensions(json_object_get(root, "i1"));
  mu_check(dims->size == 1);
  mu_check(dims->data[0] == 9);
  ivec_free(dims);

  dims = json_array_dimensions(json_object_get(root, "i2"));
  mu_check(dims->size == 2);
  mu_check(dims->data[0] == 5);
  mu_check(dims->data[1] == 3);
  ivec_free(dims);

  dims = json_array_dimensions(json_object_get(root, "i3"));
  mu_check(dims == NULL);

  data = json_get_data(json_object_get(root, "i1"));
  /*ivec_print(data->array_i, "i1");*/
  mu_check(data->dtype == 'i');
  mu_check(data->array_i->size == 9);
  mu_check(data->array_i->data[0] == 1);
  mu_check(data->array_i->data[2] == 3);
  mu_check(data->array_i->data[3] == 1);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "i2"));
  /*ivec_print(data->array_i, "i2");*/
  mu_check(data->dtype == 'i');
  mu_check(data->array_i->size == 15);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "r4"));
  /*vec_print(data->array_r, "r4");*/
  mu_check(data->dtype == 'r');
  mu_check(data->array_r->size == 9);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-int"));
  mu_check(data->dtype == 'i');
  mu_check(data->dims == NULL);
  mu_check(data->array_i->data[0] == 1);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-real"));
  mu_check(data->dtype == 'r');
  mu_check(data->dims == NULL);
  mu_check(data->array_r->data[0] == 2.0);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-true"));
  mu_check(data->dtype == 'b');
  mu_check(data->dims == NULL);
  mu_check(data->array_i->data[0] == 1);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-false"));
  mu_check(data->dtype == 'b');
  mu_check(data->dims == NULL);
  mu_check(data->array_i->data[0] == 0);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-str"));
  mu_check(data->dtype == 's');
  mu_check(data->dims == NULL);
  mu_check(str_equal(data->array_s->data[0], "Hello"));
  str_list_free(data->array_s, true);
  data->array_s = NULL;
  json_data_free(data);

  data = json_get_data(json_object_get(root, "s1"));
  /*str_list_print(data->array_s, "s1");*/
  mu_check(data->dtype == 's');
  mu_check(str_list_size(data->array_s) == 3);
  mu_check(str_equal(data->array_s->data[0], "a"));
  mu_check(str_equal(data->array_s->data[1], "ab"));
  mu_check(str_equal(data->array_s->data[2], "abc"));
  str_list_free(data->array_s, true);
  data->array_s = NULL;
  json_data_free(data);

  data = json_get_data(json_object_get(root, "s2"));
  /*str_list_print(data->array_s, "s2");*/
  mu_check(data->dtype == 's');
  mu_check(str_list_size(data->array_s) == 6);
  mu_check(str_equal(data->array_s->data[0], "a"));
  mu_check(str_equal(data->array_s->data[1], "ab"));
  mu_check(str_equal(data->array_s->data[2], "abc"));
  mu_check(str_equal(data->array_s->data[3], "d"));
  mu_check(str_equal(data->array_s->data[4], "de"));
  mu_check(str_equal(data->array_s->data[5], "def"));
  str_list_free(data->array_s, true);
  data->array_s = NULL;
  json_data_free(data);

  json_decref(root);
}

MU_TEST(test_json_entity)
{
  json_error_t error;
  json_t *dims;
  json_t *prop;

  json_t *root = json_load_file(jsonfile2, 0, &error);
  mu_check(json_is_object(root));

  dims = json_object_get(root, "dimensions");
  prop = json_object_get(root, "properties");

  mu_check(check_dimensions("alloy", json_array_get(prop, 0), dims) == 1);
  mu_check(check_dimensions("elements", json_array_get(prop, 1), dims) == 1);
  //mu_assert_int_eq(2, dlite_json_entity_dim_count(root));
  //mu_assert_int_eq(8, dlite_json_entity_prop_count(root));

  json_decref(root);
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_vector);
  MU_RUN_TEST(test_json_array);
  MU_RUN_TEST(test_json_entity);
}



int main(int argc, char *argv[])
{
  if (argc > 1) jsonfile2 = argv[1];

  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
