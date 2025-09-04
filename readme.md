### Single-Exchange Triangular Arbitrage Bot

A C++20 project that demonstrates how to trade a three-leg
arbitrage cycle on the Binance spot exchange.  The bot keeps local order books
for **BTC/USDT**, **ETH/BTC** and **ETH/USDT**, scans them for pricing edges and
submits orders when an opportunity arises.


## Four hurdles between “edge” and **real** profit

| Hurdle                          | What it means for you                                                                                                                                                                     |
| ------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Fill probability**            | Depth-5 often shows only a few BTC/ETH. If you submit 100 % maker orders the edge may vanish before you’re posted. Taker IOC guarantees a fill but costs 0.04 %.                          |
| **Partial fills / re-quotes**   | Leg 1 may fill 70 %, Leg 2 only 10 %. Without a catcher-order you’re left with inventory risk. You need logic to cancel/hedge leftovers.                                                  |
| **Latency & position in queue** | You’re hitting a Tokyo endpoint from home → \~200–250 ms RTT. Pro traders colocated in the same data-centre hover at < 2 ms. Many edges disappear before your order arrives.              |
| **Fees & BNB balance**          | With default taker fee 0.1 % the edge must exceed \~0.3 % to be net-positive for three taker legs. Holding enough BNB gets you 0.075 %. Using maker on two legs + taker on one helps too. |

---

| Parameter                     | Conservative value                    |
| ----------------------------- | ------------------------------------- |
| Capital tied up               | 1 000 USDT                            |
| Average edge after fees       | 0.03 % (maker-maker-taker mix)        |
| Successful edges caught / day | 50                                    |
| Average notional / triangle   | 20 USDT (limited by depth / slippage) |
| **Daily P/L**                 | `20 × 0.0003 × 50 ≈ 0.30 USDT`        |
| **Monthly**                   | ≈ 9 USDT                              |


cmake --build . --config Release    # compile
./Release/triarb.exe                # run the executable

# run tests
ctest -C Release -V                 # run the tests


For detailed build instructions and architecture notes see [docs/README.md](docs/README.md).
