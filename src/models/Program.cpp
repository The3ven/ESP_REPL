#include "Program.h"
#include <string.h>

void Program::clear() {
  size = 0;
  sourceIdx = 0;
  funcCount = 0;
}

void Program::addLine(const char *line) {
  if (!line)
    return;
  int len = strlen(line);
  if (size < MAX_LINES && sourceIdx + len + 1 < MAX_SOURCE_SIZE) {
    char *start = &source[sourceIdx];
    strcpy(start, line);
    sourceIdx += len;
    source[sourceIdx++] = 0; // null terminator

    lines[size].ptr = start;
    lines[size].len = len;
    size++;
  }
}

bool Program::replaceLine(int id, const char *line) {
  if (id < 0 || id >= size || !line)
    return false;
  int len = strlen(line);
  if (sourceIdx + len + 1 < MAX_SOURCE_SIZE) {
    char *start = &source[sourceIdx];
    strcpy(start, line);
    sourceIdx += len;
    source[sourceIdx++] = 0;
    lines[id].ptr = start;
    lines[id].len = len;
    return true;
  }
  return false;
}

bool Program::insertLine(int id, const char *line) {
  if (id < 0 || id > size || !line || size >= MAX_LINES)
    return false;
  int len = strlen(line);
  if (sourceIdx + len + 1 < MAX_SOURCE_SIZE) {
    for (int i = size; i > id; i--) {
      lines[i] = lines[i - 1];
    }
    char *start = &source[sourceIdx];
    strcpy(start, line);
    sourceIdx += len;
    source[sourceIdx++] = 0;
    lines[id].ptr = start;
    lines[id].len = len;
    size++;
    return true;
  }
  return false;
}

bool Program::deleteLine(int id) {
  if (id < 0 || id >= size)
    return false;
  for (int i = id; i < size - 1; i++) {
    lines[i] = lines[i + 1];
  }
  size--;
  return true;
}

void Program::scanFuncs() {
  funcCount = 0;
  for (int i = 0; i < size; i++) {
    // lines[i] is Span
    if (lines[i].len > 5 && lines[i].ptr[0] == 'f' && lines[i].ptr[1] == 'u' &&
        lines[i].ptr[2] == 'n' && lines[i].ptr[3] == 'c' &&
        lines[i].ptr[4] == ' ') {
      // "func name"
      const char *nameStart = lines[i].ptr + 5;
      int nameLen = lines[i].len - 5;
      if (funcCount < MAX_FUNCS) {
        funcs[funcCount++] = {{nameStart, nameLen}, i + 1};
      }
    }
  }
}

int Program::findFunc(const Span &name) {
  for (int i = 0; i < funcCount; i++) {
    if (name.len == funcs[i].name.len &&
        strncmp(name.ptr, funcs[i].name.ptr, name.len) == 0)
      return funcs[i].ip;
  }
  return -1;
}
