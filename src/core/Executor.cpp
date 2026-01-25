#include "Executor.h"
#include "../Config.h"
#include "../context/Task.h"
#include "../models/Aliases.h"
#include "../models/Program.h"
#include "../models/Variables.h"
#include "Scheduler.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <string.h>
#include <time.h>

// --- Helpers ---
int Executor::precedence(char op) {
  if (op == '*' || op == '/' || op == '%')
    return 3;
  if (op == '+' || op == '-')
    return 2;
  if (op == '&' || op == '^' || op == '|')
    return 1;
  return 0;
}

long Executor::applyOp(long a, long b, char op) {
  switch (op) {
  case '+':
    return a + b;
  case '-':
    return a - b;
  case '*':
    return a * b;
  case '/':
    return (b != 0) ? a / b : 0;
  case '%':
    return (b != 0) ? a % b : 0;
  case '&':
    return a & b;
  case '|':
    return a | b;
  case '^':
    return a ^ b;
  }
  return 0;
}

long Executor::resolve(Context &ctx, const Span &t) {
  if (t.len == 0)
    return 0;
  if (isdigit(t.ptr[0]) || (t.len > 1 && t.ptr[0] == '-'))
    return t.toInt();
  return ctx.vars.get(t);
}

long Executor::evalExpr(Context &ctx, Span tokens[], int count) {
  // simplified shunting yard
  char opStack[MAX_TOKENS];
  int opTop = -1;
  struct {
    long val;
    char op;
    bool isOp;
  } rpn[MAX_TOKENS];
  int rpnLen = 0;

  for (int i = 0; i < count; i++) {
    Span t = tokens[i];
    char c = t.ptr[0];
    bool isOpChar = (t.len == 1) && (strchr("+-*/%&|^()", c));

    if (!isOpChar) {
      rpn[rpnLen++] = {resolve(ctx, t), 0, false};
    } else if (c == '(') {
      opStack[++opTop] = '(';
    } else if (c == ')') {
      while (opTop >= 0 && opStack[opTop] != '(')
        rpn[rpnLen++] = {0, opStack[opTop--], true};
      if (opTop >= 0)
        opTop--;
    } else {
      while (opTop >= 0 && opStack[opTop] != '(' &&
             precedence(opStack[opTop]) >= precedence(c))
        rpn[rpnLen++] = {0, opStack[opTop--], true};
      opStack[++opTop] = c;
    }
  }
  while (opTop >= 0)
    rpn[rpnLen++] = {0, opStack[opTop--], true};

  long stack[MAX_TOKENS];
  int sTop = -1;
  for (int i = 0; i < rpnLen; i++) {
    if (rpn[i].isOp) {
      if (sTop < 1)
        return 0;
      long b = stack[sTop--];
      long a = stack[sTop--];
      stack[++sTop] = applyOp(a, b, rpn[i].op);
    } else {
      stack[++sTop] = rpn[i].val;
    }
  }
  return (sTop >= 0) ? stack[sTop] : 0;
}

void Executor::formatPath(const char *input, char *output) {
  if (input[0] == '/')
    strcpy(output, input);
  else {
    output[0] = '/';
    strcpy(output + 1, input);
  }
}

// --- Handlers ---

bool Executor::execIO(Context &ctx, Span t[], int n) {
  if (t[0].equals("help")) {
    if (ctx.out) {
      if (n > 1) {
        if (t[1].equals("list"))
          ctx.out->println("list: Show program with line IDs.");
        else if (t[1].equals("edit"))
          ctx.out->println("edit <id> <text>: Replace line <id>.");
        else if (t[1].equals("del"))
          ctx.out->println("del <id>: Delete line <id>.");
        else if (t[1].equals("insert"))
          ctx.out->println("insert <id> <text>: Insert line at <id>.");
        else if (t[1].equals("write"))
          ctx.out->println("write <pin> <val>: Digital Write (0/1).");
        else if (t[1].equals("read"))
          ctx.out->println("read <pin> [var]: Digital Read.");
        else if (t[1].equals("aread"))
          ctx.out->println("aread <pin> [var]: Analog Read (0-4095).");
        else if (t[1].equals("pwm"))
          ctx.out->println("pwm <pin> <val>: Analog Write (0-255).");
        else if (t[1].equals("tone"))
          ctx.out->println("tone <pin> <freq> <dur>: Play tone.");
        else if (t[1].equals("touch"))
          ctx.out->println("touch <pin> [var]: Read touch sensor.");
        else if (t[1].equals("post"))
          ctx.out->println("post <url> <data>: HTTP POST.");
        else if (t[1].equals("time"))
          ctx.out->println("time [sync]: Get time or sync NTP.");
        else if (t[1].equals("sleep"))
          ctx.out->println("sleep <sec>: Deep sleep.");
        else if (t[1].equals("rand"))
          ctx.out->println("rand <min> <max>: Random number.");
        else if (t[1].equals("map"))
          ctx.out->println("map <val> <i1> <i2> <o1> <o2>: Map value.");
        else if (t[1].equals("vars"))
          ctx.out->println("vars: List all variables.");
        else if (t[1].equals("run"))
          ctx.out->println("run: Execute program.");
        else
          ctx.out->println("Unknown command.");
      } else {
        ctx.out->println("--- ESP_REPL v8 ---");
        ctx.out->println("Editor: list edit del insert load loadn save");
        ctx.out->println("IO: write read pwm aread tone touch i2c");
        ctx.out->println("Sys: mem info reboot ip rssi vars");
        ctx.out->println("File: cat rm cp ls");
        ctx.out->println("Net: wifi wget post time");
        ctx.out->println("Logic: run bg kill ps rand map");
        ctx.out->println("Type 'help <cmd>' for details.");
      }
    }
  } else if (t[0].equals("mode"))
    pinMode(resolve(ctx, t[1]), t[2].equals("in") ? INPUT : OUTPUT);
  else if (t[0].equals("write"))
    digitalWrite(resolve(ctx, t[1]), resolve(ctx, t[2]));
  else if (t[0].equals("read")) {
    int v = digitalRead(resolve(ctx, t[1]));
    if (n > 2)
      ctx.vars.set(t[2], v);
    else if (ctx.out)
      ctx.out->println(v);
  } else if (t[0].equals("pwm"))
    analogWrite(resolve(ctx, t[1]), resolve(ctx, t[2]));
  else if (t[0].equals("aread")) {
    int v = analogRead(resolve(ctx, t[1]));
    if (n > 2)
      ctx.vars.set(t[2], v);
    else if (ctx.out)
      ctx.out->println(v);
  } else if (t[0].equals("tone")) {
    // tone <pin> <freq> <duration>
    tone(resolve(ctx, t[1]), resolve(ctx, t[2]), resolve(ctx, t[3]));
  } else if (t[0].equals("touch")) {
    int v = touchRead(resolve(ctx, t[1]));
    if (n > 2)
      ctx.vars.set(t[2], v);
    else if (ctx.out)
      ctx.out->println(v);
  } else
    return false;
  return true;
}

bool Executor::execSys(Context &ctx, Span t[], int n) {
  char buf1[64];
  char buf2[64];
  if (t[0].equals("wifi")) {
    t[1].toBuffer(buf1, 64);
    t[2].toBuffer(buf2, 64);
    t[2].toBuffer(buf2, 64);
    WiFi.begin(buf1, buf2);
  } else if (t[0].equals("ip")) {
    if (ctx.out)
      ctx.out->println(WiFi.localIP());
  } else if (t[0].equals("rssi")) {
    if (ctx.out)
      ctx.out->println(WiFi.RSSI());
  } else if (t[0].equals("wget")) {
    if (WiFi.status() == WL_CONNECTED) {
      t[1].toBuffer(buf1, 64);
      HTTPClient h;
      h.begin(buf1);
      if (h.GET() > 0 && ctx.out)
        ctx.out->println(h.getString());
      h.end();
    }
  } else if (t[0].equals("post")) {
    // post <url> <data>
    if (WiFi.status() == WL_CONNECTED) {
      t[1].toBuffer(buf1, 64);
      t[2].toBuffer(buf2, 64);
      HTTPClient h;
      h.begin(buf1);
      h.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int rc = h.POST(buf2);
      if (ctx.out)
        ctx.out->println(rc);
      if (rc > 0 && ctx.out)
        ctx.out->println(h.getString());
      h.end();
    }
  } else if (t[0].equals("time")) {
    // time [sync]
    if (n > 1 && t[1].equals("sync")) {
      configTime(0, 0, "pool.ntp.org");
      if (ctx.out)
        ctx.out->println("Syncing...");
    } else {
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
        if (ctx.out)
          ctx.out->println("No time");
      } else {
        if (ctx.out)
          ctx.out->println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      }
    }
  } else if (t[0].equals("sleep")) {
    // sleep <sec>
    if (ctx.out)
      ctx.out->println("Sleep...");
    delay(100);
    int s = (n > 1) ? resolve(ctx, t[1]) : 1;
    esp_deep_sleep(s * 1000000ULL);
  } else if (t[0].equals("rand")) {
    // rand <min> <max>
    if (n > 2) {
      long v = random(resolve(ctx, t[1]), resolve(ctx, t[2]));
      if (n > 3)
        ctx.vars.set(t[3], v);
      else if (ctx.out)
        ctx.out->println(v);
    }
  } else if (t[0].equals("map")) {
    // map <val> <in_min> <in_max> <out_min> <out_max>
    if (n > 5) {
      long v = map(resolve(ctx, t[1]), resolve(ctx, t[2]), resolve(ctx, t[3]),
                   resolve(ctx, t[4]), resolve(ctx, t[5]));
      if (n > 6)
        ctx.vars.set(t[6], v);
      else if (ctx.out)
        ctx.out->println(v);
    }
  } else if (t[0].equals("abs")) {
    long v = abs(resolve(ctx, t[1]));
    if (n > 2)
      ctx.vars.set(t[2], v);
    else if (ctx.out)
      ctx.out->println(v);
  } else if (t[0].equals("min")) {
    long v = min(resolve(ctx, t[1]), resolve(ctx, t[2]));
    if (n > 3)
      ctx.vars.set(t[3], v);
    else if (ctx.out)
      ctx.out->println(v);
  } else if (t[0].equals("max")) {
    long v = max(resolve(ctx, t[1]), resolve(ctx, t[2]));
    if (n > 3)
      ctx.vars.set(t[3], v);
    else if (ctx.out)
      ctx.out->println(v);
  } else if (t[0].equals("i2c")) {
    if (t[1].equals("scan")) {
      // I2C Scan
      Wire.begin();
      for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
          if (ctx.out) {
            ctx.out->print("Found 0x");
            ctx.out->println(addr, HEX);
          }
        }
      }
    } else if (t[1].equals("read")) {
      // i2c read <addr> <reg>
      Wire.beginTransmission((int)resolve(ctx, t[2]));
      Wire.write((int)resolve(ctx, t[3]));
      Wire.endTransmission();
      Wire.requestFrom((int)resolve(ctx, t[2]), 1);
      if (Wire.available() && ctx.out) {
        ctx.out->println(Wire.read());
      }
    } else if (t[1].equals("write")) {
      // i2c write <addr> <reg> <val>
      Wire.beginTransmission((int)resolve(ctx, t[2]));
      Wire.write((int)resolve(ctx, t[3]));
      Wire.write((int)resolve(ctx, t[4]));
      Wire.endTransmission();
    }
  } else if (t[0].equals("save")) {
    t[1].toBuffer(buf1, 64);
    char path[70];
    formatPath(buf1, path);

    if (ctx.prog.size > 0) {
      File f = FILESYSTEM.open(path, "w");
      for (int i = 0; i < ctx.prog.size; i++) {
        Span &l = ctx.prog.lines[i];
        f.write((const uint8_t *)l.ptr, l.len);
        f.println();
      }
      f.close();
      LOGI("Saved");
    } else {
      LOGE("Empty Prog");
    }
  } else if (t[0].equals("rm")) {
    t[1].toBuffer(buf1, 64);
    char path[70];
    formatPath(buf1, path);
    if (FILESYSTEM.remove(path)) {
      if (ctx.out)
        ctx.out->println("Ok");
    } else
      LOGE("Err");
  } else if (t[0].equals("cat")) {
    t[1].toBuffer(buf1, 64);
    char path[70];
    formatPath(buf1, path);
    File f = FILESYSTEM.open(path, "r");
    if (f) {
      if (ctx.out) {
        while (f.available())
          ctx.out->write(f.read());
        ctx.out->println();
      }
      f.close();
    } else
      LOGE("404");
  } else if (t[0].equals("cp")) {
    t[1].toBuffer(buf1, 64);
    t[2].toBuffer(buf2, 64);
    char p1[70];
    formatPath(buf1, p1);
    char p2[70];
    formatPath(buf2, p2);
    File src = FILESYSTEM.open(p1, "r");
    if (src) {
      File dst = FILESYSTEM.open(p2, "w");
      if (dst) {
        while (src.available())
          dst.write(src.read());
        dst.close();
        if (ctx.out)
          ctx.out->println("Ok");
      } else
        LOGE("Dst Err");
      src.close();
    } else {
      LOGE("Src Err");
    }
  } else if (t[0].equals("mem")) {
    if (ctx.out) {
      ctx.out->print("Free Heap: ");
      ctx.out->print(ESP.getFreeHeap());
      ctx.out->println(" bytes");
    }
  } else if (t[0].equals("reboot")) {
    if (ctx.out)
      ctx.out->println("Rebooting...");
    delay(100);
    ESP.restart();
  } else if (t[0].equals("info")) {
    if (ctx.out) {
      ctx.out->println("ESP_REPL v8 System");
      ctx.out->print("Uptime: ");
      ctx.out->print(millis() / 1000);
      ctx.out->println("s");
      ctx.out->print("CPU Freq: ");
      ctx.out->print(ESP.getCpuFreqMHz());
      ctx.out->println("MHz");
    }
  } else if (t[0].equals("vars")) {
    if (ctx.out) {
      for (int i = 0; i < ctx.vars.getCount(); i++) {
        ctx.out->print(ctx.vars.getName(i));
        ctx.out->print("=");
        ctx.out->println(ctx.vars.getValue(i));
      }
    }
  } else if (t[0].equals("load")) {
    t[1].toBuffer(buf1, 64);
    char path[70];
    formatPath(buf1, path);

    File f = FILESYSTEM.open(path, "r");
    if (f) {
      ctx.prog.clear();
      char lineBuf[256];
      while (f.available()) {
        int len = f.readBytesUntil('\n', lineBuf, 255);
        lineBuf[len] = 0;
        // trim
        int start = 0;
        while (lineBuf[start] == ' ')
          start++;
        // Note: addLine copies content, so we can use temp buffer
        if (len > 0)
          ctx.prog.addLine(lineBuf + start);
      }
      f.close();
      ctx.prog.scanFuncs();
      LOGI("Loaded");
    }
  } else if (t[0].equals("bg")) {
    int ip = ctx.prog.findFunc(t[1]);
    if (ip != -1)
      ctx.sched.spawn(ip);
    else
      LOGE("Func BG?");
  } else if (t[0].equals("kill")) {
    ctx.sched.kill(resolve(ctx, t[1]));
  } else if (t[0].equals("ps")) {
    for (int i = 0; i < MAX_TASKS; i++)
      if (ctx.sched.tasks[i].active && ctx.out)
        ctx.out->printf("%d: IP %d %s\n", i, ctx.sched.tasks[i].ip,
                        ctx.sched.tasks[i].waiting ? "W" : "R");
  } else if (t[0].equals("ls")) {
    File root = FILESYSTEM.open("/");
    if (!root || !root.isDirectory()) {
      LOGE("Dir error");
    } else {
      File f = root.openNextFile();
      while (f && ctx.out) {
        ctx.out->print(f.name());
        if (f.isDirectory())
          ctx.out->print("/");
        ctx.out->println();
        f = root.openNextFile();
      }
    }
  } else if (t[0].equals("menu")) {
    File root = FILESYSTEM.open("/");
    int i = 1;
    File f = root.openNextFile();
    while (f && ctx.out) {
      if (!f.isDirectory()) {
        ctx.out->print("[");
        ctx.out->print(i++);
        ctx.out->print("] ");
        ctx.out->println(f.name());
      }
      f = root.openNextFile();
    }
    if (ctx.out)
      ctx.out->println("Type: loadn <id>");
  } else if (t[0].equals("loadn")) {
    int id = resolve(ctx, t[1]);
    File root = FILESYSTEM.open("/");
    File f = root.openNextFile();
    int i = 1;
    while (f) {
      if (!f.isDirectory()) {
        if (i == id) {
          if (ctx.out)
            ctx.out->println(f.name());
          // Load logic (duplicate of load)
          File lf = FILESYSTEM.open(
              f.name(),
              "r"); // SD needs full path? relative to root works often
          if (lf) {
            ctx.prog.clear();
            char lineBuf[256];
            while (lf.available()) {
              int len = lf.readBytesUntil('\n', lineBuf, 255);
              lineBuf[len] = 0;
              int start = 0;
              while (lineBuf[start] == ' ')
                start++;
              if (len > 0)
                ctx.prog.addLine(lineBuf + start);
            }
            lf.close();
            ctx.prog.scanFuncs();
            LOGI("Loaded");
          }
          break;
        }
        i++;
      }
      f = root.openNextFile();
    }
  } else if (t[0].equals("list")) {
    for (int i = 0; i < ctx.prog.size; i++) {
      if (ctx.out) {
        ctx.out->print(i);
        ctx.out->print(" ");
        ctx.out->println(ctx.prog.lines[i].ptr);
      }
    }
  } else if (t[0].equals("del")) {
    int id = resolve(ctx, t[1]);
    if (ctx.prog.deleteLine(id)) {
      if (ctx.out)
        ctx.out->println("Ok");
    } else
      LOGE("Err");
  } else if (t[0].equals("edit")) {
    int id = resolve(ctx, t[1]);
    char buf[256];
    buf[0] = 0;
    for (int i = 2; i < n; i++) {
      strcat(buf, t[i].ptr);
      if (i < n - 1)
        strcat(buf, " ");
    }
    if (ctx.prog.replaceLine(id, buf)) {
      if (ctx.out)
        ctx.out->println("Ok");
    } else
      LOGE("Err");
  } else if (t[0].equals("insert")) {
    int id = resolve(ctx, t[1]);
    char buf[256];
    buf[0] = 0;
    for (int i = 2; i < n; i++) {
      strcat(buf, t[i].ptr);
      if (i < n - 1)
        strcat(buf, " ");
    }
    if (ctx.prog.insertLine(id, buf)) {
      if (ctx.out)
        ctx.out->println("Ok");
    } else
      LOGE("Err");
  }

  else if (t[0].equals("check")) {
    // Syntax Validator
    int stack[MAX_STACK];
    int top = -1;
    bool err = false;
    for (int i = 0; i < ctx.prog.size; i++) {
      Span &l = ctx.prog.lines[i];
      if (l.len < 2)
        continue;
      if (strncmp(l.ptr, "if", 2) == 0 && (l.len == 2 || l.ptr[2] == ' ')) {
        if (top < MAX_STACK - 1)
          stack[++top] = BLK_IF;
        else {
          if (ctx.out)
            ctx.out->printf("Err: Stack Overflow at line %d\n", i);
          err = true;
          break;
        }
      } else if (strncmp(l.ptr, "while", 5) == 0 &&
                 (l.len == 5 || l.ptr[5] == ' ')) {
        if (top < MAX_STACK - 1)
          stack[++top] = BLK_WHILE;
        else {
          if (ctx.out)
            ctx.out->printf("Err: Stack Overflow at line %d\n", i);
          err = true;
          break;
        }
      } else if (strncmp(l.ptr, "for", 3) == 0 &&
                 (l.len == 3 || l.ptr[3] == ' ')) {
        if (top < MAX_STACK - 1)
          stack[++top] = BLK_FOR;
        else {
          if (ctx.out)
            ctx.out->printf("Err: Stack Overflow at line %d\n", i);
          err = true;
          break;
        }
      } else if (l.equals("endif")) {
        if (top >= 0 && stack[top] == BLK_IF)
          top--;
        else {
          if (ctx.out)
            ctx.out->printf("Err: Unexpected 'endif' at line %d\n", i);
          err = true;
          break;
        }
      } else if (l.equals("endw")) {
        if (top >= 0 && stack[top] == BLK_WHILE)
          top--;
        else {
          if (ctx.out)
            ctx.out->printf("Err: Unexpected 'endw' at line %d\n", i);
          err = true;
          break;
        }
      } else if (l.equals("next")) {
        if (top >= 0 && stack[top] == BLK_FOR)
          top--;
        else {
          if (ctx.out)
            ctx.out->printf("Err: Unexpected 'next' at line %d\n", i);
          err = true;
          break;
        }
      }
    }
    if (!err) {
      if (top == -1) {
        if (ctx.out)
          ctx.out->println("Syntax OK");
      } else {
        if (ctx.out)
          ctx.out->printf("Err: Unclosed block type %d\n", stack[top]);
      }
    }
  } else
    return false;
  return true;
}

bool Executor::execFlow(Context &ctx, Span t[], int n) {
  Task &task = ctx.task;
  if (t[0].equals("func")) {
    int d = 1;
    task.ip++;
    while (task.ip < ctx.prog.size && d > 0) {
      Span &l = ctx.prog.lines[task.ip];
      if (l.len >= 5 && strncmp(l.ptr, "func ", 5) == 0)
        d++;
      if (l.equals("endf"))
        d--;
      if (d > 0)
        task.ip++;
    }
    return true;
  }
  if (t[0].equals("endf")) {
    if (task.callTop >= 0)
      task.ip = task.callStack[task.callTop--];
    else {
      task.active = false;
      LOGI("Finished");
    }
    return true;
  }
  if (t[0].equals("call")) {
    int addr = ctx.prog.findFunc(t[1]);
    if (addr != -1) {
      if (task.callTop < MAX_STACK - 1) {
        task.callStack[++task.callTop] = task.ip;
        task.ip = addr - 1;
      } else {
        LOGE("Call Stack Overflow");
      }
      return true;
    }
    LOGE("Func?");
    return true;
  }
  // --- Control Flow (Unified) ---
  if (t[0].equals("if")) {
    bool parentExec =
        (task.blockTop < 0) ? true : task.blocks[task.blockTop].execute;
    bool cond = false;
    if (parentExec && n >= 4) {
      long a = resolve(ctx, t[1]);
      long b = resolve(ctx, t[3]);
      if (t[2].equals("=="))
        cond = (a == b);
      else if (t[2].equals("!="))
        cond = (a != b);
      else if (t[2].equals("<"))
        cond = (a < b);
      else if (t[2].equals(">"))
        cond = (a > b);
      else if (t[2].equals("<="))
        cond = (a <= b);
      else if (t[2].equals(">="))
        cond = (a >= b);
    }
    if (task.blockTop < MAX_STACK - 1) {
      task.blockTop++;
      task.blocks[task.blockTop].type = BLK_IF;
      task.blocks[task.blockTop].execute = parentExec && cond;
      task.blocks[task.blockTop].parentExec = parentExec;
      task.blocks[task.blockTop].elseSeen = false;
    } else
      LOGE("Stack Overflow");
    return true;
  }
  if (t[0].equals("else")) {
    if (task.blockTop >= 0 && task.blocks[task.blockTop].type == BLK_IF &&
        !task.blocks[task.blockTop].elseSeen) {
      bool p = task.blocks[task.blockTop].parentExec;
      bool w = task.blocks[task.blockTop].execute;
      task.blocks[task.blockTop].execute = p && !w;
      task.blocks[task.blockTop].elseSeen = true;
    }
    return true;
  }
  if (t[0].equals("end") || t[0].equals("endif")) {
    if (task.blockTop >= 0 && task.blocks[task.blockTop].type == BLK_IF)
      task.blockTop--;
    return true;
  }
  if (t[0].equals("while")) {
    bool parentExec =
        (task.blockTop < 0) ? true : task.blocks[task.blockTop].execute;

    // Re-entry? Only if top is current WHILE
    if (task.blockTop >= 0 && task.blocks[task.blockTop].type == BLK_WHILE &&
        task.blocks[task.blockTop].startIp == task.ip) {
      // Check condition
      if (!parentExec) { // Should check condition
                         // already active logic implies parentExec true?
      }
      long a = resolve(ctx, t[1]);
      long b = resolve(ctx, t[3]);
      bool run = false;
      if (t[2].equals("<"))
        run = a < b;
      else if (t[2].equals(">"))
        run = a > b;
      else if (t[2].equals("=="))
        run = a == b;
      else if (t[2].equals("!="))
        run = a != b;

      if (!run) {
        // Exit loop
        task.blockTop--;
        // Skip to endw
        // For robustness, scan
        int d = 1;
        while (++task.ip < ctx.prog.size && d) {
          Span l = ctx.prog.lines[task.ip];
          if (strncmp(l.ptr, "while", 5) == 0)
            d++;
          if (l.equals("endw"))
            d--;
        }
      }
      return true;
    }

    // First Entry
    if (parentExec) {
      long a = resolve(ctx, t[1]);
      long b = resolve(ctx, t[3]);
      bool run = false;
      if (t[2].equals("<"))
        run = a < b;
      else if (t[2].equals(">"))
        run = a > b;
      else if (t[2].equals("=="))
        run = a == b;
      else if (t[2].equals("!="))
        run = a != b;

      if (run) {
        if (task.blockTop < MAX_STACK - 1) {
          task.blockTop++;
          task.blocks[task.blockTop].type = BLK_WHILE;
          task.blocks[task.blockTop].execute = true;
          task.blocks[task.blockTop].parentExec = true;
          task.blocks[task.blockTop].startIp = task.ip;
        }
      } else {
        // Skip
        int d = 1;
        while (++task.ip < ctx.prog.size && d) {
          Span l = ctx.prog.lines[task.ip];
          if (strncmp(l.ptr, "while", 5) == 0)
            d++;
          if (l.equals("endw"))
            d--;
        }
      }
    } else {
      // Skip (parent exec false)
      int d = 1;
      while (++task.ip < ctx.prog.size && d) {
        Span l = ctx.prog.lines[task.ip];
        if (strncmp(l.ptr, "while", 5) == 0)
          d++;
        if (l.equals("endw"))
          d--;
      }
    }
    return true;
  }
  if (t[0].equals("endw")) {
    if (task.blockTop >= 0 && task.blocks[task.blockTop].type == BLK_WHILE) {
      task.ip = task.blocks[task.blockTop].startIp - 1; // Loop back
    }
    return true;
  }
  if (t[0].equals("for")) {
    // for i 0 10
    bool parentExec =
        (task.blockTop < 0) ? true : task.blocks[task.blockTop].execute;
    if (parentExec && n >= 4) {
      Span var = t[1];
      long start = resolve(ctx, t[2]);
      long end = resolve(ctx, t[3]);
      ctx.vars.set(var, start);

      if (task.blockTop < MAX_STACK - 1) {
        task.blockTop++;
        Block &b = task.blocks[task.blockTop];
        b.type = BLK_FOR;
        b.execute = true;
        b.parentExec = true;
        b.startIp = task.ip;
        b.limit = end;
        b.step = 1;
        int vLen = (var.len < MAX_VAR_NAME - 1) ? var.len : MAX_VAR_NAME - 1;
        memcpy(b.varName, var.ptr, vLen);
        b.varName[vLen] = 0;
      }
    } else {
      // Skip
      int d = 1;
      while (++task.ip < ctx.prog.size && d) {
        Span l = ctx.prog.lines[task.ip];
        if (strncmp(l.ptr, "for", 3) == 0)
          d++;
        if (l.equals("next"))
          d--;
      }
    }
    return true;
  }
  if (t[0].equals("next")) {
    if (task.blockTop >= 0 && task.blocks[task.blockTop].type == BLK_FOR) {
      Block &b = task.blocks[task.blockTop];
      Span vSpan(b.varName, strlen(b.varName));
      long val = ctx.vars.get(vSpan);
      val += b.step;
      ctx.vars.set(vSpan, val);

      if (val < b.limit) {
        task.ip = b.startIp; // Loop (IP will increment at loop end)
      } else {
        task.blockTop--;
      }
    }
    return true;
  }
  if (t[0].equals("repeat")) {
    // repeat <n> <cmd>
    if (n >= 3) {
      int count = resolve(ctx, t[1]);
      // Reconstruct command (simple approximate)
      // We can just execute the rest tokens? But tokenize destroys original
      // structure if we aren't careful. Better: executing repeated calls
      // requires parsing? Simplest: use t[2..n] as tokens? executeLine needs
      // char*. Let's reconstruct from t[2] ptr.
      const char *cmdStart = t[2].ptr;
      // Length is tricky if spaces were compressed but Tokenizer keeps pointers
      // into original string. We assume original string is unmodified. We need
      // to execute `count` times. ISSUE: executeLine is recursive? `repeat 5
      // echo hi` -> loops 5 times.
      for (int i = 0; i < count; i++) {
        // We need a non-const Buffer? executeLine takes const char*
        // But `execLang` takes Span[].
        // We can call `execLang` or `execSys` directly with shifted tokens!
        // t+2 is the new start, n-2 is new count.
        // Check where it goes.
        Span *subT = t + 2;
        int subN = n - 2;
        if (execFlow(ctx, subT, subN))
          continue;
        if (execLang(ctx, subT, subN))
          continue;
        if (execIO(ctx, subT, subN))
          continue;
        if (execSys(ctx, subT, subN))
          continue;
      }
    }
    return true;
  }
  return false;
}

bool Executor::execLang(Context &ctx, Span t[], int n) {
  if (t[0].equals("set")) {
    if (n > 2)
      ctx.vars.set(t[1], evalExpr(ctx, t + 2, n - 2));
  } else if (t[0].equals("alias")) {
    // alias name cmd
    if (n >= 3) {
      // cmd is the rest of the string
      // Assume single token cmd for now or reconstruct
      // Simpler: alias name "cmd"
      if (t[2].ptr[0] == '"') {
        // string literal
        int len = t[2].len;
        const char *s = t[2].ptr + 1;
        if (len > 0 && s[len - 2] == '"')
          len--;
        Span cmdSpan(s, len > 1 ? len - 2 : 0);
        ctx.aliases.set(t[1], cmdSpan);
      } else {
        ctx.aliases.set(t[1], t[2]);
      }
    }
  } else if (t[0].equals("inc")) {
    ctx.vars.set(t[1], ctx.vars.get(t[1]) + (n > 2 ? resolve(ctx, t[2]) : 1));
  } else if (t[0].equals("print") || t[0].equals("echo")) {
    for (int i = 1; i < n; i++) {
      // ... (existing print logic)
      if (t[i].ptr[0] == '"') {
        int len = t[i].len;
        const char *s = t[i].ptr + 1;
        if (len > 0 && s[len - 2] == '"')
          len--;
        if (ctx.out)
          ctx.out->write((const uint8_t *)s, len > 1 ? len - 2 : 0);
      } else {
        if (ctx.out)
          ctx.out->print(resolve(ctx, t[i]));
      }
      if (ctx.out)
        ctx.out->print(" ");
    }
    if (ctx.out)
      ctx.out->println();
  } else if (t[0].equals("delay") || t[0].equals("wait")) {
    ctx.task.waitMs = resolve(ctx, t[1]);
    ctx.task.waitStart = millis();
    ctx.task.waiting = true;
  } else
    return false;
  return true;
}

bool Executor::executeLine(Context &ctx, const char *line) {
  if (line == nullptr || line[0] == 0 || line[0] == '#')
    return true;
  // Check Alias first (simple check)
  // We tokenize first to get first word
  Span t[MAX_TOKENS];
  int n = tokenize(line, t, MAX_TOKENS);
  if (n == 0)
    return true;

  const char *aliasCmd = ctx.aliases.get(t[0]);
  if (aliasCmd) {
    // Execute the alias command instead
    // This supports simple alias replacement (single level)
    // Recursive alias? Be careful.
    // New buffer for recursion safety
    char buf[64];
    strncpy(buf, aliasCmd, 63);
    buf[63] = 0;
    return executeLine(ctx, buf);
  }

  if (execFlow(ctx, t, n))
    return true;
  if (ctx.task.blockTop >= 0 && !ctx.task.blocks[ctx.task.blockTop].execute)
    return true;
  if (execLang(ctx, t, n))
    return true;
  if (execIO(ctx, t, n))
    return true;
  if (execSys(ctx, t, n))
    return true;
  return true;
}
