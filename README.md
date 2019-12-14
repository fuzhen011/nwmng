[![Build status](https://img.shields.io/badge/Build-MacOS-brightgreen)](www.baidu.com)

---

## Architecture

[ProcessOn](https://www.processon.com/diagraming/5a581bfae4b0332f15299433)

3 parts: CLI, MNG, CFG

These 3 parts can be completely separated.

### IPC Strategy

Unix domain socket.

Header types:

| Header | Description    |
| ------ | -------------- |
| 0x01   | Command Start  |
| 0x02   | Response       |
| 0x03   | Command End    |
| 0x04   | Anonmous Event |

IPC between CLI and MNG.

It's always the CLI process which sends the "Command Start", then it waits for "Response" and "Command End" either in blocking or non-blocking mode.

**REQs:**

- [] Config file format
  - [] JSON - Bluetooth SIG JSON device database format

## CLI

The command line interface process, which receives commands from user and outputs the status.

## MNG

## CFG

## Utils

This part can be used by any other parts as utils.

### Logging

Logging has the level feature which is inspired from Android logging system.

| Key word | Meaning | Note                                          |
| -------- | ------- | --------------------------------------------- |
| AST      | assert  | Assert, call assert(0) directly, NON-MASKABLE |
| ERR      | error   |                                               |
| WRN      | warning |                                               |
| MSG      | message |                                               |
| DBG      | debug   |                                               |
| VER      | verbose |                                               |

Format: \[Time\]\[File:Line]\[Level\]: Log Message...  
\[2019-12-12 21:22:33\]\[xxx_source_xxx.c:225][MSG]: Initializing...

![Logging](doc/pic/logging.png)
