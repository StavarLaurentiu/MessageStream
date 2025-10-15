# MessageStream

Small messaging project (C++ server/subscriber + Python UDP client) used for experimenting with message delivery over UDP/TCP.

This repository contains a simple C++ message server and subscriber, some supporting headers/utilities, a Python-based UDP client with sample payloads, and a small test harness.

## Contents

- `server.cpp` - C++ server program (message producer / broker).
- `subscriber.cpp` - C++ subscriber program (message consumer).
- `headers.h`, `po_tcp.h`, `po_udp.h` - protocol and helper headers.
- `utils.cpp`, `utils.h` - shared helper utilities used by the C++ code.
- `Makefile` - build rules for compiling the C++ binaries.
- `test.py` - small Python test harness (usage depends on your setup).
- `pcom_hw2_udp_client/` - Python UDP client and sample payloads:
  - `udp_client.py` - UDP client used to send sample payloads.
  - `my_payloads.json`, `sample_payloads.json`, `three_topics_payloads.json`, `sample_wildcard_payloads.json` - example JSON payloads.
  - `README.md` - client-specific instructions and notes.

## Quick start

Prerequisites:

- A C++ compiler (g++/clang++) supporting C++11 or later.
- Python 3.8+ for the client and test scripts.

Build the C++ programs using the provided Makefile (recommended):

```sh
# build default targets
make

# or to build specific targets (if present in the Makefile)
# make server subscriber
```

If you don't want to use the Makefile, you can compile manually (example):

```sh
g++ -std=c++11 -O2 server.cpp utils.cpp -o server
g++ -std=c++11 -O2 subscriber.cpp utils.cpp -o subscriber
```

Running the programs

The exact command-line arguments and behavior depend on the implementation in each source file and the `Makefile`. If you need the README updated with exact run examples (ports, flags, and argument order), I can extract and add them from the source. Typical workflows are:

- Start the server (broker) in one terminal.
- Run one or more subscribers to receive messages.
- Use the Python UDP client in `pcom_hw2_udp_client/` to send payloads to the server.

Examples (assumptions: binaries accept a port or default to a hardcoded port):

```sh
# Start server (assumes server listens on a port passed as an argument)
./server 9000

# Start subscriber (assumes subscriber connects to host:port)
./subscriber 127.0.0.1 9000

# From the Python client folder, send a sample payload
python3 pcom_hw2_udp_client/udp_client.py pcom_hw2_udp_client/sample_payloads.json
```

Note: The exact flags/arguments above are representative. If you'd like, I can parse the C++ source and Makefile to replace these placeholders with exact usage strings.

Using the Python UDP client

Open `pcom_hw2_udp_client/README.md` for client-specific instructions. The client includes several sample JSON payload files you can use to exercise different topics and wildcard/topic matching.

Running tests

There is a `test.py` in the repo root. Its behavior depends on the environment and the built binaries. Run it with Python 3:

```sh
python3 test.py
```

If tests or the harness expect the server/subscriber to be running, start those first as described above.

Project structure (short)

```
MessageStream/
├─ server.cpp
├─ subscriber.cpp
├─ utils.cpp
├─ utils.h
├─ headers.h
├─ po_tcp.h
├─ po_udp.h
├─ Makefile
├─ test.py
└─ pcom_hw2_udp_client/
   ├─ udp_client.py
   ├─ my_payloads.json
   ├─ sample_payloads.json
   └─ README.md
```
