# ESP_REPL Language Guide

This guide covers the syntax and commands for the ESP_REPL v8 scripting language.

## 🖥️ Top-Level Commands

These commands control the environment and are typed directly into the serial console.

| Command            | Description                                                            |
| :----------------- | :--------------------------------------------------------------------- |
| `load`             | Start recording lines into the program buffer. Type `endprog` to stop. |
| `endprog`          | Stop recording mode.                                                   |
| `run`              | Execute the currently loaded program (Task 0).                         |
| `list`             | Display the code currently in memory with line numbers.                |
| `del <id>`         | Delete a specific line number.                                         |
| `insert <id> LINE` | Insert a line at a specific position.                                  |
| `save <path>`      | Save current program to storage (e.g., `save /script.re`).             |
| `load <path>`      | Load program from storage (e.g., `load /script.re`).                   |
| `check`            | Run syntax validator to find error (unmatched blocks).                 |
| `help`             | Show available commands.                                               |
| `reset`            | Clear program memory.                                                  |

## 📜 Script Syntax

The language is line-based. Tokens are separated by spaces.
**Strict Syntax**: Blocks must be closed with specific keywords (`endif`, `endw`, `next`).

### Variables & Math

Variables are created implicitly. Names must be < 16 chars.

```bash
set x 10
set y 20
inc x 1          # x = x + 1
set z + x y      # z = x + y (Prefix Notation for ops if complex?)
                 # Actually: set var val. val can be "op a b".
set a * 5 5      # a = 25
```

_Supported Ops_: `+`, `-`, `*`, `/`, `%`, `&` (AND), `|` (OR), `^` (XOR).

### Control Flow

#### If / Else

```bash
if x < 10
  echo "Small"
else
  echo "Big"
endif
```

#### While Loop

```bash
set i 0
while i < 5
  echo i
  inc i 1
endw
```

#### For Loop

```bash
# for <var> <start> <end>
for i 0 5
  echo i
next
```

#### Repeater

```bash
# repeat <count> <command...>
repeat 5 echo "Hello"
```

#### Delays

```bash
wait 1000    # Wait 1000ms
```

### Functions

```bash
func blink
  write 2 1
  wait 500
  write 2 0
  wait 500
endf

call blink
```

## 🔌 IO & Hardware

| Command | Usage                   | Description                                     |
| :------ | :---------------------- | :---------------------------------------------- |
| `mode`  | `mode <pin> <in/out>`   | Set Pin Mode (`mode 2 out`).                    |
| `write` | `write <pin> <val>`     | Digital Write 1/0 (`write 2 1`).                |
| `read`  | `read <pin> [var]`      | Digital Read. Prints if var omitted (`read 0`). |
| `pwm`   | `pwm <pin> <val>`       | Analog Write (0-255) (`pwm 2 128`).             |
| `aread` | `aread <pin> [var]`     | Analog Read.                                    |
| `tone`  | `tone <pin> <freq>`     | Play tone. `tone <pin> 0` to stop.              |
| `touch` | `touch <pin> [var]`     | Read touch sensor.                              |
| `i2c`   | `i2c <scan/read/write>` | I2C Tools.                                      |

## 📂 File System

| Command | Usage            | Description                                 |
| :------ | :--------------- | :------------------------------------------ |
| `ls`    | `ls`             | List files in root.                         |
| `cat`   | `cat <file>`     | Print file contents.                        |
| `rm`    | `rm <file>`      | Delete file.                                |
| `cp`    | `cp <src> <dst>` | Copy file.                                  |
| `menu`  | `menu`           | Interactive file menu.                      |
| `loadn` | `loadn <n>`      | Load file by menu index (used with `menu`). |

## 🌐 System & Connectivity

| Command        | Description                                         |
| :------------- | :-------------------------------------------------- |
| `wifi "S" "P"` | Connect to WiFi.                                    |
| `ip`           | Print local IP.                                     |
| `rssi`         | Print Signal Strength.                              |
| `wget "U"`     | GET Request to URL.                                 |
| `post "U" "D"` | POST Request (Content-Type: x-www-form-urlencoded). |
| `reboot`       | Restart ESP32.                                      |
| `sleep <n>`    | Deep Sleep for n seconds.                           |
| `mem`          | Show free heap.                                     |
| `info`         | Show system info (CPU, Uptime).                     |
| `vars`         | Dump all variables.                                 |

## 🚀 Multitasking

| Command         | Description                       |
| :-------------- | :-------------------------------- |
| `ps`            | List active tasks.                |
| `bg <func>`     | Run function in background task.  |
| `kill <id>`     | Kill task.                        |
| `alias <n> <c>` | Create alias (e.g. `alias l ls`). |
