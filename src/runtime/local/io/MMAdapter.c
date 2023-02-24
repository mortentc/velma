#include <runtime/local/datastructures/CArrowStructures.h>
#include <runtime/local/io/MatrixMarket.h>
#include <runtime/local/io/MMAdapter.h>
#include <stdlib.h>

void ReleaseDenseType(struct ArrowSchema* schema){
    schema->release = NULL;
}

void ReleaseDenseData(struct ArrowArray *data){
    free((void *) data->buffers[1]);
    free(data->buffers);
    data->release=NULL;
}

void CreateDenseMatrix(const char *fname, struct ArrowSchema *type_info, struct ArrowArray *data){
    int row, col, nz, *xs, *ys;
    double *val;
    MM_typecode code;

    mm_read_mtx_crd(fname, &row, &col, &nz, &xs, &ys, &val, &code);

    free(xs);
    free(ys);

    *type_info = (struct ArrowSchema) {
        .format = "g",
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseDenseType
    };

    *data = (struct ArrowArray) {
        .length = nz,
        .offset = 0,
        .null_count = 0,
        .n_buffers = 2,
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseDenseData
    };

    data->buffers = (const void**) malloc(2 * sizeof(void*));
    data->buffers[0] = NULL;
    data->buffers[1] = val;

}