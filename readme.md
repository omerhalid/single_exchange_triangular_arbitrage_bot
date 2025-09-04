### Single-Exchange Triangular Arbitrage Bot

A C++20 project that demonstrates how to trade a three-leg
arbitrage cycle on the Binance spot exchange.  The bot keeps local order books
for **BTC/USDT**, **ETH/BTC** and **ETH/USDT**, scans them for pricing edges and
submits orders when an opportunity arises.


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
