#include <runtime/local/datastructures/CArrowStructures.h>
#include <runtime/local/io/MatrixMarket.h>
#include <runtime/local/io/MMAdapter.h>
#include <stdlib.h>
#include <string.h>

const char* TypeCodeToArrow(MM_typecode type){
    switch (type[2])
    {
    case 'R': //real to float64
        return "g";
    case 'P': //pattern to boolean
        return "b";
    default:  //default to int32
        return "i";
    }
};

int SizeofTypeCode(MM_typecode type){
    switch (type[2])
    {
    case 'R': //real to float64
        return 8;
    case 'P': //pattern to boolean
        return 1;
    default:  //default to int32
        return 4;
    }
}

void ReleaseBaseType(Schema *schema){
    schema->release = NULL;
}

void ReleaseNamedType(Schema *schema){
    if(schema->name != NULL && schema->name[0]!=0)
        free((void*)schema->name);
    schema->release = NULL;
}

// void ReleaseCSRType(Schema *schema){
//     free(schema->children[0])
// }

void ReleaseFrameType(Schema *schema){
    for(int i = 0; i<schema->n_children; i++)
        schema->children[i]->release(schema->children[i]);
    free(schema->children[0]);
    free(schema->children);
    schema->release = NULL;
}

void ReleaseBaseData(Array *data){
    free(data->buffers);
    data->release = NULL;
}

void ReleaseDenseData(Array *data){
    free((void *) data->buffers[1]);
    free(data->buffers);
    data->release = NULL;
}

void ReleaseCSRData(Array *data){
    for(int i = 0; i<data->n_children; i++)
        data->children[i]->release(data->children[i]);
    free(data->children);
    free((void*)data->buffers[0]);
    free(data->buffers);
    data->release=NULL;
}

void ReleaseFrameData(Array *data){
    free((void*) data->buffers);
    free((void*) data->children[0]);
    free(data->children);
    data->release = NULL;
}

void CreateDenseMatrix(const char *fname, Schema *type_info, Array *data){
    int row, col, nz, *xs, *ys;
    void *val;
    MM_typecode code;

    mm_read_mtx_crd(fname, &row, &col, &nz, &xs, &ys, &val, &code);

    free(xs);
    free(ys);

    *type_info = (Schema) {
        .format = TypeCodeToArrow(code),
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseBaseType
    };

    *data = (Array) {
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

void SortCSRElements(int **rows, int **cols, void **vals, int num, int size){
    int *_rows = *rows;
    int *_cols = *cols;
    void *_vals = *vals;
    int *ids = (int*)malloc(sizeof(int)*num);
    for(int i = 0; i<num; i++) ids[i] = i;
    // TODO: Replace sorting with something more efficient than selection sort.
    for(int i = 0; i<num; i++){
        int best_idx = i;
        for(int j = i+1; j<num; j++){
            int _j = ids[j], _best = ids[best_idx];
            if(_rows[_j] < _rows[_best] || (_rows[_j]==_rows[_best] && _cols[_j] < _cols[_best]))
                best_idx = j;
        }
        int temp = ids[best_idx];
        ids[best_idx] = ids[i];
        ids[i] = temp;
    }
    // Rest is independent of sorting implementation.
    int *dest = (int*)malloc((sizeof(int)*2+size)*num);
    int *row_dest = dest, *col_dest = dest+num;
    void *val_dest = (void*)(dest+2*num);

    for(int i = 0; i<num; i++){
        int src = ids[i];
        row_dest[i] = _rows[src];
        col_dest[i] = _cols[src];
        memcpy(val_dest+i*size, _vals+src*size, size);
        //val_dest[i] = _vals[src];
    }

    free((void*)*rows);
    free((void*)*cols);
    free((void*)*vals);
    *rows = row_dest;
    *cols = col_dest;
    *vals = val_dest;
}

void CreateCSRMatrix(const char *fname, Schema *type_info, Array *data){

    int row, col, nz, *rs, *cs;
    void *val;
    MM_typecode code;

    mm_read_mtx_crd(fname, &row, &col, &nz, &rs, &cs, &val, &code);
    // sort val,rs and cs based on row_idx in rs first, then col_idx in cs (ascending)
    SortCSRElements(&rs, &cs, &val, nz, SizeofTypeCode(code));
    // val and cs can now be element 0 and 1 in the resulting array.
    // create element 2 by [0] + [count(i, rs) | 0<=i<row]
    int *row_ids = (int*)malloc((row+1)*sizeof(int));
    for(int i = 0; i<=row; i++) row_ids[i] = 0;
    for(int i = 0; i<nz; i++) row_ids[rs[i]]++;
    for(int i = 1; i<=row; i++) row_ids[i] += row_ids[i-1];

    // create and arrow object of type
    // DenseUnion<List<UInt64>, FixedSizeList<VT>[nz]>
    // OR DenseUnion<List<UInt64>, List<VT>>

    Schema *eles = (Schema*)malloc(sizeof(Schema)*4);
    Schema
    *vt = eles+3,
    *ui = eles+2,
    *fsl = eles+1,
    *lui = eles;

    Schema **ptrs = (Schema**)malloc(sizeof(Schema*)*4);

    ptrs[3] = vt;
    ptrs[2] = ui;
    ptrs[1] = fsl;
    ptrs[0] = lui;

    *vt = (Schema){
        .format = TypeCodeToArrow(code),
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseBaseType
    };

    *fsl = (Schema){
        .format = "+l", //list (variable)
        .name = "values",
        .metadata = NULL,
        .flags = 0,
        .n_children = 1,
        .children = ptrs+3,
        .dictionary = NULL,
        .release = &ReleaseBaseType
    };

    *ui = (Schema){
        .format = "i", //int32 "L", //uint64
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseBaseType
    };

    *lui = (Schema){
        .format = "+l", //list (variable)
        .name = "indices",
        .metadata = NULL,
        .flags = 0,
        .n_children = 1,
        .children = ptrs+2,
        .dictionary = NULL,
        .release = &ReleaseBaseType
    };

    *type_info = (Schema){
        .format = "+ud:0,1", //DenseUnion<List,List>
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = 2,
        .children = ptrs,
        .dictionary = NULL,
        .release = &ReleaseFrameType
    };

    Array *children = (Array*)malloc(4*sizeof(Array));
    Array
    *child0 = children,
    *child1 = children+1,
    *child0child = children+2,
    *child1child = children+3;

    int *indices = (int*)malloc(sizeof(int)*(row+1+nz));
    memcpy(indices, row_ids, sizeof(int)*(row+1));
    memcpy(indices+row+1, cs, sizeof(int)*nz);

    int32_t *offset_buffer = (int32_t*)malloc(sizeof(int32_t)*3);
    offset_buffer[0] = 0;
    offset_buffer[1] = row+1;
    offset_buffer[2] = row+1+nz;

    *child0child = (Array){
        .length = row+1+nz,
        .null_count = 0,
        .n_buffers = 2,
        .buffers = (const void**)malloc(2*sizeof(void*)),
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseBaseData
    };
    child0child->buffers[0] = NULL;
    child0child->buffers[1] = indices;

    *child0 = (Array){
        .length=2,
        .null_count = 0,
        .n_buffers = 2,
        .buffers = (const void**)malloc(2*sizeof(void*)),
        .n_children = 1,
        .children = malloc(sizeof(Array*)),//&child0child,
        .dictionary = NULL,
        .release = &ReleaseBaseData
    };

    child0->buffers[0] = NULL;
    child0->buffers[1] = offset_buffer;
    child0->children[0] = child0child;

    *child1child = (Array){
        .length = nz,
        .null_count = 0,
        .n_buffers = 2,
        .buffers = (const void**)malloc(2*sizeof(void*)),
        .n_children = 0,
        .children = NULL,
        .dictionary = NULL,
        .release = &ReleaseBaseData
    };

    child1child->buffers[0] = NULL;
    child1child->buffers[1] = val;

    *child1 = (Array){
        .length = 1,
        .null_count = 0,
        .n_buffers = 2,
        .buffers = (const void**)malloc(2*sizeof(void*)),
        .n_children = 1,
        .children = malloc(sizeof(Array*)),//&child1child,
        .dictionary = NULL,
        .release = &ReleaseBaseData 
    };

    int32_t *offset_buffer1 = (int32_t*)malloc(sizeof(int32_t)*2);
    offset_buffer1[0] = 0;
    offset_buffer1[1] = nz;

    child1->buffers[0] = NULL;
    child1->buffers[1] = offset_buffer1;
    child1->children[0] = child1child;
    // child1->buffers[2] = val;

    int8_t *types = malloc(3*sizeof(int8_t)+3*sizeof(int32_t));
    //set types of first 2 elements to id 0, third element id 1
    types[0] = 0;
    types[1] = 0;
    types[2] = 1;

    int32_t *offsets = (int32_t*)types+3;
    offsets[0] = 0;
    offsets[1] = 1;
    offsets[2] = 0;

    const void **arr_bufs = malloc(2*sizeof(void*));
    arr_bufs[0] = (void*)types;
    arr_bufs[1] = (void*)offsets;

    *data = (Array){
        .length = 3,
        .null_count = 0,
        .offset = 0,
        .n_buffers = 2,
        .buffers = arr_bufs,
        .n_children = 2,
        .children = (Array**)malloc(sizeof(Array*)*2),
        .dictionary = NULL,
        .release = &ReleaseCSRData
    };

    data->children[0]=child0;
    data->children[1]=child1;
}

void CreateFrame(const char *fname, Schema *type_info, Array *data){
    int row, col, nz, *xs, *ys;
    void *val;
    MM_typecode code;

    mm_read_mtx_crd(fname, &row, &col, &nz, &xs, &ys, &val, &code);

    free(xs);
    free(ys);

    Schema *fields = (Schema*)malloc(col*sizeof(Schema));

    for(int i = 0; i<col; i++){
        char *field_name = (char*)malloc(2);
        sprintf(field_name, "%d", i);
        fields[i] = (Schema){
            .format = TypeCodeToArrow(code),
            .name = field_name,
            .metadata = NULL,
            .flags = 0,
            .n_children = 0,
            .children = NULL,
            .dictionary = NULL,
            .release = &ReleaseNamedType
        };
    };

    *type_info = (Schema) {
        .format = "+s",
        .name = "",
        .metadata = NULL,
        .flags = 0,
        .n_children = col,
        .children = (Schema**) malloc(col * sizeof(Schema*)),
        .dictionary = NULL,
        .release = &ReleaseFrameType
    };

    for(int i = 0; i<col; i++)
        type_info->children[i] = fields+i;

    *data = (Array) {
        .length = nz,
        .offset = 0,
        .null_count = 0,
        .n_buffers = 1,
        .n_children = col,
        .children = (Array**)malloc(col*sizeof(Array*)),
        .dictionary = NULL,
        .release = &ReleaseFrameData
    };

    data->buffers = (const void**) malloc(sizeof(void*));
    data->buffers[0] = NULL;

    Array *contents = (Array*)malloc(col*sizeof(Array));

    for(int i = 0; i<col; i++){
        contents[i] = (Array){
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