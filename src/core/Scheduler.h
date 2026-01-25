#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../context/Task.h"

// Forward decl
class Program;
class Variables;
class Aliases;

class Scheduler {
public:
  Task tasks[MAX_TASKS];

  void spawn(int startIp);
  void kill(int id);
  void run(Program &prog, Variables &vars, Aliases &aliases, Stream *out);
};

#endif
