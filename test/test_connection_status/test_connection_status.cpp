#include <unity.h>

#include "connection_status.h"

void should_classify_200_as_ok() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::kOk), static_cast<int>(classifyPollStatusCode(200)));
}

void should_classify_204_as_ok() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::kOk), static_cast<int>(classifyPollStatusCode(204)));
}

void should_classify_401_as_auth_failed() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::kAuthFailed), static_cast<int>(classifyPollStatusCode(401)));
}

void should_classify_429_as_rate_limited() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::kRateLimited),
                        static_cast<int>(classifyPollStatusCode(429)));
}

void should_classify_503_as_server_error() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::kServerError),
                        static_cast<int>(classifyPollStatusCode(503)));
}

void should_classify_403_as_server_error() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::kServerError),
                        static_cast<int>(classifyPollStatusCode(403)));
}

void should_classify_negative_code_as_network_error() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::kNetworkError),
                        static_cast<int>(classifyPollStatusCode(-1)));
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
