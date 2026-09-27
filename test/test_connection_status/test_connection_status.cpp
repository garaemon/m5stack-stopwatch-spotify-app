#include <unity.h>

#include "connection_status.h"

void should_classify_200_as_ok() { TEST_ASSERT_TRUE(classifyPollStatusCode(200) == ConnectionStatus::kOk); }

void should_classify_204_as_ok() { TEST_ASSERT_TRUE(classifyPollStatusCode(204) == ConnectionStatus::kOk); }

void should_classify_401_as_auth_failed() {
  TEST_ASSERT_TRUE(classifyPollStatusCode(401) == ConnectionStatus::kAuthFailed);
}

void should_classify_429_as_rate_limited() {
  TEST_ASSERT_TRUE(classifyPollStatusCode(429) == ConnectionStatus::kRateLimited);
}

void should_classify_503_as_server_error() {
  TEST_ASSERT_TRUE(classifyPollStatusCode(503) == ConnectionStatus::kServerError);
}

void should_classify_403_as_server_error() {
  TEST_ASSERT_TRUE(classifyPollStatusCode(403) == ConnectionStatus::kServerError);
}

void should_classify_negative_code_as_network_error() {
  TEST_ASSERT_TRUE(classifyPollStatusCode(-1) == ConnectionStatus::kNetworkError);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_classify_200_as_ok);
  RUN_TEST(should_classify_204_as_ok);
  RUN_TEST(should_classify_401_as_auth_failed);
  RUN_TEST(should_classify_429_as_rate_limited);
  RUN_TEST(should_classify_503_as_server_error);
  RUN_TEST(should_classify_403_as_server_error);
  RUN_TEST(should_classify_negative_code_as_network_error);
  return UNITY_END();
}
