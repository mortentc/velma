# Using C Programs to Import Files as Arrow Objects
In case you are restricted to using C for constructing Arrow objects (as is the case for computational storage),
the Arrow object has designed a [data interface](https://arrow.apache.org/docs/format/CDataInterface.html) for such a case. The data interface consists of two C structs, `ArrowSchema` and `ArrowArray`, which can be passed to Arrows C++ code and [imported](https://arrow.apache.org/docs/cpp/api/c_abi.html) to create equivalent Arrow Arrays or RecordBatches.
```C
#ifndef ARROW_C_DATA_INTERFACE
#define ARROW_C_DATA_INTERFACE

#define ARROW_FLAG_DICTIONARY_ORDERED 1
#define ARROW_FLAG_NULLABLE 2
#define ARROW_FLAG_MAP_KEYS_SORTED 4

struct ArrowSchema {
  // Array type description
  const char* format;
  const char* name;
  const char* metadata;
  int64_t flags;
  int64_t n_children;
  struct ArrowSchema** children;
  struct ArrowSchema* dictionary;

  // Release callback
  void (*release)(struct ArrowSchema*);
  // Opaque producer-specific data
  void* private_data;
};

struct ArrowArray {
  // Array data description
  int64_t length;
  int64_t null_count;
  int64_t offset;
  int64_t n_buffers;
  int64_t n_children;
  const void** buffers;
  struct ArrowArray** children;
  struct ArrowArray* dictionary;

  // Release callback
  void (*release)(struct ArrowArray*);
  // Opaque producer-specific data
  void* private_data;
};

#endif  // ARROW_C_DATA_INTERFACE
```

The `ArrowSchema` struct is responsible for describing the type of content of the `ArrowArray`. This is primarily done using the `format`-string field, by using letters to encode type information according to [this](https://arrow.apache.org/docs/format/CDataInterface.html#data-type-description-format-strings) table. The possible encodings corresponds to the logical types described in the [Arrow Columnar Format](https://arrow.apache.org/docs/format/Columnar.html#physical-memory-layout). This format also describes the necessary information the `ArrowArray` should provide, in addition to the raw data values.

The `buffers` of the `ArrowArray` contain not only the data of the array, but also information like validity bitmaps and offsets. The `buffers` must be structured based on the type of the array, according to [this table](https://arrow.apache.org/docs/format/Columnar.html#buffer-listing-for-each-layout).

Additionally, both structs need a [callback procedure](https://arrow.apache.org/docs/format/CDataInterface.html#memory-management) to release any `malloc`'ed buffers, as the data is transferred via `memcpy` on the C++ side, so memory on the C side will have to be freed after the transfer is completed. The callback procedure should set the `release` of a struct to `NULL` once the release is complete.

## Arrow objects representing Daphne objects
### DenseMatrix
The `DenseMatrix` of Daphne is the simplest structure, and simply corresponds to an Arrow [Array](https://arrow.apache.org/docs/cpp/api/array.html) with a [fixed-size primitive layout](https://arrow.apache.org/docs/format/Columnar.html#fixed-size-primitive-layout). To create such an object, invoke [`ImportArray`](https://arrow.apache.org/docs/cpp/api/c_abi.html#_CPPv411ImportArrayP10ArrowArrayP11ArrowSchema), where the `format` of the `ArrowSchema` is set to the letter corresponding to the value type of the `DenseMatrix`.

### CSRMatrix
The `CSRMatrix` does not have an obviously equivalent Arrow structure, as the matrix contains two additional arrays of meta-data. This meta-data does not easily fit into any of the relevant Arrow fields, so we instead choose to treat it the same as the regular data. For that purpose, modelling a `CSRMatrix` can be done using a `Union`-type to mix the value data with the index data.

Using an object of type `DenseUnion<indices: List<uint64>, values: List<VT>>` of exactly 3 elements, we can include column indices and row offsets as the `indices`-type, and the actual matrix elements as a single instance of the `values`-type.

The Arrow object is imported using the same `ImportArray` as for the `DenseMatrix`, but the `ArrowSchema` is a nested structure with three levels. At the top level, a single schema with format `"+ud:0,1"` is used. It has two children, each with format `"+l"` to indicate a list-type. Each child also has its own child, indicating the element type of the list (so `"L"` for the indices, and the relevant character for the value type).

The buffers of the union array must consist of type ids and element offsets.
The format-string `"ud:0,1"` means that 0 is the type id of the first type (indices), and 1 is the id of the second type (values). If the layout of the `CSRMatrix` is structured as: `[row_offsets; col_ids; vals]`, the buffers should be `[0;0;1]` for the type ids, and `[0;1;0]` for the element offsets. The actual values for the indices and the matrix elements are stored as child arrays, using the [variable-size list layout](https://arrow.apache.org/docs/format/Columnar.html#variable-size-list-layout).

### Frame
The Daphne `Frame` type logically corresponds to Arrows [`Record Batch`](https://arrow.apache.org/docs/cpp/tables.html#record-batches). A `Record Batch` is a two-dimensional dataset, where a `Schema` consisting of list of `Fields`, associates a name and a type with each column of the dataset. ![](https://arrow.apache.org/docs/_images/tables-versus-record-batches.svg)

Take care not to confuse `ArrowSchema` and `Schema`, as they serve very different purposes. The `ArrowSchema` is specific to C and is used to describe *any* Arrow type. The `Schema` is only used for describing the names and types of columns in an Arrow `Table` or `Record Batch`.

`Record Batches` are not one of the logical types described by the columnar format, but it corresponds to a [`Struct`-array](https://arrow.apache.org/docs/format/Columnar.html#struct-layout), with the type of the array being `Struct<column1_type, column2_type, ..., columnN_type>`. Viewed logically, each element of the `Struct`-array corresponds to a row in the `Record Batch`, but physically, the values of each column is stored together in the same child array. The layout of the `ArrowArray` in C must match this specification, so each column have to be its own `ArrowArray`, and must be stored as a `child` in the base structure.

To create a `Record Batch`, invoke [`ImportRecordBatch`](https://arrow.apache.org/docs/cpp/api/c_abi.html#_CPPv417ImportRecordBatchP10ArrowArrayP11ArrowSchema) where the `format` of the `ArrowSchema` is set to `"+s"`. This implies the imported array is a `Struct`-array. The names and types of each column should be stored as `children` in the base `ArrowSchema`, by creating a separate `ArrowSchema` for each column.