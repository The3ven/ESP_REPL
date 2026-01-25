#ifndef CONTEXT_H
#define CONTEXT_H

#include <Arduino.h>

// Forward declarations
class Program;
class Variables;
class Aliases;
class Scheduler;
class Task;

struct Context {
  Program &prog;
  Variables &vars;
  Aliases &aliases;
  Scheduler &sched;
  Task &task;
  Stream *out;
};

#endif
