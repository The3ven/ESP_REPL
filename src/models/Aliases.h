#ifndef ALIASES_H
#define ALIASES_H

#include "../Config.h"
#include "../utils/Span.h"
#include <string.h>

class Aliases {
private:
  struct Entry {
    char name[MAX_ALIAS_NAME];
    char cmd[MAX_ALIAS_CMD];
  };
  Entry data[MAX_ALIASES];
  int count = 0;

public:
  void set(const Span &name, const Span &cmd) {
    if (count < MAX_ALIASES) {
      int nLen =
          (name.len < MAX_ALIAS_NAME - 1) ? name.len : MAX_ALIAS_NAME - 1;
      int cLen = (cmd.len < MAX_ALIAS_CMD - 1) ? cmd.len : MAX_ALIAS_CMD - 1;
      memcpy(data[count].name, name.ptr, nLen);
      data[count].name[nLen] = 0;
      memcpy(data[count].cmd, cmd.ptr, cLen);
      data[count].cmd[cLen] = 0;
      count++;
    }
  }

  // Returns pointer to command if found, else nullptr
  const char *get(const Span &name) {
    for (int i = 0; i < count; i++) {
      if (name.equals(data[i].name))
        return data[i].cmd;
    }
    return nullptr;
  }

  void clear() { count = 0; }

  int getCount() { return count; }
  const char *getName(int i) { return data[i].name; }
  const char *getCmd(int i) { return data[i].cmd; }
};

#endif
