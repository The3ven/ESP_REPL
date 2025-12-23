import re
import sys

def convert_line(line):
    line = line.strip()

    # ignore empty & braces
    if not line or line in ["{", "}"]:
        return None

    # int x = 5;
    m = re.match(r'int\s+(\w+)\s*=\s*(-?\d+);', line)
    if m:
        return f"set {m.group(1)} {m.group(2)}"

    # x = 5;
    m = re.match(r'(\w+)\s*=\s*(-?\d+);', line)
    if m:
        return f"set {m.group(1)} {m.group(2)}"

    # delay(1000);
    m = re.match(r'delay\s*\(\s*(\d+)\s*\)\s*;', line)
    if m:
        return f"delay {m.group(1)}"

    # rgb(38, red);
    m = re.match(r'rgb\s*\(\s*(\d+)\s*,\s*(\w+)\s*\)\s*;', line)
    if m:
        return f"rgb {m.group(1)} {m.group(2)}"

    # if (a > b)
    m = re.match(r'if\s*\(\s*(\w+)\s*(==|!=|<=|>=|<|>)\s*(\w+)\s*\)', line)
    if m:
        return f"if {m.group(1)} {m.group(2)} {m.group(3)}"

    # while (a < b)
    m = re.match(r'while\s*\(\s*(\w+)\s*(==|!=|<=|>=|<|>)\s*(\w+)\s*\)', line)
    if m:
        return f"while {m.group(1)} {m.group(2)} {m.group(3)}"

    # else
    if line.startswith("else"):
        return "else"

    # end of if
    if line == "}":
        return "end"

    return None


def convert_file(src):
    out = []
    for line in src:
        converted = convert_line(line)
        if converted:
            out.append(converted)
    return out


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python cpp_to_script.py input.cpp")
        sys.exit(1)

    with open(sys.argv[1]) as f:
        script = convert_file(f.readlines())

    print("begin")
    for l in script:
        print(l)
    print("endprog")
    print("run")
