#include "Variables.h"
#include <string.h>

long Variables::get(const Span &name) {
  for (int i = 0; i < count; i++) {
    if (name.equals(data[i].name))
      return data[i].value;
  }
  return 0;
}

void Variables::set(const Span &name, long val) {
  for (int i = 0; i < count; i++) {
    if (name.equals(data[i].name)) {
      data[i].value = val;
      return;
    }
  }
  if (count < MAX_VARS) {
    int len = (name.len < MAX_VAR_NAME - 1) ? name.len : MAX_VAR_NAME - 1;
    if (len > 0) {
      memcpy(data[count].name, name.ptr, len);
      data[count].name[len] = 0;
      data[count].value = val;
      count++;
    }
  }
}

void Variables::reset() { count = 0; }
