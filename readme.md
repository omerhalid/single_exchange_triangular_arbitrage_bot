### Single-Exchange Triangular Arbitrage Bot

A small-scale C++20 project that demonstrates how to trade a three-leg
arbitrage cycle on the Binance spot exchange.  The bot keeps local order books
for **BTC/USDT**, **ETH/BTC** and **ETH/USDT**, scans them for pricing edges and
submits orders when an opportunity arises.

---

## Why profits exist at all

| Micro-edge source                                             | Typical size                 | Lifespan  |
| ------------------------------------------------------------- | ---------------------------- | --------- |
| Independent order-book moves across three Binance pairs       | 0.03 – 0.15 % (before fees)  | 10–500 ms |
| Inaccurate maker/taker spread after a large market order      | 0.1 – 0.3 %                  | < 100 ms  |
| Momentary imbalance when one leg is illiquid (∴ high bid/ask) | 0.2 – 0.5 % (but fills thin) | < 1 s     |

Those fleeting mismatches are what your bot is detecting when it prints **“EDGE 0.09 % → FIRE”**.

---

## Four hurdles between “edge” and **real** profit

| Hurdle                          | What it means for you                                                                                                                                                                     |
| ------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Fill probability**            | Depth-5 often shows only a few BTC/ETH. If you submit 100 % maker orders the edge may vanish before you’re posted. Taker IOC guarantees a fill but costs 0.04 %.                          |
| **Partial fills / re-quotes**   | Leg 1 may fill 70 %, Leg 2 only 10 %. Without a catcher-order you’re left with inventory risk. You need logic to cancel/hedge leftovers.                                                  |
| **Latency & position in queue** | You’re hitting a Tokyo endpoint from home → \~200–250 ms RTT. Pro traders colocated in the same data-centre hover at < 2 ms. Many edges disappear before your order arrives.              |
| **Fees & BNB balance**          | With default taker fee 0.1 % the edge must exceed \~0.3 % to be net-positive for three taker legs. Holding enough BNB gets you 0.075 %. Using maker on two legs + taker on one helps too. |

---

## Realistic P/L sketch for a “bedroom-bot”

| Parameter                     | Conservative value                    |
| ----------------------------- | ------------------------------------- |
| Capital tied up               | 1 000 USDT                            |
| Average edge after fees       | 0.03 % (maker-maker-taker mix)        |
| Successful edges caught / day | 50                                    |
| Average notional / triangle   | 20 USDT (limited by depth / slippage) |
| **Daily P/L**                 | `20 × 0.0003 × 50 ≈ 0.30 USDT`        |
| **Monthly**                   | ≈ 9 USDT                              |

*“pays for coffee, not rent.”*

---

1. **Learn & build skills that recruiters love**
   Low-latency C++, Asio, WebSockets, REST signing, exchange micro-structure—great portfolio talking points.
2. **Treat profits as a latency scoreboard**
   Even \$0.05 profit on a 0.08 % edge means you beat 99 % of retail bots—an engineering win.
3. **Leverage maker rebates**
   Binance pays 0.01 % if your posted order adds liquidity. Two maker legs + one taker leg can turn a 0.05 % raw edge into 0.07 % net.
4. **Graduate to multi-exchange**
   Cross-venue arbitrage (e.g., Binance ⇆ Kraken) has larger edges (0.2 – 1 %) at the cost of extra transfer latency and counter-party risk.
5. **Co-locate or rent a VPS near Tokyo (AWS ap-northeast-1)**
   Drops round-trip to 4 – 8 ms and doubles the number of edges you actually capture.

---

cmake --build . --config Release    # compile
./Release/triarb.exe                # run the executable

# run tests
ctest -C Release -V                 # run the tests

# Full clean rebuild (if you change dependencies in cmakelist.txt)
cd ..
Remove-Item -Recurse -Force C:\Users\katka\source\single_exchange_triangular_arbitrage_bot\build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/Users/katka/source/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release


For detailed build instructions and architecture notes see [docs/README.md](docs/README.md).
