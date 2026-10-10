<div align="center">

<h1>Sanchar</h1>

<h3>Bluetooth-Based Offline Chat for Windows</h3>

Team ID: DSCPP-III-2026-T344<br>
Department of Computer Science &amp; Engineering, Graphic Era (Deemed to be University), Dehradun

</div>

---

## Overview

Sanchar is a console chat application for Windows. Two nearby laptops that have been paired over Bluetooth can exchange text messages with **no internet, Wi-Fi or mobile data**. Each conversation is saved to a local text file on each laptop, so history is available after restarting the program.

It is built in **C++** (application, Bluetooth and chat logic) and **C** (data-structure library), using the Windows Winsock2 Bluetooth (RFCOMM) API.

---

## Features

- Scan nearby Bluetooth devices and show which are paired.
- Pair with a device using Windows Bluetooth authentication (MITM-protected bonding).
- Host a chat (waits for a paired laptop; Esc cancels) or connect to a laptop that is hosting.
- Send and receive printable ASCII messages of 1 to 1023 characters.
- Messages are timestamped and saved per device in `history/<address>.txt`.
- Chat commands: `/history` shows saved messages, `/quit` leaves the chat.
- View saved history for a device from the main menu.
- **Practice mode** (`--demo`): simulated replies on one laptop, no Bluetooth needed.

---

## Project Structure

```
sanchar/
├── main.cpp          Application entry point, menu and chat screen
├── bluetooth.hpp     Bluetooth declarations (Device, BluetoothConnection, BluetoothManager)
├── bluetooth.cpp     Bluetooth implementation (scan, pair, host, connect, send/receive)
├── chat.hpp          Message validation, MessagePump, ChatHistory (file-backed)
├── connection.hpp    Abstract ConnectionHandler interface
├── demo.hpp          DemoConnection: simulated peer for practice mode
├── dsa.h             Data-structure declarations (C API)
├── dsa.c             Data-structure implementation (queue, linked list, sort, search)
└── README.md         This file
```

Generated at run time (not part of the source):

```
history/              Created automatically: one text file per peer, plus demo.txt
sanchar.exe           The compiled program
```

---

## File-by-File Description and Requirement

<table>
  <thead>
    <tr>
      <th align="left">File</th>
      <th align="left">What it does</th>
      <th align="left">Required?</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>main.cpp</code></td>
      <td>Starts Winsock, shows the menu (scan, pair, host, connect, history, exit), runs the chat screen and reads keyboard input. Handles <code>--demo</code>.</td>
      <td><b>Required</b> - program entry point</td>
    </tr>
    <tr>
      <td><code>bluetooth.hpp</code></td>
      <td>Declares <code>Device</code>, <code>BluetoothConnection</code> (a <code>ConnectionHandler</code> over a Bluetooth socket) and <code>BluetoothManager</code> (scan, pair, host, connect).</td>
      <td><b>Required</b> for real Bluetooth chat</td>
    </tr>
    <tr>
      <td><code>bluetooth.cpp</code></td>
      <td>Implements the Windows Bluetooth calls: <code>BluetoothFindFirstDevice</code> (scan), <code>BluetoothAuthenticateDeviceEx</code> (pair), <code>WSASetServiceW</code> (advertise the service), <code>connect()</code> by service UUID, and non-blocking socket I/O. Holds the shared service UUID.</td>
      <td><b>Required</b> for real Bluetooth chat</td>
    </tr>
    <tr>
      <td><code>chat.hpp</code></td>
      <td><code>Message</code> checks text. <code>MessagePump</code> sends queued messages a few bytes at a time, reads incoming bytes, splits them into lines and queues complete messages. <code>ChatHistory</code> loads and appends the per-device history file.</td>
      <td><b>Required</b> - core chat logic</td>
    </tr>
    <tr>
      <td><code>connection.hpp</code></td>
      <td>Defines the abstract <code>ConnectionHandler</code> interface (<code>sendBytes</code>, <code>receiveBytes</code>), so chat code works with a real socket or the demo connection.</td>
      <td><b>Required</b> - shared interface</td>
    </tr>
    <tr>
      <td><code>demo.hpp</code></td>
      <td><code>DemoConnection</code> implements <code>ConnectionHandler</code> in memory and echoes "[SIMULATED] Received: ...".</td>
      <td><b>Required to build</b>. Needed only for practice mode. Included by <code>main.cpp</code>.</td>
    </tr>
    <tr>
      <td><code>dsa.h</code></td>
      <td>C header with the data types (<code>ChatRecord</code>, <code>MessageQueue</code>, <code>HistoryNode</code>, <code>History</code>, <code>DeviceRecord</code>) and function prototypes. Wrapped in <code>extern "C"</code>. Limits: <code>TEXT_SIZE</code> 1024, <code>QUEUE_SIZE</code> 32, <code>DEVICE_LIMIT</code> 64.</td>
      <td><b>Required</b></td>
    </tr>
    <tr>
      <td><code>dsa.c</code></td>
      <td>Circular queue (O(1) push/pop), singly linked list with tail pointer (O(1) append), insertion sort for devices, and linear search for duplicate devices.</td>
      <td><b>Required</b> - compiled as C</td>
    </tr>
  </tbody>
</table>

All eight source files are needed to build the full application. `main.cpp` includes every header, so none can be removed without changing `main.cpp`.

---

## Workflow of the Code

### 1. Start-up (`main.cpp`)
1. `main` checks for `--demo`. If given, it runs the practice chat with `DemoConnection` and exits when the chat ends.
2. Otherwise it starts Winsock (`WSAStartup`), creates the `history/` folder and shows the menu.

### 2. Scan and list devices (option 1)
`BluetoothManager::scan` calls `BluetoothFindFirstDevice` and `BluetoothFindNextDevice`. For each device it skips duplicates using `devices_find` (linear search in `dsa.c`) and stores the address, UTF-8 name and paired state in a `DeviceRecord`. The list is sorted by name with `devices_sort` (insertion sort in `dsa.c`) and shown as a numbered list.

### 3. Pair a device (option 2)
The user picks a device number. `chooseDevice` validates it, and `BluetoothManager::pair` calls `BluetoothAuthenticateDeviceEx`, which opens the Windows pairing prompt. Pairing is required before chat.

### 4. Host or connect (options 3 and 4)
- **Host:** `waitForPeer` creates an RFCOMM socket with authentication required (`SO_BTH_AUTHENTICATE`), binds and listens, and advertises the chat service with `WSASetServiceW` using the shared UUID. It polls with `select()` and returns when a peer connects. Esc cancels.
- **Connect:** `connectTo` creates the same kind of socket and connects to the chosen device by service UUID (port 0, so Windows finds the service).

Both paths return a socket, which is wrapped in `BluetoothConnection`. That class switches the socket to non-blocking mode.

### 5. Chat session (`chat` in `main.cpp`)
1. `ChatHistory` loads the saved file for that peer (`history/<address>.txt`, or `demo.txt`) into a linked list and opens the file for appending.
2. A `MessagePump` is created around the `ConnectionHandler`.
3. The loop repeats every 5 ms:
   - **Send:** on Enter, `MessagePump::enqueue` validates the text (`Message`) and pushes it into the outgoing circular queue. `tick()` sends one message at a time; if only part is sent, the offset is remembered and the rest goes on the next tick.
   - **Receive:** `tick()` reads up to 32 bytes. Each byte must be printable ASCII; a newline (`\n`) ends a message. Complete messages go into the incoming queue.
   - **Display and save:** when a message is fully sent or received, `onMessage` appends it to the history file and prints it with a timestamp.
   - **Keyboard:** typed characters are echoed, Backspace edits, `/history` reprints saved messages and `/quit` ends the chat.
4. If the connection fails or invalid data arrives, the chat ends and the number of unconfirmed outgoing messages is reported.

### 6. View history (option 5)
Loads the peer's history file and prints every saved message.

### Data flow summary

```
Keyboard
   |
   v
main.cpp (chat) --> Message (validation) --> outgoing queue (dsa.c)
   |
   v
MessagePump --> ConnectionHandler --> Bluetooth socket (or DemoConnection)

Bluetooth socket --> MessagePump (newline framing) --> incoming queue (dsa.c)
   |
   v
onMessage --> ChatHistory (history/<address>.txt) and console
```

---

## Requirements

**Hardware**
- Two Windows laptops, each with a working Bluetooth adapter (for real chat).
- Practice mode needs only one laptop and no Bluetooth.

**Software**
- Windows 10 or later (uses Winsock Bluetooth, the Windows Bluetooth API and `conio.h`; Linux and macOS are not supported).
- A C/C++ compiler for Windows: Visual Studio (MSVC) or MinGW-w64 (g++/gcc). C++17 is recommended.
- Windows SDK headers (`bluetoothapis.h`, `ws2bth.h`), included with Visual Studio or MinGW-w64.
- Libraries to link: `ws2_32` (Winsock) and `Bthprops` (Bluetooth API).

---

## Building

> These commands are written from the source code. Confirm them on your own machine before submission.

**Option A: Visual Studio Developer Command Prompt (MSVC)**

```bat
cl /std:c++17 /EHsc /W3 main.cpp bluetooth.cpp dsa.c ws2_32.lib Bthprops.lib /Fe:sanchar.exe
```

**Option B: MinGW-w64**

```bat
gcc -c dsa.c -o dsa.o
g++ -std=c++17 -c main.cpp -o main.o
g++ -std=c++17 -c bluetooth.cpp -o bluetooth.o
g++ main.o bluetooth.o dsa.o -lws2_32 -lBthprops -o sanchar.exe
```

`dsa.c` is compiled with `gcc` so it is treated as C.

---

## Usage

### Real chat (two laptops)
1. On both laptops, turn Bluetooth on and open **Settings > Bluetooth & devices**.
2. Pair the two laptops in Windows, or use option 2 in Sanchar on one laptop and accept the prompt on both.
3. On laptop A, run `sanchar.exe`, choose **3** (Host a chat) and wait.
4. On laptop B, run `sanchar.exe`, choose **1** (Scan), note the number of laptop A, choose **4** (Connect) and enter that number.
5. Type a message and press Enter. Use `/history` to view saved messages and `/quit` to leave.

Both laptops save their own history under `history/`.

### Practice mode (one laptop, no Bluetooth)

```bat
sanchar.exe --demo
```

Messages are answered by a simulated reply. History is saved to `history/demo.txt`.

---

## Input Rules

- **Messages:** 1 to 1023 characters, printable ASCII only (characters 32 to 126). Non-English characters (for example Hindi or emoji) are rejected.
- **Device choice:** the number shown in the scan list.
- **Esc** cancels hosting.

---

## Known Limitations

- **Windows only.** The Bluetooth API and `conio.h` are Windows-specific.
- **One-to-one chat only.** A host accepts one client; there are no group chats.
- **Peer name is not exchanged.** The other laptop appears as `Peer <address>`.
- **"Sent" means handed to Bluetooth.** It is not a read or delivery receipt.
- **Keyboard polling.** Input is read with `_kbhit`/`_getch`, so the chat screen refreshes every 5 ms rather than blocking.
- **Plain-text history.** Files are not encrypted; anyone with access to the folder can read them.
- **Fixed limits.** 32 messages can be queued in each direction, and up to 64 devices are listed.
- **Device list is not refreshed automatically.** Scan again to update paired status.
- **Bluetooth range and adapter support** depend on the laptop's hardware and drivers.

---

## Troubleshooting

<table>
  <thead>
    <tr>
      <th align="left">Problem</th>
      <th align="left">Likely cause and fix</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>"Scan failed"</td><td>Bluetooth is off or the adapter is missing. Turn it on and try again.</td></tr>
    <tr><td>"Pairing failed/cancelled"</td><td>Pair the device in Windows Bluetooth Settings, then scan again.</td></tr>
    <tr><td>"Connect (is the other laptop hosting Sanchar?)"</td><td>The other laptop must choose option 3 first and be paired.</td></tr>
    <tr><td>"Bluetooth socket failed"</td><td>Check Bluetooth is on and that the Windows Bluetooth API is available.</td></tr>
    <tr><td>"Cannot open history file"</td><td>Run from a folder where you can create <code>history/</code>.</td></tr>
    <tr><td>Build errors about <code>bluetoothapis.h</code> or <code>ws2bth.h</code></td><td>Install the Windows SDK, or use Visual Studio with the C++ desktop tools.</td></tr>
  </tbody>
</table>

---

## Course Concepts Used

- **Data Structures in C (`dsa.c/h`):** circular queue, singly linked list, insertion sort, linear search.
- **OOP in C++:** classes, abstract interface `ConnectionHandler`, inheritance (`BluetoothConnection`, `DemoConnection`), RAII-style socket cleanup in destructors.
- **Operating Systems:** socket-based inter-process communication over Bluetooth RFCOMM, non-blocking sockets, `select()` polling.

---

## Team

| Name | ID |
|------|----|
| Jain, Divya | 2510010430 |
| Pratap Singh, Abhay | 2510015287 |
| Awasthi, Prince | 2510016733 |
| Kishor Shah, Dhirendra | 2510011409 |

**Mentor:** Mr. Chitransh Bose, Assistant Professor, CSE

---

<p align="center">
  Sanchar &middot; Department of Computer Science &amp; Engineering, Graphic Era (Deemed to be University) &middot; Academic Session 2026&ndash;27
</p>