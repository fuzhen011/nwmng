[![Build status](https://img.shields.io/badge/Build-MacOS-brightgreen)](www.baidu.com)

---

## Architecture

[ProcessOn](https://www.processon.com/diagraming/5a581bfae4b0332f15299433)

3 parts: CLI, MNG, CFG

### IPC Strategy

Unix domain socket.

## CLI

The command line interface process, which receives commands from user and outputs the status.

**REQs:**

- [] Tab completion
  - [x] Complete command name
  - [] Complete command parameters
- [x] History
  - [x] Only save correct ones
- [] Synchronized command - block until the command ends normally or unexpectedly.
- [] Trap Ctrl-C to end all operations.

## MNG

**REQs:**

- [] Configure itself.
- [] Adding/Configuring/Removing/Blacklisting devices.
- [] Set light status.
  - [] Set light onoff.
  - [] Set light lightness.
  - [] Set light color temperature.
- [] Logging, including below items
  - Level - when tail, could "grep -v" to exclude the unwanted log on level(s)
  - Time
  - File and line
  - Information

## CFG

**REQs:**

- [] Read node by "key"
- [] Save when the nwmng configured itself for the last time, so it can know if
  the config changes through comparing last modification time with it.
