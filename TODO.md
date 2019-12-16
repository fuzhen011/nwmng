# ToDo List

## Architecture

- [] Config file format
  - [] JSON - Bluetooth SIG JSON device database format - search in SF
- [] Unix domain socket
  - [] utils level handler

## CLI

- [] Go through the bluez to learn how the arguments completion works
- [] Input prompt - when blacklisting or removing typed, prompt to user to
  confirm - refer the implementation in bluez
- [] Tab completion
  - [x] Complete command name
  - [x] Complete command parameters
- [x] History
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
  - [] Device database as a hash table, key is the string of address e.g.
    "0x0003", value is the structure of device item.
- [] Save when the nwmng configured itself for the last time, so it can know if
  the config changes through comparing last modification time with it.

## Utils

### Logging

- [x] Level - when tail, could "grep -v" to exclude the unwanted log on level(s)
- [x] Time
- [x] File and line
- [x] Information

### Error Code

- [x] Porting the error code design from btmesh
  - [x] Porting source file names python script

## Tools

- [] shell script to view log with level filter

## UTest

- [] basic porting
