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

TEMPLATE_PRODUCT_TEST_CASE("Import C Arrow as Dense", "[arrow]", (DenseMatrix), (double_t)) {
  struct ArrowSchema schema;
  struct ArrowArray data;

  CreateDenseMatrix("./test/runtime/local/io/dense_crd.mtx", &schema, &data);
  auto array = arrow::ImportArray(&data, &schema);

  int contents[12] = {1,8,3,4,5,2,7,12,9,10,11,6};
  std::vector<int> vec(contents, contents+12);
  auto builder = arrow::Int32Builder();
  arrow::Status _ = builder.AppendValues(vec);
  auto comp_to = builder.Finish();
  auto comp_to_res = std::move(comp_to).ValueOrDie();

  CHECK(array.ok());
  auto arr = std::move(array).ValueOrDie();
  CHECK(arr->length()==12);
  CHECK(arr->Equals(comp_to_res));
}

TEMPLATE_PRODUCT_TEST_CASE("Import C Arrow as CSR", "[arrow]", (DenseMatrix), (double_t)) {
  struct ArrowSchema schema;
  struct ArrowArray data;
  CreateCSRMatrix("./test/runtime/local/io/dense_crd.mtx", &schema, &data);
  auto array = arrow::ImportArray(&data, &schema);
  CHECK(array.ok());
  auto arr = std::move(array).ValueOrDie();
  // std::cout << arr->ToString() << std::endl;
}

TEMPLATE_PRODUCT_TEST_CASE("Import C Arrow as Frame", "[arrow]", (DenseMatrix), (double_t)) {
  struct ArrowSchema schema;
  struct ArrowArray data;
  CreateFrame("./test/runtime/local/io/dense_crd.mtx", &schema, &data);
  auto array = arrow::ImportRecordBatch(&data, &schema);
  CHECK(array.ok());
  auto arr = std::move(array).ValueOrDie();
  // std::cout << arr->ToString() << std::endl;
}