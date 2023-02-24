#include <arrow/api.h>

#include <iostream>

int main() {
  arrow::Status st = RunMain();
  if (!st.ok()) {
    std::cerr << st << std::endl;
    return 1;
  }
  return 0;
}

arrow::Status RunMain() {
    
}