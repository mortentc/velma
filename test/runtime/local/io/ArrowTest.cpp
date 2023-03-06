/*
 * Copyright 2022 The DAPHNE Consortium
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <runtime/local/datastructures/DataObjectFactory.h>
#include <runtime/local/datastructures/DenseMatrix.h>
#include <runtime/local/datastructures/Frame.h>
#include <runtime/local/io/ArrowMM.h>
extern "C" {
#include <runtime/local/io/MMAdapter.h>
}
#include <arrow/c/bridge.h>

#include <tags.h>

#include <catch.hpp>

#include <vector>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

TEMPLATE_PRODUCT_TEST_CASE("Buffer from MM", "[arrow]", (DenseMatrix), (int32_t)) {
  std::shared_ptr<arrow::Buffer> arr = RunMain();
  CHECK(arr->is_cpu());
  uint8_t expected[] {1,2,3,4,5,6,7,8,9,10,11,12};
  arrow::Buffer exp_buf(expected, 12);
  CHECK(arr->Equals(exp_buf,12));
}

TEMPLATE_PRODUCT_TEST_CASE("Import C Arrow as Dense", "[arrow]", (DenseMatrix), (double_t)) {
  struct ArrowSchema schema;
  struct ArrowArray data;
  CreateDenseMatrix("./test/runtime/local/io/dense_crd.mtx", &schema, &data);
  auto array = arrow::ImportArray(&data, &schema);
  CHECK(array.ok());
  auto arr = std::move(array).ValueOrDie();
  std::cout << arr->ToString() << std::endl;
  CHECK(arr->length()==12);
}

TEMPLATE_PRODUCT_TEST_CASE("Import C Arrow as Frame", "[arrow]", (DenseMatrix), (double_t)) {
  struct ArrowSchema schema;
  struct ArrowArray data;
  CreateFrame("./test/runtime/local/io/dense_crd.mtx", &schema, &data);
  auto array = arrow::ImportRecordBatch(&data, &schema);
  CHECK(array.ok());
  auto arr = std::move(array).ValueOrDie();
  std::cout << arr->ToString() << std::endl;
}