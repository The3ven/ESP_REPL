#ifndef REPL_H
#define REPL_H

#include "../Config.h"
#include "../models/Aliases.h"
#include "../models/Program.h"
#include "../models/Variables.h"
#include "Scheduler.h"
#include <WiFi.h>


class REPL {
  Program prog;
  Variables vars;
  Aliases aliases;
  Scheduler sched;

  char buffer[MAX_BUFFER_SIZE];
  int bufIdx = 0;
  bool loading = false;

#if ENABLE_TELNET
  WiFiServer server;
  WiFiClient client;
#endif

public:
  void begin();
  void loop();
};

#endif
