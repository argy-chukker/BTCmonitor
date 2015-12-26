# mojrai

A small BTC screener that shows current spot exchange rate, USD and BTC rates, and option prices (in USD). From these, the implicit volatility in the contract is calculated.

A minimal build is tested as of this commit both for g++ and clang++. To replicate it, just clone and

gcc/clang++ main.cpp apiCallers.cpp -std=c++14 -lcurl -I /path/to/boost_1_59_0