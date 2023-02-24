#ifndef MM_ADAPTER
#define MM_ADAPTER

void CreateDenseMatrix(const char *fname, struct ArrowSchema *type_info, struct ArrowArray *data);

void CreateCSRMatrix(char *fname, struct ArrowSchema *type_info, struct ArrowArray *data);

void CreateFrame(char *fname, struct ArrowSchema *type_info, struct ArrowArray *data);
#endif //MM_ADAPTER