#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <chrono>
#include "include/financeMetrics.hpp"
#include "include/apiCallers.hpp"
#include <omp.h>

void printFields();
void printValues(double& price, double& option, double& rate, double& btcRate,
                 double& strike, double& tau, double& implicit,
                 double& timeTaken,
                 double& delta, double& vega, double& theta, double& rho);

int main(void)
{
    CURLcode res;
    res = curl_global_init(CURL_GLOBAL_ALL);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n",
                curl_easy_strerror(res));
        return 1;
    }
    initscr();

    double option, price, strike, implicit, rate, btcRate, tau;
    double delta, callVega, theta, rho;
    int expiryDate;

    coinutExpiryTime expiryTimeSetter ((char*) "VANILLA_OPTION");
    expiryDate = expiryTimeSetter.getExpiryTime();

    bitfinexSpot spotPricer((char*) "ask");

    strike = spotPricer.getSpot();
    strike -= (int) strike % 5;
    strike = floor(strike);

    coinutOrderbook optionPricer ((char*)"VANILLA_OPTION",
                                  (char*) "CALL",
                                  expiryDate,
                                  strike,
                                  (std::string) "mid");

    bitfinexLendbook usdRateReceiver ((std::string) "USD",
                                      (std::string) "mid");
    bitfinexLendbook btcRateReceiver ((std::string) "BTC",
                                      (std::string) "mid");

    printFields();

    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
    double duration;

    while(true) {

        startTime = std::chrono::high_resolution_clock::now();

        #pragma omp parallel
        {
            #pragma omp single
            {
                #pragma omp task
                {
                    btcRate = btcRateReceiver.getRate();
                }
                #pragma omp task
                {
                    rate = usdRateReceiver.getRate();
                }
                #pragma omp task
                {
                    option = optionPricer.getOptionPrice();
                }
                #pragma omp task
                {
                    price = spotPricer.getSpot();
                }
                #pragma omp task
                {
                    tau = ((double) expiryDate - std::time(0))/31536000.0;
                }
            }
        }
        implicit = getImplicitVolatility (price, strike, rate,
                                          tau, btcRate, option);

        callVega =  vega (implicit, price, strike, rate,  tau, btcRate);

        delta = callDelta (price, strike, rate, btcRate, implicit, tau);

        theta = callTheta (price, strike, rate, btcRate, implicit, tau);

        rho = callRho (price, strike, rate, btcRate, implicit, tau);

        endTime = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>
            (endTime - startTime).count();
        duration /= 1000000000;

        printValues(price, option, rate, btcRate,strike, tau, implicit,
                    duration, delta, callVega, theta, rho);

    }
    curl_global_cleanup();
    endwin();
    return 0;
}

void printFields() {

    mvprintw(0, 0, "%-15s\n%-15s\n%-15s\n%-15s\n%-15s\n%-15s\n%-15s\n"
             "%-15s\n%-15s\n%-15s\n%-15s\n%-15s\n",
             "Option Price:",
             "Spot Price:",
             "USD Rate:",
             "BTC Rate:",
             "Strike:",
             "Maturity (y):",
             "Implied vol.:",
             "Delta:", "Vega:", "Theta:", "Rho:",
             "Time Taken:"
             );

    refresh();
}

void printValues (double& price, double& option, double& rate, double& btcRate,
                  double& strike, double& tau, double& implicit,
                  double& timeTaken,
                  double& delta, double& vega, double& theta, double& rho)
{
    mvprintw(0, 16, "%-13.9f", (price*option));
    mvprintw(1, 16, "%-13.8f", price);
    mvprintw(2, 16, "%-13.11f", rate);
    mvprintw(3, 16, "%-13.11f", btcRate);
    mvprintw(4, 16, "%-13.8f", strike);
    mvprintw(5, 16, "%-13.11f", tau);
    mvprintw(6, 16, "%-13.10f", implicit);
    mvprintw(7, 16, "%-13.10f", delta);
    mvprintw(8, 16, "%-13.10f", vega);
    mvprintw(9, 16, "%-13.10f",theta);
    mvprintw(10, 16, "%-13.10f", rho);
    mvprintw(11, 16, "%-13.10f", timeTaken);

    refresh();
}
