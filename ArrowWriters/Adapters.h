#include <arrow-glib/array-builder.h>
#include <arrow-glib/arrow-glib.h>

enum value_type {
    int8, uint8,
    int16, uint16,
    int32, uint32,
    int64, uint64
};

typedef GArrowTensor ArrowDenseMatrix;

typedef struct {
    GArrowArray values;
    GArrowUInt64Array colIdxs;
    GArrowUInt64Array rowOffsets;
} ArrowCSRMatrix;

GArrowArrayBuilder *constructCSR(enum value_type vt){
    GArrowArrayBuilder *builder;
    switch(vt){
        case int8: builder =
        (GArrowArrayBuilder *)garrow_int8_array_builder_new();
    }
    return builder;
}