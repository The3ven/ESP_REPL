#include <Arduino.h>

/* ================= CONFIG ================= */

#define MAX_LINES   120
#define MAX_VARS    20
#define MAX_STACK   5

/* ================= LOGGING ================= */

#define LOG_ENABLE 1

#if LOG_ENABLE
#define LOG(level, msg) \
  do { \
    Serial.print("["); Serial.print(level); Serial.print("] "); \
    Serial.print(__FUNCTION__); Serial.print("(): "); \
    Serial.println(msg); \
  } while (0)
#else
#define LOG(level, msg)
#endif

#define LOGI(msg) LOG("INF", msg)
#define LOGD(msg) LOG("DBG", msg)
#define LOGW(msg) LOG("WRN", msg)
#define LOGE(msg) LOG("ERR", msg)

/* ================= PROGRAM BUFFER ================= */

String program[MAX_LINES];
int programSize = 0;
int ip = 0;

enum ExecState { REPL, LOADING, RUNNING };
ExecState state = REPL;

/* ================= VARIABLES ================= */

struct Var {
  String name;
  long value;
};

Var vars[MAX_VARS];
int varCount = 0;

/* ================= CONTROL STACKS ================= */

struct IfFrame {
  bool execute;
  bool elseSeen;
};

struct WhileFrame {
  int startIp;
  String a, op, b;
};

IfFrame ifStack[MAX_STACK];
int ifTop = -1;

WhileFrame whileStack[MAX_STACK];
int whileTop = -1;

/* ================= DELAY ================= */

bool waiting = false;
unsigned long waitStart = 0;
long waitMs = 0;

/* ================= UTILITIES ================= */

int tokenize(String line, String out[], int maxT) {
  LOGD("tokenize input: " + line);
  int c = 0, s = 0;
  line.trim();
  for (int i = 0; i <= line.length(); i++) {
    if (i == line.length() || line[i] == ' ') {
      if (i > s && c < maxT) {
        out[c++] = line.substring(s, i);
        LOGD("token[" + String(c - 1) + "] = " + out[c - 1]);
      }
      s = i + 1;
    }
  }
  LOGD("token count = " + String(c));
  return c;
}

long getVar(const String& name) {
  LOGD("getVar: " + name);
  for (int i = 0; i < varCount; i++) {
    if (vars[i].name == name) {
      LOGD("found " + name + " = " + String(vars[i].value));
      return vars[i].value;
    }
  }
  LOGW("variable not found, returning 0");
  return 0;
}

long resolve(const String& t) {
  LOGD("resolve token: " + t);
  if (t.length() == 0) return 0;
  if (isDigit(t[0]) || t[0] == '-') {
    long v = t.toInt();
    LOGD("numeric literal -> " + String(v));
    return v;
  }
  return getVar(t);
}

void setVar(const String& name, long val) {
  LOGI("set " + name + " = " + String(val));
  for (int i = 0; i < varCount; i++) {
    if (vars[i].name == name) {
      vars[i].value = val;
      return;
    }
  }
  if (varCount < MAX_VARS) {
    vars[varCount++] = { name, val };
  } else {
    LOGE("variable table full");
  }
}

void printToken(const String& t)
{
  // String literal
  if (t.startsWith("\"") && t.endsWith("\"") && t.length() >= 2) {
    Serial.print(t.substring(1, t.length() - 1));
    return;
  }

  // Number or variable
  Serial.print(resolve(t));
}


void setRGBColor(int pin, const char* color) {
  if      (!strcmp(color, "red"))     rgbLedWrite(pin, 255, 0, 0);
  else if (!strcmp(color, "green"))   rgbLedWrite(pin, 0, 255, 0);
  else if (!strcmp(color, "blue"))    rgbLedWrite(pin, 0, 0, 255);
  else if (!strcmp(color, "yellow"))  rgbLedWrite(pin, 255, 255, 0);
  else if (!strcmp(color, "cyan"))    rgbLedWrite(pin, 0, 255, 255);
  else if (!strcmp(color, "magenta")) rgbLedWrite(pin, 255, 0, 255);
  else if (!strcmp(color, "white"))   rgbLedWrite(pin, 255, 255, 255);
  else                                rgbLedWrite(pin, 0, 0, 0);
}

bool evalCond(const String& a, const String& op, const String& b) {
  long x = resolve(a);
  long y = resolve(b);
  LOGI("condition check: " + String(x) + " " + op + " " + String(y));

  if (op == "==") return x == y;
  if (op == "!=") return x != y;
  if (op == "<")  return x < y;
  if (op == ">")  return x > y;
  if (op == "<=") return x <= y;
  if (op == ">=") return x >= y;

  LOGE("unknown operator");
  return false;
}

bool handleDelay(long ms) {

  if (!waiting) {
    LOGI("delay start " + String(ms) + "ms");
    waitStart = millis();
    waitMs = ms;
    waiting = true;
  }

  if (millis() - waitStart >= (unsigned long)waitMs) {
    LOGI("delay finished");
    waiting = false;
    return true;
  }
  LOGD("delay running");
  return false;
}

/* ================= EXECUTION ================= */

bool executeLine(const String& line) {
  LOGI("EXEC IP=" + String(ip) + " : " + line);

  String t[6];
  int n = tokenize(line, t, 6);
  if (n == 0) return true;

  /* IF */
  if (t[0] == "if") {
    bool r = evalCond(t[1], t[2], t[3]);
    ifStack[++ifTop] = { r, false };
    LOGI(String("IF result = ") + (r ? "TRUE" : "FALSE"));
    return true;
  }

  /* ELSE */
  if (t[0] == "else") {
    if (ifTop >= 0 && !ifStack[ifTop].elseSeen) {
      ifStack[ifTop].execute = !ifStack[ifTop].execute;
      ifStack[ifTop].elseSeen = true;
      LOGI("ELSE toggled execution");
    }
    return true;
  }

  /* END */
  if (t[0] == "end") {
    if (ifTop >= 0) {
      LOGI("END IF");
      ifTop--;
    } else {
      LOGE("END without IF");
    }
    return true;
  }

  /* WHILE */
  if (t[0] == "while") {
    bool cond = evalCond(t[1], t[2], t[3]);
    if (!cond) {
      LOGI("WHILE false → skipping loop");
      int depth = 1;
      while (++ip < programSize && depth) {
        if (program[ip].startsWith("while")) depth++;
        if (program[ip] == "endw") depth--;
      }
    } else {
      LOGI("WHILE entered");
      whileStack[++whileTop] = { ip, t[1], t[2], t[3] };
    }
    return true;
  }

  /* ENDW */
  if (t[0] == "endw") {
    if (whileTop >= 0) {
      auto &w = whileStack[whileTop];
      if (evalCond(w.a, w.op, w.b)) {
        LOGI("WHILE repeat → jump back");
        ip = w.startIp;
      } else {
        LOGI("WHILE exit");
        whileTop--;
      }
    }
    return true;
  }

  /* SKIP IF FALSE */
  if (ifTop >= 0 && !ifStack[ifTop].execute) {
    LOGD("skipped due to IF=false");
    return true;
  }

  /* COMMANDS */
  if (t[0] == "set") {
    setVar(t[1], resolve(t[2]));
  }

  /* PRINT */
  else if (t[0] == "print")
  {
    if (n < 2) {
      Serial.println();
      return true;
    }

    for (int i = 1; i < n; i++) {
      printToken(t[i]);
      if (i < n - 1) Serial.print(" ");
    }
    Serial.println();
  }


  /* INC */
  else if (t[0] == "inc")
  {
    if (n < 2) {
      LOGE("inc: missing variable name");
      return true;
    }

    long increment = 1;   // default

    if (n >= 3) {
      increment = resolve(t[2]);  // supports negative & variables
    }

    long v = getVar(t[1]);
    setVar(t[1], v + increment);

    LOGI("inc " + t[1] + " by " + String(increment) +
         " -> " + String(v + increment));
  }


  else if (t[0] == "delay")
  {
    return handleDelay(resolve(t[1]));
  }

  else if (t[0] == "rgb") {
    setRGBColor(t[1].toInt(), t[2].c_str());
  }

  return true;
}

/* ================= RUNNER ================= */

void runProgram() {
  if (state != RUNNING) return;

  LOGD("runProgram IP=" + String(ip));

  if (ip >= programSize) {
    LOGI("program finished");
    state = REPL;
    return;
  }

  if (executeLine(program[ip]))
  {
    ip++;
  }
}

/* ================= SERIAL ================= */

void handleSerial() {
  static String line;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      line.trim();

      if (line == "load") {
        programSize = 0;
        state = LOADING;
        LOGI("program loading started");
      }
      else if (line == "endprog") {
        state = REPL;
        LOGI("loading finished");
      }
      else if (line == "run") {
        ip = 0;
        state = RUNNING;
        LOGI("program running");
      }
      else if (state == LOADING) {
        if (programSize < MAX_LINES) {
          program[programSize++] = line;
          LOGD("buffered line " + String(programSize - 1));
          LOGD("with " + line);
        }
        else {
          LOGE("program buffer full");
        }
      }

      else if (state == REPL) {
        LOGI("REPL exec: " + line);
        executeLine(line);
      }

      line = "";
      Serial.print(">>> ");
    } else {
      line += c;
    }
  }
}

/* ================= SETUP / LOOP ================= */

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BUFFER-FIRST SCRIPT ENGINE (FULL DEBUG) ===");
  Serial.println("Commands: begin | endprog | run");
  Serial.print(">>> ");
}

void loop() {
  handleSerial();
  runProgram();
}
