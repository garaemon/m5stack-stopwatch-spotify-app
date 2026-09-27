#include "connection_status.h"

ConnectionStatus classifyPollStatusCode(int statusCode) {
  if (statusCode < 0) {
    return ConnectionStatus::kNetworkError;
  }
  if (statusCode >= 200 && statusCode < 300) {
    return ConnectionStatus::kOk;
  }
  if (statusCode == 401) {
    return ConnectionStatus::kAuthFailed;
  }
  if (statusCode == 429) {
    return ConnectionStatus::kRateLimited;
  }
  return ConnectionStatus::kServerError;
}
