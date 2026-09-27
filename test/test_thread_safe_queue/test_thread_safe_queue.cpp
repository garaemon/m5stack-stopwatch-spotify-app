#include <unity.h>

#include <string>

#include "thread_safe_queue.h"

void should_return_nullopt_when_queue_is_empty() {
  ThreadSafeQueue<int> queue;

  TEST_ASSERT_FALSE(queue.tryPop().has_value());
}

void should_pop_pushed_value() {
  ThreadSafeQueue<std::string> queue;
  queue.push("hello");

  TEST_ASSERT_EQUAL_STRING("hello", queue.tryPop().value().c_str());
}

void should_pop_values_in_push_order() {
  ThreadSafeQueue<int> queue;
  queue.push(1);
  queue.push(2);
  queue.tryPop();

  TEST_ASSERT_EQUAL_INT(2, queue.tryPop().value());
}

void should_be_empty_after_popping_all_values() {
  ThreadSafeQueue<int> queue;
  queue.push(1);
  queue.tryPop();

  TEST_ASSERT_FALSE(queue.tryPop().has_value());
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_return_nullopt_when_queue_is_empty);
  RUN_TEST(should_pop_pushed_value);
  RUN_TEST(should_pop_values_in_push_order);
  RUN_TEST(should_be_empty_after_popping_all_values);
  return UNITY_END();
}
