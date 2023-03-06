#include <runtime/local/datastructures/CArrowStructures.h>
#include <runtime/local/io/MatrixMarket.h>
#include <runtime/local/io/MMAdapter.h>
#include <stdlib.h>
#include <string.h>

void ReleaseBaseType(struct ArrowSchema* schema){
    if(schema->name != NULL && schema->name[0]!=0)
        free(schema->name);
    schema->release = NULL;
}

void ReleaseFrameType(struct ArrowSchema* schema){
    for(int i = 0; i<schema->n_children; i++)
        schema->children[i]->release(schema->children[i]);
    free(schema->children[0]);
    free(schema->children);
    schema->release = NULL;
}

void ReleaseBaseData(struct ArrowArray *data){
    free(data->buffers);
    data->release = NULL;
}

void ReleaseDenseData(struct ArrowArray *data){
    free((void *) data->buffers[1]);
    free(data->buffers);
    data->release = NULL;
}

void ReleaseFrameData(struct ArrowArray *data){
    free((void*) data->buffers);
    free((void*) data->children[0]);
    free(data->children);
    data->release = NULL;
}

void CreateDenseMatrix(const char *fname, struct ArrowSchema *type_info, struct ArrowArray *data){
    int row, col, nz, *xs, *ys;
    double *val;
    MM_typecode code;

    mm_read_mtx_crd(fname, &row, &col, &nz, &xs, &ys, &val, &code);

    free(xs);
    free(ys);

    *type_info = (struct ArrowSchema) {
        .format = "g", //double
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseBaseType
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

void CreateCSRMatrix(const char *fname, struct ArrowSchema *type_info, struct ArrayData *data){
    struct ArrowSchema *child = (struct ArrowSchema*)malloc(sizeof(struct ArrowSchema));

    *child = (struct ArrowSchema){
        .format = "g", //double
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseBaseType
    };
}

void CreateFrame(const char *fname, struct ArrowSchema *type_info, struct ArrowArray *data){
    int row, col, nz, *xs, *ys;
    double *val;
    MM_typecode code;

    mm_read_mtx_crd(fname, &row, &col, &nz, &xs, &ys, &val, &code);

    free(xs);
    free(ys);

    struct ArrowSchema *fields = (struct ArrowSchema*)malloc(col*sizeof(struct ArrowSchema));

    for(int i = 0; i<col; i++){
        char *field_name = (char*)malloc(2);
        sprintf(field_name, "%d", i);
        fields[i] = (struct ArrowSchema){
            .format = "g", //double
            .name = field_name,
            .metadata = NULL,
            .flags = 0,
            .n_children = 0,
            .children = NULL,
            .dictionary = NULL,
            .release = &ReleaseBaseType
        };
    };

    *type_info = (struct ArrowSchema) {
        .format = "+s", //double
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = col,
        .children = (struct ArrowSchema**) malloc(col * sizeof(struct ArrowSchema*)),
        .dictionary = NULL,
        .release = &ReleaseFrameType
    };

    for(int i = 0; i<col; i++)
        type_info->children[i] = fields+i;

    *data = (struct ArrowArray) {
        .length = nz,
        .offset = 0,
        .null_count = 0,
        .n_buffers = 1,
        .n_children = col,
        .children = (struct ArrowArray**)malloc(col*sizeof(struct ArrowArray*)),
        .dictionary = NULL,
        .release = &ReleaseFrameData
    };

    data->buffers = (const void**) malloc(sizeof(void*));
    data->buffers[0] = NULL;

    struct ArrowArray *contents = (struct ArrowArray*)malloc(col*sizeof(struct ArrowArray));

    for(int i = 0; i<col; i++){
        contents[i] = (struct ArrowArray){
            .length = row,
            .offset = 0,
            .null_count = 0,
            .n_buffers = 2,
            .n_children = 0,
            .buffers = (const void**) malloc(2*sizeof(void*)),
            .children = NULL,
            .dictionary = NULL,
            .release = &ReleaseBaseData
        };
        contents[i].buffers[0] = NULL;
        contents[i].buffers[1] = val+i*row;
        data->children[i] = contents+i;
    };
}