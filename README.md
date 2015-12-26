# Bitcoin Screener

A small BTC screener that shows current spot (Bitfinex) exchange rate, USD and BTC rates, and option prices (in USD). From these, the implicit volatility in the contract is calculated.

A minimal build is tested as of this commit both for g++ and clang++. To replicate it, just clone and run:

```{g++, clang++} main.cpp apiCallers.cpp -std=c++14 -lcurl -lncurses -I /path/to/boost_1_60_0```

The dependencies under which it has been tested are cURL 7.43.0, Boost 1.60.0 and ncurses 5.9.20150516, running on Lubuntu 15.10

