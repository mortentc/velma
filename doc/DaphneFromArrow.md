# Using C Programs to Import Files as Arrow Objects
In case you are restricted to using C for constructing Arrow objects (as is the case for computational storage),
the Arrow object has designed a [data interface](https://arrow.apache.org/docs/format/CDataInterface.html) for such a case. The data interface consists of two C structs, `ArrowSchema` and `ArrowArray`, which can be passed to Arrows C++ code and [imported](https://arrow.apache.org/docs/cpp/api/c_abi.html) to create equivalent Arrow Arrays or RecordBatches.
```
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

Additionally, both structs need a [callback procedure](https://arrow.apache.org/docs/format/CDataInterface.html#memory-management) to release any `malloc`'ed buffers, as the data is transferred via `memcpy` on the C++ side, so memory on the C side will have to be freed after the transfer is completed. The callback procedure should set the `release` of a struct to `NULL` once the release is complete.

## Arrow objects representing Daphne objects
### DenseMatrix
The `DenseMatrix` of Daphne is the simplest structure, and simply corresponds to an Arrow [Array](https://arrow.apache.org/docs/cpp/api/array.html) with a [fixed-size primitive layout](https://arrow.apache.org/docs/format/Columnar.html#fixed-size-primitive-layout). To create such an object, invoke [`ImportArray`](https://arrow.apache.org/docs/cpp/api/c_abi.html#_CPPv411ImportArrayP10ArrowArrayP11ArrowSchema), where the `format` of the `ArrowSchema` is set to the letter corresponding to the value type of the `DenseMatrix`.

### CSRMatrix
TBD

### Frame
The Daphne `Frame` type logically corresponds to Arrows [`Record Batch`](https://arrow.apache.org/docs/cpp/tables.html#record-batches). A `Record Batch` is a two-dimensional dataset, where a `Schema` consisting of list of `Fields`, associates a name and a type with each column of the dataset. Take care not to confuse `ArrowSchema` and `Schema`, as they serve very different purposes. The `ArrowSchema` is specific to C and is used to describe *any* Arrow type. The `Schema` is only used for describing the names and types of columns in an Arrow `Table` or `Record Batch`.

`Record Batches` are not one of the logical types described by the columnar format, but it corresponds to a [`Struct`-array](https://arrow.apache.org/docs/format/Columnar.html#struct-layout), with the type of the array being `Struct<column1_type, column2_type, ..., columnN_type>`. Viewed logically, each element of the `Struct`-array corresponds to a row in the `Record Batch`, but physically, the values of each column is stored together in the same child array. The layout of the `ArrowArray` in C must match this specification, so each column have to be its own `ArrowArray`, and must be stored as a `child` in the base structure.

To create a `Record Batch`, invoke [`ImportRecordBatch`](https://arrow.apache.org/docs/cpp/api/c_abi.html#_CPPv417ImportRecordBatchP10ArrowArrayP11ArrowSchema) where the `format` of the `ArrowSchema` is set to `"+s"`. This implies the imported array is a `Struct`-array. The names and types of each column should be stored as `children` in the base `ArrowSchema`, by creating a separate `ArrowSchema` for each column.