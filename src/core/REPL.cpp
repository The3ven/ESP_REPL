#include "REPL.h"
#include "../Config.h"
#include "../context/Context.h"
#include "Executor.h"
#include <Arduino.h>
#include <string.h>


void REPL::begin() {
  Serial.begin(115200);
  delay(200);
#if ENABLE_SD
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Fail");
  }
#else
  LittleFS.begin(true);
#endif
  Serial.println("ESP_REPL v8 - NO STRINGS");
  Serial.print(">>> ");

  // Autorun
  if (FILESYSTEM.exists("/boot.re")) {
    File f = FILESYSTEM.open("/boot.re", "r");
    if (f) {
      Serial.println("AutoRun...");
      loading = true; // Use loading mode to ingest lines
      char lineBuf[256];
      while (f.available()) {
        int len = f.readBytesUntil('\n', lineBuf, 255);
        lineBuf[len] = 0;
        // Trim
        int start = 0;
        while (lineBuf[start] == ' ')
          start++;
        if (len > 0)
          prog.addLine(lineBuf + start);
      }
      f.close();
      loading = false;
      prog.scanFuncs();
      // Auto-start task 0 if main/loop func found?
      // Guide says: boot.re is just loaded.
      // If user wants to run, they usually put a call at top level,
      // but REPL language is "func based".
      // To actually "Run", we need to execute top-level commands or trigger a
      // specific task. Current architecture: `load` only loads. `run` triggers
      // task 0. We should auto-trigger run:
      sched.tasks[0].reset(0);
      sched.tasks[0].id = 0;
      vars.reset();
      LOGI("Boot Run");
    }
  }
}

void REPL::loop() {
  Stream *io = &Serial;

#if ENABLE_TELNET
  if (server.hasClient()) {
    if (!client || !client.connected()) {
      if (client)
        client.stop();
      client = server.available();
      client.println("ESP_REPL v8 Telnet");
      client.print(">>> ");
    } else {
      // Reject other clients or just ignore
      WiFiClient reject = server.available();
      reject.stop();
    }
  }
  if (client && client.connected()) {
    io = &client;
  }
#endif

  while (io->available()) {
    char c = io->read();
    if (c == '\n' || c == '\r') {
      if (bufIdx > 0) {
        buffer[bufIdx] = 0;

        // trim right
        while (bufIdx > 0 &&
               (buffer[bufIdx - 1] == ' ' || buffer[bufIdx - 1] == '\t'))
          bufIdx--;
        buffer[bufIdx] = 0;

        // trim left (pointer shift)
        char *cmd = buffer;
        while (*cmd == ' ' || *cmd == '\t')
          cmd++;

        if (strcmp(cmd, "load") == 0) {
          prog.clear();
          loading = true;
          io->println("Ld...");
        } else if (strcmp(cmd, "endprog") == 0) {
          loading = false;
          prog.scanFuncs();
          io->println("Ok");
        } else if (strcmp(cmd, "run") == 0) {
          sched.tasks[0].reset(0);
          sched.tasks[0].id = 0;
          vars.reset();
          // Log output for run is tricky if async, but task 0 starts sync
          // usually
          if (io)
            io->println("Run");
        } else if (loading) {
          prog.addLine(cmd);
        } else if (bufIdx > 0) {
          Context ctx = {prog, vars, aliases, sched, sched.tasks[0], io};
          Executor::executeLine(ctx, cmd);
        }

        bufIdx = 0;
        io->print(">>> ");
      }
    } else {
      if (bufIdx < MAX_BUFFER_SIZE - 1)
        buffer[bufIdx++] = c;
    }
  }
  // Context for scheduler... Scheduler outputs to where?
  // Ideally Scheduler needs output stream too if tasks print.
  // For now, pass io if client connected, else Serial.
  // BUT Scheduler::run doesn't take context or stream currently.
  // Tasks usually don't print unless they call Executor functions which we
  // changed to take Context. So we need to update Scheduler::run signature or
  // Context passing. Actually, Scheduler::run(prog, vars) is called. We need to
  // pass 'io' to it.
  // pass 'io' to it.
  sched.run(prog, vars, aliases, io);
}
