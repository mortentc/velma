#include <arrow/api.h>
#include <runtime/local/io/MMFile.h>
#include <iostream>

std::shared_ptr<arrow::Buffer> RunMain(){
    MMFile<uint8_t> mmfile("./test/runtime/local/io/aig.mtx");
    uint8_t *data = (uint8_t*)malloc(sizeof(uint8_t)*mmfile.entryCount());
    size_t i = 0;
    for(auto &entry : mmfile) data[i++]=entry.val;
    arrow::Result<std::unique_ptr<arrow::Buffer>> maybe_buffer = arrow::AllocateBuffer(12);
    if (!maybe_buffer.ok()) {
    // ... handle allocation error
    }
    std::shared_ptr<arrow::Buffer> buffer = *std::move(maybe_buffer);
    uint8_t* buffer_data = buffer->mutable_data();
    memcpy(buffer_data, data, 12);
    return buffer;
}