#include <gtest/gtest.h>

#include <iostream>

using namespace std;

template <typename T>
T add(T value) {
  return value;
}

template <typename T, typename... Args>
T add(T value, Args... args) {
  auto result = add(args...);
  return value + add(args...);
}

TEST(cpp, lazy_evaluation) {
  auto result = add(1, 2, 3);

  ASSERT_EQ(result, 6);
}
