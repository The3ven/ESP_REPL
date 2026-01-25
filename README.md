# ESP_REPL

A powerful, zero-allocation REPL (Read-Eval-Print Loop) for ESP32 microcontrollers.
Designed for rapid prototyping, runtime scripting, and hardware automation without reflashing.

## 🚀 Key Features

- **Zero-Allocation Architecture**: Built entirely without `String` to prevent heap fragmentation. Stable for 24/7 operation.
- **Multitasking Scheduler**: true concurrency with `bg` (background) tasks. Run non-blocking loops alongside the REPL.
- **Advanced Control Flow**: unified stack supporting nesting of `if/else/endif`, `while/endw`, and `for/next`.
- **Integrated File System**: Persistent storage using LittleFS. Editor tools included (`edit`, `insert`, `del`).
- **Hardware Suite**: Native commands for GPIO `mode`, `write`, `read`, `pwm`, `adc`, `tone`, `touch`, and `i2c`.
- **Connectivity**: WiFi manager with built-in `wget` (GET) and `post` (POST) commands.
- **Developer Tools**: Syntax validator (`check`), Variable inspector (`vars`), and System monitor (`mem`, `info`, `ps`).

## 📂 Project Structure

```
ESP_REPL/
├── ESP_REPL.ino          # Entry point
├── GUIDE.md              # Complete Language Reference
├── examples/             # Sample scripts & transpiler
├── src/
│   ├── Config.h          # Global settings (Buffer sizes, logging, Pins)
│   ├── core/             # REPL, Executor, Scheduler
│   ├── models/           # Program storage, Variables
│   ├── context/          # Execution context structs
│   └── utils/            # Zero-copy string helpers (Span)
```

## 🛠️ Feature Overview

### Scripting Power

- **Logic**: Strict typed blocks for robust parsing.
- **Math**: Full RPN-style expression evaluator (e.g., `set x + 10 5`).
- **Automation**: `repeat` commands, blocking `wait`, and auto-running `boot.re`.

### Network & IoT

- **WiFi**: Connect dynamically `wifi "SSID" "PASS"`.
- **HTTP**: Interact with APIs directly from the console.
- **Telnet**: Remote remote shell access (optional in `Config.h`).

## 📦 Installation

1.  **Requirements**:
    - Arduino IDE
    - ESP32 Board Package

2.  **Setup**:
    - Open `ESP_REPL.ino` in Arduino IDE.
    - Select your ESP32 board.
    - **Partition Scheme**: Select a scheme with LittleFS (e.g., "Default 4MB with spiffs").
    - Compile and Upload.

3.  **Usage**:
    - Open Serial Monitor at **115200** baud.
    - Type `help` to see basic commands.
    - Refer to `GUIDE.md` for the full command reference.

## ⚙️ Configuration (`src/Config.h`)

Tune the engine to your specific hardware needs:

```cpp
#define MAX_LINES 200        // Max lines per program
#define MAX_VARS 50          // Max distinct variables
#define MAX_TASKS 4          // Max concurrent tasks
#define MAX_BUFFER_SIZE 512  // Input buffer size
#define MAX_SOURCE_SIZE 4096 // Total script storage in RAM
#define ENABLE_TELNET 1      // Enable remote access
```

## 📝 License

Open Source. Feel free to modify and use in your projects.
