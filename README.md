# P2PMessaging
A cross-platform CLI application for direct peer-to-peer messaging &amp; File Transfer in C

## Features
- **Local Chat History:** Automatically logs sent/received messages and file transfers locally on your computer in dedicated text files (`Hof_<IP>.txt`).

## Prerequisites

- C11 compatible compiler (required for `stdatomic.h`).
- CMake 3.10+
- **Firewall Configuration:** The receiving peer must allow inbound TCP traffic on **port 8080** through their firewall/router to successfully receive messages and files.
## Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build .
```
## Usage
Run the compiled executable:
```bash
# Linux / macOS
./P2PMessagingApp

# Windows
.\P2PMessagingApp.exe
```
## Future Roadmap
**Data Encryption:** Implement encryption for messages and file transfers.

**Protocol Development:** Expand and refine the transmission protocol for better reliability and performance.

**Security Enhancements:** Add spam prevention mechanisms, connection rate limiting, and IP blocking.

**User Interface:** Develop a simple graphical interface (GUI).
