# ToDo List

## CLI

- [] Tab completion
  - [x] Complete command name
  - [] Complete command parameters
- [x] History
  - [x] Only save correct ones
- [] Synchronized command - block until the command ends normally or unexpectedly.
- [] Trap Ctrl-C to end all operations.

## MNG

- [] Configure itself.
- [] Adding/Configuring/Removing/Blacklisting devices.
- [] Set light status.
  - [] Set light onoff.
  - [] Set light lightness.
  - [] Set light color temperature.

## CFG

- [] Read node by "key"
- [] Save when the nwmng configured itself for the last time, so it can know if
  the config changes through comparing last modification time with it.

## Utils

### Logging

- [] Level - when tail, could "grep -v" to exclude the unwanted log on level(s)
- [x] Time
- [] File and line
- [] Information

## Tools

- [] shell script to view log with level filter

## DONE

- [x] Porting the error code design from btmesh
  - [x] Porting source file names python script
