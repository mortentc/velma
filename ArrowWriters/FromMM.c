/* 
*   Matrix Market I/O library for ANSI C
*
*   See http://math.nist.gov/MatrixMarket for details.
*
*
*/
#include "mmio.h"
#include "Adapters.h"

int main(int argc, char *argv[]) {
    char* hard_coded = "../test/runtime/local/io/aig.mtx";
    int M,N,nz;
    int *I,*J;
    double *vals;
    MM_typecode mcode;
    // FILE* fptr = fopen(hard_coded, "r");
    // if (fptr == NULL) printf("oh no\n");
    // else{
    //     mm_read_mtx_array_size(fptr, &M, &N);
    //     printf("%d %d\n", M, N);
    // }
    // mm_read_banner(fptr, mcode);
    int res = mm_read_mtx_crd(hard_coded, &M, &N, &nz, &I, &J, &vals, &mcode);
    printf("Code: %d\n", res);
    // printf("%d", mm_is_array(*mcode));
    GArrowArrayBuilder *builder = constructCSR(int8);
    GArrowType t = garrow_array_builder_get_value_type(builder);
}