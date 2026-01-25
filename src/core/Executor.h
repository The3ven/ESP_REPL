#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../Config.h"
#include "../context/Context.h"
#include "../utils/Span.h"

class Executor {
private:
  static int precedence(char op);
  static long applyOp(long a, long b, char op);
  static long resolve(Context &ctx, const Span &t);
  static long evalExpr(Context &ctx, Span tokens[], int count);
  static void formatPath(const char *input, char *output);

public:
  static bool execFlow(Context &ctx, Span t[], int n);
  static bool execIO(Context &ctx, Span t[], int n);
  static bool execSys(Context &ctx, Span t[], int n);
  static bool execLang(Context &ctx, Span t[], int n);
  static bool executeLine(Context &ctx, const char *line);
};

#endif
