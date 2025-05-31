```markdown
# Triangular Arbitrage Bot – Developer Roadmap

> **Goal**  
> Build a **single-exchange triangular-arbitrage bot for Binance spot** that can detect and execute profitable cycles in **< 100 ms** end-to-end, written in modern **C++20** and deployable on a low-cost VPS.

---

## Repository layout
```

/bot-root
│
├── include/
│   ├── common.hpp          // shared aliases & helpers
│   ├── orderbook.hpp       // OrderBook, Quote structs
│   ├── arbitrage.hpp       // core edge-scanner API
│   └── gateway.hpp         // REST/FIX execution
├── src/
│   ├── main.cpp            // CLI & event loop
│   ├── websocket\_session.cpp
│   ├── orderbook.cpp
│   ├── arbitrage.cpp
│   └── gateway.cpp
├── test/
│   ├── replay\_driver.cpp   // depth-feed replay harness
│   └── arbitrage\_test.cpp  // unit tests (Catch2)
├── scripts/
│   └── record\_depth.py     // optional data recorder
├── CMakeLists.txt
└── README.md               // this file

````

---

## Milestone checklist

### 1  Bootstrapping
- [x] **TODO:** Initialise CMake ≥ 3.22, set  
      `CXX_STANDARD 20`, `-O3 -march=native -flto`.
- [x] **TODO:** Pull Catch2 using `CPM.cmake`.

### 2  Market-data intake
- [x] **TODO:** Implement `WebsocketSession` (Boost::Beast) connecting to  
      `wss://stream.binance.com:9443/stream?streams=<symbol>@depth5@100ms`.
- [ ] **TODO:** Push raw JSON strings into a lock-free  
      `moodycamel::ReaderWriterQueue<OrderBookUpdate>`.

### 3  Order-book cache
- [ ] **TODO:** Define `struct Quote { double px; double qty; };`
- [ ] **TODO:** Maintain per-symbol `OrderBook` with best bid/ask and `lastUpdateId`.

### 4  Arbitrage engine
- [ ] **TODO:** Generate compile-time  
      `constexpr std::array<TriPath, N>` representing (p1, p2, p3) symbol cycles.
- [ ] **TODO:** `double computeEdge(const OrderBook&, const OrderBook&, const OrderBook&)`  
      applying taker/maker fees.
- [ ] **TODO:** On order-book update, rescan affected paths;  
      trigger when `edge_bp >= MIN_EDGE_BP`.

### 5  Execution gateway
- [ ] **TODO:** Async POST `/api/v3/order` with HMAC-SHA256 signature.
- [ ] **TODO:** Support IOC, LIMIT-MAKER, FOK order types.
- [ ] **TODO:** Expose  
      ```cpp
      struct NewOrder {
          std::string symbol;
          Side   side;
          double qty;
          double px;
          OrderType type;
      };
      ```

### 6  Risk manager
- [ ] **TODO:** Track open positions/cash; abort if exposure > `MAX_NOTIONAL`  
      or realized PnL < `-MAX_DRAWDOWN`.

### 7  Config & CLI
- [ ] **TODO:** Parse `config.yml` via YAML-CPP for API keys, fee schedule, thresholds, VPS heartbeat URL.

### 8  Replay harness
- [ ] **TODO:** Build `replay_driver` replaying recorded depth snapshots at original timestamps for deterministic tests.

### 9  Paper vs live
- [ ] **TODO:** `--mode=paper|live` CLI switch; in paper mode, write simulated fills to CSV.

### 10  Deployment & monitoring
- [ ] **TODO:** Provide `systemd` unit; push metrics (edge, latency, fills) to InfluxDB → Grafana dashboard.

---
