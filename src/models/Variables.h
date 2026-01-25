#ifndef VARIABLES_H
#define VARIABLES_H

#include "../Config.h"
#include "../utils/Span.h"

class Variables {
private:
  struct Entry {
    char name[MAX_VAR_NAME];
    long value;
  };
  Entry data[MAX_VARS];
  int count = 0;

public:
  long get(const Span &name);
  void set(const Span &name, long val);
  void reset();

  int getCount() { return count; }
  const char *getName(int i) { return data[i].name; }
  long getValue(int i) { return data[i].value; }
};

#endif
