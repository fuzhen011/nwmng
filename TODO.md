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
- [] Finish the command execs

## MNG

- [] Configure itself.
- [] Adding/Configuring/Removing/Blacklisting devices.
- [] Set light status.
  - [] Set light onoff.
  - [] Set light lightness.
  - [] Set light color temperature.
- [] Whenever start provision a new device, check if it's in backlog, if yes,
  delete it;

## CFG

- [x] Add both timeout for configuring normal nodes and lpns to prov.json
- [] Consider the pos and cons of replacing the hash table by the balanced
  binary tree
- [x] Read node by "key"
  - [x] Device database as a hash table, key is the hex format address e.g.
        0x0003, value is the structure of device item.
- [] Save when the nwmng configured itself for the last time, so it can know if
  the config changes through comparing last modification time with it.
- [] Config File loaders

  - [x] TTL
  - [] Features
    - [] Low Power
    - [] Proxy
    - [] Friend
    - [] Relay and its setting
  - [x] Pub
  - [x] Secure network beacn
  - [x] Tx parameters
  - [x] Binding Appkeys
  - [x] Subscribe from

- [] IPC - Config file writes
  - [] nwk.json
    - [] node address (key: UUID)
    - [] node errbits (key: address)
    - [] node rmorbl (key: address)
    - [] node done (key: address)
    - [x] add node to backlog
  - [] prov.json
    - [] clear all control fields (key: NULL)
    - [] addr (key: NULL)
    - [] sync time (key: NULL)
    - [] netkey id (key: NULL)
    - [] netkey done flag (key: NULL)
    - [] appkey id (key: refid)
    - [] appkey done flag (key: refid)

## Utils

- [x] define ASSERT, ASSERT_MSG

### Logging

- [x] Level - when tail, could "grep -v" to exclude the unwanted log on level(s)
- [x] Time
- [x] File and line
- [x] Information
- [] file length and line number length settable

### Error Code

- [x] Porting the error code design from btmesh
  - [x] Porting source file names python script

## Tools

- [] shell script to view log with level filter

## UTest

- [] basic porting
