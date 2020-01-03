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
- [] Separate CLI and MNG to 2 threads
  - [] Move all the socket related function to mng component

## MNG

- [x] Configure itself.
- [] Adding/Configuring/Removing/Blacklisting devices.
  - [] clearing the fail list actually moves all the nodes to the config list
- [] Set light status.
  - [] Set light onoff.
  - [] Set light lightness.
  - [] Set light color temperature.
- [] Whenever start provision a new device, check if it's in backlog, if yes,
  delete it;
- [x] backoff sync NCP with timeout

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
    - [x] clear all control fields (key: NULL)
    - [x] addr (key: NULL)
    - [x] ivi (key: NULL)
    - [x] sync time (key: NULL)
    - [x] netkey id (key: NULL)
    - [x] netkey done flag (key: NULL)
    - [x] appkey id (key: refid)
    - [x] appkey done flag (key: refid)
- [] Write node not only write json file also modify the value in the hash table

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
- [] Python script to generate the returns for function - get_error_str

## Tools

- [] shell script to view log with level filter

## UTest

- [] basic porting

## IPC

- [] address already in use problem when backoff connect to server
