import re
import sys

# Stack to track block types for correct 'end' keywords
block_stack = []

def convert_line(line):
    line = line.strip()
    # Remove comments
    if "//" in line:
        line = line.split("//")[0].strip()

    if not line: return None
    
    # 1. IO Commands
    # pinMode(2, OUTPUT) -> mode 2 out
    m = re.match(r'pinMode\s*\(\s*(\w+)\s*,\s*(OUTPUT|INPUT)\s*\)\s*;', line)
    if m: return f"mode {m.group(1)} {'out' if m.group(2)=='OUTPUT' else 'in'}"

    # digitalWrite(2, HIGH) -> write 2 1
    m = re.match(r'digitalWrite\s*\(\s*(\w+)\s*,\s*(HIGH|LOW|1|0)\s*\)\s*;', line)
    if m:
        val = '1' if m.group(2) in ['HIGH','1'] else '0'
        return f"write {m.group(1)} {val}"

    # analogWrite(2, 128) -> pwm 2 128
    m = re.match(r'analogWrite\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*;', line)
    if m: return f"pwm {m.group(1)} {m.group(2)}"

    # digitalRead(2) -> read 2
    # int x = digitalRead(2); -> read 2 x
    m = re.match(r'(?:int\s+(\w+)\s*=\s*)?digitalRead\s*\(\s*(\w+)\s*\)\s*;', line)
    if m:
        return f"read {m.group(2)} {m.group(1) if m.group(1) else ''}".strip()

    # delay(1000) -> delay 1000
    m = re.match(r'delay\s*\(\s*(\w+)\s*\)\s*;', line)
    if m: return f"delay {m.group(1)}"

    # 2. System Commands
    # WiFi.begin("ssid", "pass") -> wifi "ssid" "pass"
    m = re.search(r'WiFi\.begin\s*\(\s*("[^"]+")\s*,\s*("[^"]+")\s*\)\s*;', line)
    if m: return f"wifi {m.group(1)} {m.group(2)}"
    
    # HTTPClient h; h.begin("url"); h.GET() -> wget "url" (Simplified)
    # Mapping simple h.begin("url") if strictly followed by GET logic is hard, 
    # so we assume a direct `wget("url");` macro style or just h.begin mapping.
    m = re.search(r'wget\s*\(\s*("[^"]+")\s*\)\s*;', line)
    if m: return f"wget {m.group(1)}"

    # Serial.println("msg", var) -> print "msg" var
    # This is complex regex, simplifying to basic print("msg")
    m = re.match(r'(?:Serial\.)?print(?:ln)?\s*\((.*)\)\s*;', line)
    if m:
        # crude split by comma, remove func parens
        parts = m.group(1).split(',')
        return "print " + " ".join([p.strip() for p in parts])

    # 3. Variables & Math
    # int x = 10; -> set x 10
    m = re.match(r'(?:int|long)\s+(\w+)\s*=\s*(.+);', line)
    if m:
        return f"set {m.group(1)} {parse_expr(m.group(2))}"
    
    # x = 10; -> set x 10
    m = re.match(r'(\w+)\s*=\s*(.+);', line)
    if m:
         return f"set {m.group(1)} {parse_expr(m.group(2))}"
         
    # x++; -> inc x 1
    m = re.match(r'(\w+)\+\+\s*;', line)
    if m: return f"inc {m.group(1)} 1"

    # x--; -> inc x -1
    m = re.match(r'(\w+)\-\-\s*;', line)
    if m: return f"inc {m.group(1)} -1"
    
    # x += 5; -> inc x 5
    m = re.match(r'(\w+)\s*\+=\s*(\w+)\s*;', line)
    if m: return f"inc {m.group(1)} {m.group(2)}"

    # 4. Control Flow
    # func loop() {
    m = re.match(r'(?:void|int)\s+(\w+)\s*\(\s*\)\s*\{', line)
    if m:
        block_stack.append('func')
        return f"func {m.group(1)}"

    # while (x < 10) {
    m = re.match(r'while\s*\(\s*(.+)\s*\)\s*\{', line)
    if m:
        block_stack.append('while')
        expr = m.group(1).replace('==', ' == ').replace('!=', ' != ').replace('<', ' < ').replace('>', ' > ')
        return f"while {expr}"

    # if (x < 10) {
    m = re.match(r'if\s*\(\s*(.+)\s*\)\s*\{', line)
    if m:
        block_stack.append('if')
        expr = m.group(1).replace('==', ' == ').replace('!=', ' != ').replace('<', ' < ').replace('>', ' > ')
        return f"if {expr}"

    # for (int i = 0; i < 10; i++)
    # Regex is tricky, assume simple standard loop
    m = re.match(r'for\s*\(\s*(?:int\s+)?(\w+)\s*=\s*(\d+)\s*;\s*\1\s*<\s*(\d+)\s*;\s*\1\+\+\s*\)\s*\{', line)
    if m:
        block_stack.append('for')
        return f"for {m.group(1)} {m.group(2)} {m.group(3)}"

    # else {
    if line.startswith("else") and "{" in line:
        return "else"
    
    # call foo(); -> call foo
    m = re.match(r'(\w+)\s*\(\s*\)\s*;', line)
    if m: return f"call {m.group(1)}"

    # Closing Brace }
    if line == "}":
        if block_stack:
            t = block_stack.pop()
            if t == 'func': return 'endf'
            if t == 'while': return 'endw'
            if t == 'if': return 'endif'
            if t == 'for': return 'next'
        return 'end' # Default fallback

    return None

def parse_expr(expr):
    # defined: 1 + 2 -> + 1 2 prefix? No, language uses infix for single ops usually in set?
    # Guide says: set z + x y.  Wait, guide says: set z + x y
    # So we need to convert infix `1 + 2` to prefix `+ 1 2`
    # Simple single op support:
    expr = expr.strip()
    if '+' in expr:
        parts = expr.split('+')
        return f"+ {parts[0].strip()} {parts[1].strip()}"
    if '-' in expr:
        parts = expr.split('-')
        return f"- {parts[0].strip()} {parts[1].strip()}"
    if '*' in expr:
        parts = expr.split('*')
        return f"* {parts[0].strip()} {parts[1].strip()}"
    if '/' in expr:
        parts = expr.split('/')
        return f"/ {parts[0].strip()} {parts[1].strip()}"
    if '%' in expr:
        parts = expr.split('%')
        return f"% {parts[0].strip()} {parts[1].strip()}"
    return expr

def convert_file(lines):
    out = []
    global block_stack
    block_stack = []
    for line in lines:
        c = convert_line(line)
        if c: out.append(c)
    return out

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python transpiler.py input.cpp")
        sys.exit(1)

    with open(sys.argv[1]) as f:
        script = convert_file(f.readlines())

    for l in script:
        print(l)
