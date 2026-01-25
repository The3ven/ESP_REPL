#ifndef TASK_H
#define TASK_H

#include "../Config.h"

enum BlockType { BLK_IF, BLK_WHILE, BLK_FOR };
struct Block {
    BlockType type;
    bool execute;
    bool parentExec;
    bool elseSeen; // IF
    int startIp;   // WHILE/FOR
    long limit;    // FOR
    long step;     // FOR
    char varName[MAX_VAR_NAME]; // FOR
};

class Task {
public:
  int id;
  bool active;
  int ip;
  bool waiting;
  unsigned long waitStart;
  long waitMs;

  int callStack[MAX_STACK];
  int callTop;
  
  Block blocks[MAX_STACK];
  int blockTop;

  void reset(int startIp) {
    ip = startIp;
    active = true;
    waiting = false;
    callTop = -1;
    blockTop = -1;
  }
};

#endif
