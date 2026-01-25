#ifndef SPAN_H
#define SPAN_H

#include <Arduino.h>

struct Span {
  const char *ptr;
  int len;

  bool equals(const char *str) const;
  long toInt() const;
  void toBuffer(char *buf, int max) const;
};

// int tokenize(const String &line, Span out[], int maxT); // Removed
int tokenize(const char *line, Span out[], int maxT);

#endif
