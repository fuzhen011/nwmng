<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [Architecture](#architecture)
  - [IPC Strategy](#ipc-strategy)
- [CLI](#cli)
- [MNG](#mng)
- [CFG](#cfg)
  - [What information needs to be stored in config file](#what-information-needs-to-be-stored-in-config-file)
    - [Provisioner](#provisioner)
    - [Network & Nodes](#network--nodes)
- [Utils](#utils)
  - [Logging](#logging)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

[![Build status](https://img.shields.io/badge/Build-MacOS-brightgreen)](www.baidu.com)

---

## Architecture

[ProcessOn](https://www.processon.com/diagraming/5a581bfae4b0332f15299433)

Limitation of this design - Only one subnet is supported, I haven't seen any
requirements for using more than one subnets so I don't want to waste time on
it, leave it to future.

3 parts: CLI, MNG, CFG, CLI and MNG are in the same process while CFG runs on a separated process.

### IPC Strategy

Unix domain socket, network socket.

IPC protocol between cli-mng and cfg processes, where cli-mng as socket client and cfg as socket server, using the basic command-response-event mechanism.

Generally, for cli-mng to synchronize the configuration with cfg, it starts with
one or more commands "set-xxx", followed with a command "execute-yyy".

## CLI

The command line interface process, which receives commands from user and
outputs the status.

Supported commands

Conventions:

- Argument in \[\] means normal argument.
- Argument in &lt;&gt; means that the argument is optional to present, in this
  case, argument(s) probably have default value(s).
- Argument followed by ... means variable number of the argument.
- Content in \(\) following a argument is illustrative.

|  Command  |             Args             | Defaults |       Usage        | Description                                                                              |
| :-------: | :--------------------------: | :------: | :----------------: | ---------------------------------------------------------------------------------------- |
|   sync    |              \               |    \     |        sync        | Synchronize the network configuration with the JSON file                                 |
|   reset   | &lt;1(Factory)/0(Normal)&gt; |    0     |       reset        | Reset the device, if args 1, erase the storage                                           |
|   list    |              \               |    \     |        list        | List all the devices in the provisioner device database, including UUID, Device key etc. |
|     q     |              \               |    \     |         q          | Quit the program                                                                         |
| blacklist |       &lt;addr...&gt;        |    \     | blacklist 0x112c 2 | Blacklisting the specific node(s)                                                        |
|  remove   |       &lt;addr...&gt;        |    \     |  remove 0x120c 2   | Removing the specific node(s)                                                            |

<center>Table x: Network Configuration Commands</center>

|  Command  |           Args            |           Usage            | Description                            |
| :-------: | :-----------------------: | :------------------------: | -------------------------------------- |
|   onoff   |  \[on/off\] \[addr...\]   |   onoff on 0x1203 0x100c   | Set the light onoff status             |
| lightness | \[pecentage\] \[addr...\] | lightness 50 0x1203 0x100c | Set the light lightness status         |
| colortemp | \[pecentage\] \[addr...\] | colortemp 30 0x1203 0x100c | Set the light color temperature status |

<center>Table x: Lighting Control Commands</center>

For example:

lightness 50 0x1203 0x100c - set the lightness of the lights whose address is
0x1203 or 0x100c to 50%

## MNG

## CFG

### What information needs to be stored in config file

#### Provisioner

|         What's it          |    Key    |    Value    | Description      |
| :------------------------: | :-------: | :---------: | ---------------- |
| Last sync time<sup>1</sup> | SyncTime  | time_t TBD  |                  |
|          IV index          |    IVI    |   uint32    |                  |
|            Keys            |    \      |     \       | Table x          |
|   Network Transmit Count   |   TxCnt   |    uint8    | [0, 7]           |
| Network Transmit Interval  |  TxIntv   |    uint8    | [10, 320]@step10 |
|       Added Devices        |  Devices  | uint16array | (0, 0x7fff]      |
|     Publication Groups     | PubGroups | uint16array | [0xc000, 0xfeff] |
|    Subscription Groups     | SubGroups | uint16array | [0xc000, 0xfeff] |

<center>Table x. Provisioner Config File Content</center>

1. By checking the last modification time against last synchronized time to know
   if the configuration is changed out of the program.

|        What's it         |  Key  |      Value      | Description |
| :----------------------: | :---: | :-------------: | ----------- |
| Reference ID<sup>1</sup> | RefId |     uint16      |             |
|        Key Index         |  Id   |     uint16      |             |
|        Key Value         | Value | 16BL uint8array |             |
|  Created successfully?   | Done  |      bool       |             |

<center>Table x. Key Content</center>

1. The read id is allocated when the key is created successfully, however, in most of the cases, configuration of the network happens before it.

#### Network & Nodes

|    What's it    | Key  | Value  | Description |
| :-------------: | :--: | :----: | ----------- |
| Unicast address | Addr | uint16 |             |
|    IV index     | IVI  | uint32 |             |

<center>Table x. Network & Nodes Config File Content</center>

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

<center></center>

Format: \[Time\]\[File:Line]\[Level\]: Log Message...  
\[2019-12-12 21:22:33\]\[xxx_source_xxx.c:225][MSG]: Initializing...

![Logging](doc/pic/logging.png)
