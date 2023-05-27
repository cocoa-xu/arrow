#include <erl_nif.h>
#include <stdbool.h>
#include <stdio.h>
#include <climits>
#include <type_traits>
#include "nif_utils.hpp"
#include "arrow/c_data_interface.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

struct NifResArrowData {
  struct ArrowSchema * schema;
  struct ArrowArray * array;

  static ErlNifResourceType * type;
  static NifResArrowData * allocate_resource(ErlNifEnv * env, ERL_NIF_TERM &error) {
    NifResArrowData * res = (NifResArrowData *)enif_alloc_resource(NifResArrowData::type, sizeof(NifResArrowData));
    if (res == nullptr) {
      error = erlang::nif::error(env, "cannot allocate NifResArrowData resource");
      return res;
    }

    return res;
  }

  static NifResArrowData * get_resource(ErlNifEnv * env, ERL_NIF_TERM term, ERL_NIF_TERM &error) {
    NifResArrowData * self_res = nullptr;
    if (!enif_get_resource(env, term, NifResArrowData::type, (void **)&self_res) || self_res == nullptr || self_res->schema == nullptr || self_res->array == nullptr) {
        error = erlang::nif::error(env, "cannot access NifResArrowData resource");
    }
    return self_res;
  }

  static void destruct_resource(ErlNifEnv *env, void *args) {
    auto res = (NifResArrowData *)args;
    if (res) {
      if (res->schema) {
        if (res->schema->release) {
          res->schema->release(res->schema);
        }
        enif_free(res->schema);
        res->schema = nullptr;
      }

      if (res->array) {
        if (res->array->release) {
          res->array->release(res->array);
        }
        enif_free(res->array);
        res->array = nullptr;
      }
    }
  }
};

ErlNifResourceType * NifResArrowData::type = nullptr;

static void release_schema(struct ArrowSchema* schema) {
  if (schema->release == NULL) return;
  schema->release = NULL;

  int i;
  for (i = 0; i < schema->n_children; ++i) {
    struct ArrowSchema* child = schema->children[i];
    if (child->release != NULL) {
        child->release(child);
    }
  }
  free(schema->children);
}

static void release_array(struct ArrowArray* array) {
  if (array->release == NULL) return;
  array->release = NULL;

  int i;
  for (i = 0; i < array->n_children; ++i) {
    struct ArrowArray* child = array->children[i];
    if (child->release != NULL) {
        child->release(child);
    }
  }
  enif_free(array->children);

  for (i = 0; i < array->n_buffers; ++i) {
    enif_free((void *)array->buffers[i]);
  }
  enif_free(array->buffers);
}

static uint8_t * allocate_bitmap(size_t count, bool init_val = 0) {
  if ((count & 0b111) > 0) {
      count += 8;
  }
  size_t bytes = count >> 3;
  if (bytes < 8) {
      bytes = 8;
  }
  uint8_t * bitmap = nullptr;
  if ((bitmap = (uint8_t *)enif_alloc(bytes)) == nullptr) {
    return nullptr;
  }

  memset(bitmap, init_val, bytes);
  return bitmap;
}

static int32_t * allocate_offsets(size_t count) {
  int32_t * offsets = nullptr;
  if ((offsets = (int32_t *)enif_alloc(sizeof(int32_t) * count)) == nullptr) {
    return nullptr;
  }

  return offsets;
}

template <typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
bool export_array(const T* data, const uint8_t* bitmap, int64_t nitems, int64_t null_count, struct ArrowArray* array) {
  // Initialize primitive fields
  *array = (struct ArrowArray) {
    // Data description
    .length = nitems,
    .offset = 0,
    .null_count = null_count,
    .n_buffers = 2,
    .n_children = 0,
    .children = NULL,
    .dictionary = NULL,
    .release = &release_array
  };

  array->buffers = (const void **)enif_alloc(sizeof(void *) * array->n_buffers);
  if (array->buffers == nullptr) {
    return false;
  }

  array->buffers[0] = bitmap;
  array->buffers[1] = data;
  return true;
}

static ERL_NIF_TERM arrow_int64_example(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM ret, error;
  const size_t example_nitems = 4;
  const size_t example_nulls = 1;

  NifResArrowData * data = nullptr;
  if ((data = NifResArrowData::allocate_resource(env, error)) == nullptr) {
    return error;
  }

  data->schema = (struct ArrowSchema *)enif_alloc(sizeof(struct ArrowSchema));
  if (data->schema == nullptr) {
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  data->array = (struct ArrowArray *)enif_alloc(sizeof(struct ArrowArray));
  if (data->array == nullptr) {
    enif_free(data->schema);
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  *data->schema = (struct ArrowSchema) {
    // Type description
    .format = "+w:4",
    .name = "",
    .metadata = NULL,
    .flags = ARROW_FLAG_NULLABLE,
    .n_children = 0,
    .children = NULL,
    .dictionary = NULL,
    .release = &release_schema
  };

  uint8_t * bitmap = allocate_bitmap(example_nitems);
  if (bitmap == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  bitmap[0] |= 1 << 0;
  bitmap[0] |= 1 << 1;
  bitmap[0] |= 1 << 3;

  int64_t * arr = (int64_t *)enif_alloc(sizeof(int64_t *) * example_nitems);
  if (arr == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_release_resource(data);
    enif_free(bitmap);
    return erlang::nif::error(env, "out of memory");
  }

  arr[0] = 1;
  arr[1] = 2;
  arr[3] = 4;

  if (!export_array(arr, bitmap, example_nitems, example_nulls, (struct ArrowArray *)data->array)) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_release_resource(data);
    enif_free(bitmap);
    enif_free(arr);
    return erlang::nif::error(env, "out of memory");
  }

  ERL_NIF_TERM data_res = enif_make_resource(env, data);
  enif_release_resource(data);

  return erlang::nif::ok(env, data_res);
}

static ERL_NIF_TERM arrow_utf8_example(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM ret, error;
  const size_t example_nitems = 4;
  const size_t example_nulls = 1;

  NifResArrowData * data = nullptr;
  if ((data = NifResArrowData::allocate_resource(env, error)) == nullptr) {
    return error;
  }

  data->schema = (struct ArrowSchema *)enif_alloc(sizeof(struct ArrowSchema));
  if (data->schema == nullptr) {
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  data->array = (struct ArrowArray *)enif_alloc(sizeof(struct ArrowArray));
  if (data->array == nullptr) {
    enif_free(data->schema);
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  *data->schema = (struct ArrowSchema) {
    // Type description
    .format = "+l",
    .name = "",
    .metadata = NULL,
    .flags = ARROW_FLAG_NULLABLE,
    .n_children = 1,
    .children = NULL,
    .dictionary = NULL,
    .release = &release_schema
  };
  // Allocate list of children types
  data->schema->children = (struct ArrowSchema **)enif_alloc(sizeof(struct ArrowSchema *) * data->schema->n_children);
  if (data->schema->children == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  struct ArrowSchema * utf8_schema;
  utf8_schema = data->schema->children[0] = (struct ArrowSchema *)enif_alloc(sizeof(struct ArrowSchema));
  if (utf8_schema == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_free(data->schema->children);
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  *utf8_schema = (struct ArrowSchema) {
    // Type description
    .format = "u",
    .name = "",
    .metadata = NULL,
    .flags = ARROW_FLAG_NULLABLE,
    .n_children = 0,
    .dictionary = NULL,
    .children = NULL,
    .release = &release_schema
  };

  uint8_t * bitmap = allocate_bitmap(example_nitems);
  if (bitmap == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_free(data->schema->children[0]);
    enif_free(data->schema->children);
    enif_release_resource(data);
    return erlang::nif::error(env, "out of memory");
  }

  bitmap[0] |= 1 << 0;
  bitmap[0] |= 1 << 1;
  bitmap[0] |= 1 << 3;

  int32_t * offsets = allocate_offsets(example_nitems);
  if (offsets == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_free(data->schema->children[0]);
    enif_free(data->schema->children);
    enif_release_resource(data);
    enif_free(bitmap);
    return erlang::nif::error(env, "out of memory");
  }
  offsets[0] = 0;
  offsets[1] = 3;
  offsets[2] = 3;
  offsets[3] = 6;

  uint8_t p[] = "foobarbatman";
  int64_t length = sizeof(p);
  uint8_t * arr = (uint8_t *)enif_alloc(length);
  if (arr == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_free(data->schema->children[0]);
    enif_free(data->schema->children);
    enif_release_resource(data);
    enif_free(bitmap);
    enif_free(offsets);
    return erlang::nif::error(env, "out of memory");
  }

  memcpy(arr, p, sizeof(p));

  struct ArrowArray * child_array = (struct ArrowArray *)data->array;
  *child_array = (struct ArrowArray) {
    // Data description
    .length = length,
    .offset = 0,
    .null_count = example_nulls,
    .n_buffers = 3,
    .n_children = 0,
    .children = NULL,
    .dictionary = NULL,
    .release = &release_array
  };

  child_array->buffers = (const void **)enif_alloc(sizeof(void *) * child_array->n_buffers);
  if (child_array->buffers == nullptr) {
    enif_free(data->schema);
    enif_free(data->array);
    enif_free(data->schema->children[0]);
    enif_free(data->schema->children);
    enif_release_resource(data);
    enif_free(bitmap);
    enif_free(offsets);
    enif_free(arr);
    return erlang::nif::error(env, "out of memory");
  }

  child_array->buffers[0] = bitmap;
  child_array->buffers[1] = offsets;
  child_array->buffers[2] = arr;

  ERL_NIF_TERM data_res = enif_make_resource(env, data);
  enif_release_resource(data);

  return erlang::nif::ok(env, data_res);
}

static ERL_NIF_TERM arrow_to_arrow_c_data(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM error;
  NifResArrowData * data = nullptr;

  if ((data = NifResArrowData::get_resource(env, argv[0], error)) == nullptr) {
    return enif_make_badarg(env);
  }

  ERL_NIF_TERM schema_bin = enif_make_resource_binary(env, data, data->schema, sizeof(struct ArrowSchema));
  ERL_NIF_TERM array_bin = enif_make_resource_binary(env, data, data->array, sizeof(struct ArrowArray));

  return enif_make_tuple3(
    env,
    erlang::nif::ok(env),
    schema_bin,
    array_bin
  );
}

static ERL_NIF_TERM nif_error_from_adbc_error(ErlNifEnv *env, struct AdbcError * error) {
    return erlang::nif::error(env, enif_make_tuple3(env,
        erlang::nif::make_binary(env, error->message),
        enif_make_int(env, error->vendor_code),
        erlang::nif::make_binary(env, error->sqlstate, 5)
    ));
}

static ERL_NIF_TERM arrow_execute_query_example(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM ret;

  uint64_t ptr_u64;
  if (!erlang::nif::get(env, argv[0], &ptr_u64)) {
    return enif_make_badarg(env);
  }
  if (ptr_u64 == 0) return enif_make_badarg(env);

  uint64_t stmt_ptr_u64;
  if (!erlang::nif::get(env, argv[1], &stmt_ptr_u64)) {
    return enif_make_badarg(env);
  }
  if (stmt_ptr_u64 == 0) return enif_make_badarg(env);

  uint64_t array_stream_ptr_u64;
  if (!erlang::nif::get(env, argv[2], &array_stream_ptr_u64)) {
    return enif_make_badarg(env);
  }
  if (array_stream_ptr_u64 == 0) return enif_make_badarg(env);

  uint64_t error_ptr_u64;
  if (!erlang::nif::get(env, argv[3], &error_ptr_u64)) {
    return enif_make_badarg(env);
  }
  if (error_ptr_u64 == 0) return enif_make_badarg(env);

  AdbcStatusCode(*remote_AdbcStatementExecuteQuery)(void *, void *, int64_t*, void *) = nullptr;
  remote_AdbcStatementExecuteQuery = reinterpret_cast<decltype(remote_AdbcStatementExecuteQuery)>(ptr_u64);
  
  int64_t rows_affected;

  struct AdbcError * adbc_error = (struct AdbcError *)(uint64_t *)error_ptr_u64;
  memset(adbc_error, 0, sizeof(struct AdbcError));

  AdbcStatusCode code = remote_AdbcStatementExecuteQuery(
    (void *)(uint64_t *)stmt_ptr_u64, 
    (void *)(uint64_t *)array_stream_ptr_u64, 
    &rows_affected,
    adbc_error
  );
  if (code != ADBC_STATUS_OK) {
    ret = nif_error_from_adbc_error(env, adbc_error);
    if (adbc_error->release != nullptr) {
      adbc_error->release(adbc_error);
    }
    return ret;
  }

  return enif_make_tuple3(env,
    erlang::nif::ok(env),
    argv[2],
    enif_make_int64(env, rows_affected)
  );
}

static int on_load(ErlNifEnv *env, void **, ERL_NIF_TERM) {
  ErlNifResourceType *rt;

  rt = enif_open_resource_type(env, "Elixir.Arrow.Nif", "NifResArrowData", NifResArrowData::destruct_resource, ERL_NIF_RT_CREATE, NULL);
  if (!rt) return -1;
  NifResArrowData::type = rt;
  
  return 0;
}

static int on_reload(ErlNifEnv *, void **, ERL_NIF_TERM) {
  return 0;
}

static int on_upgrade(ErlNifEnv *, void **, void **, ERL_NIF_TERM) {
  return 0;
}

static ErlNifFunc nif_functions[] = {
  {"arrow_execute_query_example", 5, arrow_execute_query_example, 0}
};

ERL_NIF_INIT(Elixir.Arrow.Nif, nif_functions, on_load, on_reload, on_upgrade, NULL);

#if defined(__GNUC__)
#pragma GCC visibility push(default)
#endif
