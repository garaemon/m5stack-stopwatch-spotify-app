#include <unity.h>

#include <string>

#include "lru_cache.h"

void should_return_nullopt_for_missing_key() {
  LruCache<std::string, int> cache(2);

  TEST_ASSERT_FALSE(cache.get("a").has_value());
}

void should_return_stored_value() {
  LruCache<std::string, int> cache(2);
  cache.put("a", 1);

  TEST_ASSERT_EQUAL_INT(1, cache.get("a").value());
}

void should_overwrite_value_for_existing_key() {
  LruCache<std::string, int> cache(2);
  cache.put("a", 1);
  cache.put("a", 2);

  TEST_ASSERT_EQUAL_INT(2, cache.get("a").value());
}

void should_evict_least_recently_used_entry() {
  LruCache<std::string, int> cache(2);
  cache.put("a", 1);
  cache.put("b", 2);
  cache.put("c", 3);

  TEST_ASSERT_FALSE(cache.contains("a"));
}

void should_keep_recently_read_entry_on_eviction() {
  LruCache<std::string, int> cache(2);
  cache.put("a", 1);
  cache.put("b", 2);
  cache.get("a");
  cache.put("c", 3);

  TEST_ASSERT_TRUE(cache.contains("a"));
}

void should_not_grow_when_overwriting() {
  LruCache<std::string, int> cache(2);
  cache.put("a", 1);
  cache.put("b", 2);
  cache.put("b", 3);

  TEST_ASSERT_TRUE(cache.contains("a"));
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_return_nullopt_for_missing_key);
  RUN_TEST(should_return_stored_value);
  RUN_TEST(should_overwrite_value_for_existing_key);
  RUN_TEST(should_evict_least_recently_used_entry);
  RUN_TEST(should_keep_recently_read_entry_on_eviction);
  RUN_TEST(should_not_grow_when_overwriting);
  return UNITY_END();
}
