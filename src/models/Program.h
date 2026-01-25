#ifndef PROGRAM_H
#define PROGRAM_H

#include "../Config.h"
#include "../utils/Span.h"

class Program {
public:
  char source[MAX_SOURCE_SIZE];
  int sourceIdx = 0;

  Span lines[MAX_LINES];
  int size = 0;

  struct FuncEntry {
    Span name;
    int ip;
  };
  FuncEntry funcs[MAX_FUNCS];
  int funcCount = 0;

  void clear();
  void addLine(const char *line);
  bool replaceLine(int id, const char *line);
  bool insertLine(int id, const char *line);
  bool deleteLine(int id);
  void scanFuncs();
  int findFunc(const Span &name);
};

#endif
