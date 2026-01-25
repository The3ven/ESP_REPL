#include "Span.h"
#include <stdlib.h>
#include <string.h>

bool Span::equals(const char *str) const {
  if ((int)strlen(str) != len)
    return false;
  return strncmp(ptr, str, len) == 0;
}

long Span::toInt() const {
  if (len == 0)
    return 0;
  char buf[16];
  int copyLen = (len < 15) ? len : 15;
  memcpy(buf, ptr, copyLen);
  buf[copyLen] = 0;
  return atol(buf);
}

void Span::toBuffer(char *buf, int max) const {
  if (!buf || max <= 0)
    return;
  int l = (len < max - 1) ? len : max - 1;
  memcpy(buf, ptr, l);
  buf[l] = 0;
}

int tokenize(const char *line, Span out[], int maxT) {
  int count = 0;
  const char *start = line;
  const char *p = start;
  const char *tokenStart = nullptr;

  while (*p) {
    if (*p == ' ') {
      if (tokenStart) {
        if (count < maxT)
          out[count++] = {tokenStart, (int)(p - tokenStart)};
        tokenStart = nullptr;
      }
    } else {
      if (!tokenStart)
        tokenStart = p;
    }
    p++;
  }
  if (tokenStart && count < maxT) {
    out[count++] = {tokenStart, (int)(p - tokenStart)};
  }
  return count;
}
