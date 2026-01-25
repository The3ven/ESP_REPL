#include "Scheduler.h"
#include "../models/Program.h"
#include "../models/Variables.h"
#include "Executor.h"
#include <Arduino.h>

void Scheduler::spawn(int startIp) {
  for (int i = 1; i < MAX_TASKS; i++) {
    if (!tasks[i].active) {
      tasks[i].reset(startIp);
      tasks[i].id = i;
      tasks[i].id = i;
      Serial.print("[INF] Started Task ");
      Serial.println(i);
      return;
    }
  }
  LOGE("No free task slots");
}

void Scheduler::kill(int id) {
  if (id > 0 && id < MAX_TASKS)
    tasks[id].active = false;
}

void Scheduler::run(Program &prog, Variables &vars, Aliases &aliases,
                    Stream *out) {
  for (int i = 0; i < MAX_TASKS; i++) {
    Task &t = tasks[i];
    if (!t.active)
      continue;

    if (t.waiting) {
      if (millis() - t.waitStart >= t.waitMs)
        t.waiting = false;
      else
        continue;
    }

    if (t.ip >= prog.size) {
      t.active = false;
      t.active = false;
      Serial.print("[INF] Task ");
      Serial.print(i);
      Serial.println(" Fin");
      continue;
    }

    Context ctx = {prog, vars, aliases, *this, t, out};
    // lines are null-terminated in source buffer, so .ptr is safe
    if (Executor::executeLine(ctx, prog.lines[t.ip].ptr)) {
      t.ip++;
    }
  }
}
