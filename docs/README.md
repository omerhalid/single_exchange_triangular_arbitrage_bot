# Detailed Project Documentation

This folder contains comprehensive documentation for **TriArbBot**, a single-exchange triangular arbitrage bot written in modern C++.

* [Overview](#overview)
* [Repository structure](#repository-structure)
* [Build instructions](#build-instructions)
* [Runtime configuration](#runtime-configuration)
* [Architecture](#architecture)
* [Testing](#testing)
* [Roadmap](#roadmap)

## Overview

TriArbBot connects to the Binance spot exchange via WebSocket and maintains
local order books for **BTC/USDT**, **ETH/BTC** and **ETH/USDT**.  By
continuously recomputing the implied cross rates it detects fleeting price
inefficiencies where cycling through the three pairs yields a net profit after
fees.  Once the edge exceeds a configurable threshold the bot issues the three
orders required to complete the triangle.

This repository is designed as a **reference implementation** rather than a
fully optimised high-frequency system.  The aim is to showcase the building
blocks of a triangular arbitrage bot—depth-feed intake, order book management,
edge computation and REST order submission—in a concise and readable code base.
It should serve both as a hobby project and a learning tool for developers
interested in exchange connectivity and low-latency C++ networking.

### Project goals

* Provide an approachable example of a triangular arbitrage workflow on a single
  exchange.
* Demonstrate modern C++20 techniques (CPM, coroutine-style asynchronous code)
  without excessive complexity.
* Keep the architecture simple enough to run on a lightweight VPS while still
  achieving sub‑100 ms end‑to‑end latency in favourable conditions.

## Repository structure

```
/ - project root
├── include/         # public header files
│   ├── common.hpp
│   ├── gateway.hpp
│   ├── orderbook.hpp
│   ├── triarb_bot.hpp
│   └── websocket_session.hpp
├── src/             # C++ source files
│   ├── gateway.cpp
│   ├── main.cpp
│   ├── orderbook.cpp
│   ├── triarb_bot.cpp
│   └── websocket_session.cpp
├── test/            # unit tests using Catch2
│   ├── arbitrage_test.cpp
│   └── orderbook_test.cpp
├── CMakeLists.txt   # build configuration
├── readme.md        # quick introduction
├── requirements.md  # high level developer roadmap
└── docs/            # this documentation
```

## Build instructions

TriArbBot uses **CMake** (>=3.22) and requires a C++20 compiler. Dependencies are resolved via **vcpkg**/CPM and include Boost, OpenSSL, and nlohmann\_json.

1. Install all prerequisites (compiler, CMake, vcpkg, OpenSSL development headers).
2. Clone this repository and initialise vcpkg if it is not already available.
3. Create a build directory and run CMake with the vcpkg toolchain:

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

The `triarb` executable will be placed under `build/Release/`.

### Running tests

Unit tests are built as the `tests` target. After configuring the project, execute:

```bash
cmake --build . --target tests --config Release
ctest -C Release -V
```

## Runtime configuration

Several environment variables must be defined before launching the bot:

| Variable | Description |
|----------|-------------|
| `BINANCE_API_KEY` | API key for Binance account |
| `BINANCE_API_SECRET` | API secret for request signing |
| `LIVE` | Set to `1` or `true` to enable live trading. Any other value runs in dry-run mode |
| `MAX_NOTIONAL` | Optional per-trade USDT exposure. Defaults to `15` |

Example on Linux:

```bash
export BINANCE_API_KEY=<your key>
export BINANCE_API_SECRET=<your secret>
export LIVE=0            # dry run
export MAX_NOTIONAL=20
./triarb
```

## Architecture

### Order book management

`OrderBook` (see `include/orderbook.hpp`) stores the best bid and ask for a trading pair. Updates are applied via `update(updateId, bid, ask)` which ignores out-of-sequence data. Access is thread safe using a mutex.

### WebSocket intake

`WebsocketSession` (in `src/websocket_session.cpp`) connects to Binance, performs the SSL and WebSocket handshake, and continuously reads depth snapshots. Each incoming JSON frame is forwarded to a callback supplied by `TriArbBot`.

### Core bot logic

`TriArbBot` maintains three `OrderBook` instances and handles frames from `WebsocketSession`. Once all books have at least one update it computes the current arbitrage edge. If the edge exceeds the threshold the bot submits three orders via `Gateway`:

1. Buy ETH with USDT
2. Sell ETH for BTC
3. Sell BTC for USDT

The helper `Gateway` class constructs signed REST requests. In dry-run mode it simply prints the intended trades.

Under the hood the algorithm performs the following steps every time a depth
update arrives:

1. Parse the JSON snapshot and update the appropriate `OrderBook`.
2. Recalculate the best bid/ask cross rates for the triangle (USDT → ETH → BTC
   → USDT).
3. Compute the theoretical profit after subtracting taker/maker fees.
4. If the profit (in basis points) is greater than `MIN_EDGE_BP`, trigger the
   execution of the three legs in sequence.

## Testing

The project includes Catch2 based tests under `test/`:

- `orderbook_test.cpp` verifies basic book operations and thread safety.
- `arbitrage_test.cpp` contains a trivial sanity check.

Run tests with `ctest` as shown above.

## Roadmap

The file `requirements.md` outlines future milestones such as lock-free queues, compile-time path generation, risk management and a replay harness. Contributions are welcome!


