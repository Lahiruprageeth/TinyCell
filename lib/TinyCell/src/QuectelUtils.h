#pragma once
#include <Arduino.h>

namespace QuectelUtils {
static inline String trimCopy(const String& s) {
  String r = s;
  r.trim();
  return r;
}
}  // namespace QuectelUtils