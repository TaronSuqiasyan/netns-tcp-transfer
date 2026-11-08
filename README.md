Taron Suqiasyan                # TCP File Transfer over Linux Network Namespaces

A sender (client) and a receiver (server) written in C with the POSIX socket API.
They run in two isolated Linux network namespaces connected by a virtual
Ethernet (`veth`) pair, simulating two separate machines on one PC.

```
 ns_sender                                   ns_receiver
┌───────────────────┐                      ┌───────────────────┐
│ sender            │                      │ receiver          │
│ veth-s 10.0.0.1/24├──────── veth ────────┤veth-r 10.0.0.2/24 │
└───────────────────┘                      └───────────────────┘
```

## Project structure

```
.
├── sender/
│   ├── sender.c          # TCP client: reads a file and sends it
│   └── sample.txt        # example file to transfer
├── receiver/
│   └── receiver.c        # TCP server: receives data and saves it to a file
├── scripts/
│   ├── setup_netns.sh    # creates namespaces + veth pair + IPs
│   ├── teardown_netns.sh # removes namespaces
│   └── run_demo.sh       # full automatic demo
├── Makefile
└── README.md
```

## Concepts

- **`ip`**: Linux tool to configure interfaces, addresses, routes and namespaces.
- **`ip netns`**: creates isolated network stacks, each with its own interfaces, IPs and routes.
- **`veth`**: a virtual cable; whatever enters one end comes out of the other.

## Socket workflow

| Receiver (server)                         | Sender (client)                         |
|-------------------------------------------|-----------------------------------------|
| `socket()` create TCP/IPv4 socket         | `socket()` create TCP/IPv4 socket       |
| `bind()` attach to IP + port              | *(bind omitted: ephemeral port)*        |
| `listen()` wait for connections           | `connect()` connect to receiver IP:port |
| `accept()` get connection socket          | `fread()` + `send()` file bytes         |
| `recv()` write bytes to `receiver/` file  | `close()` signals end of file           |
| `close()`                                 |                                         |

End of file is signalled by the sender closing the connection: the receiver
stops when `recv()` returns `0`.

## Requirements

Linux, `gcc`, `make`, `iproute2` (`ip`), and root privileges (`sudo`) for namespaces.

## Build

```bash
make
```

## Quick start (automatic)

```bash
sudo ./scripts/run_demo.sh
```

This builds the programs, creates the namespaces, transfers
`sender/sample.txt` to `receiver/received_sample.txt`, and compares both files.

## Manual usage

```bash
make
sudo ./scripts/setup_netns.sh

# Terminal 1 - receiver
sudo ip netns exec ns_receiver ./receiver/receiver 10.0.0.2 5000 received_sample.txt

# Terminal 2 - sender
sudo ip netns exec ns_sender ./sender/sender 10.0.0.2 5000 sample.txt

# Verify
diff sender/sample.txt receiver/received_sample.txt && echo "Identical"

# Clean up
sudo ./scripts/teardown_netns.sh
```

Run all commands from the repository root. Arguments:

- `receiver <bind_ip> <port> <output_file>` (saved in `receiver/`)
- `sender <server_ip> <port> <file>` (read from `sender/`)

Any file works, including binary ones: copy it into `sender/` and pass its name.

## Cleanup

```bash
sudo ./scripts/teardown_netns.sh
make clean
```
