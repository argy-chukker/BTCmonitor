#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <chrono>
#include "apiCallers.hpp"

double callTheoreticPrice (double sigma, double spot, double strike,
                           double rate, double tau, double foreignRate);

double vega (double sigma, double spot, double strike,
             double rate, double tau, double foreignRate);

double callDelta (double spot, double strike, double rate, double foreignRate,
              double sigma, double tau);

double callTheta (double spot, double strike, double rate, double foreignRate,
                  double sigma, double tau);

double callRho (double spot, double strike, double rate, double foreignRate,
                double sigma, double tau);

double volInitGuess (double spot, double strike, double rate, double tau);

double getImplicitVolatility (double spot, double strike,
                              double rate, double tau, double foreignRate,
                              double callPrice, double epsilon=0.001);

double getD1 (double spot, double strike, double rate, double foreignRate,
              double sigma, double tau);
double getD2 (double spot, double strike, double rate, double foreignRate,
           double sigma, double tau);

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

        btcRate = btcRateReceiver.getRate();
        rate = usdRateReceiver.getRate();
        option = optionPricer.getOptionPrice();
        price = spotPricer.getSpot();
        tau = ((double) expiryDate - std::time(0))/31536000.0;

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

double callTheoreticPrice(double sigma, double spot, double strike,
                          double rate, double tau, double foreignRate)
{
    double d1 = getD1(spot, strike, rate, foreignRate, sigma, tau);
    double d2 = getD2(spot, strike, rate, foreignRate, sigma, tau);

    double theoreticPrice = spot * exp(-foreignRate * tau);
    theoreticPrice *= erfc(-d1 / sqrt(2)) / 2.0;
    theoreticPrice -= exp(-rate * tau) * strike * erfc(-d2 / sqrt(2.0)) / 2.0;

    return theoreticPrice;
}
double vega(double sigma, double spot, double strike,
            double rate, double tau, double foreignRate)
{
    double d1 = getD1(spot, strike, rate, foreignRate, sigma, tau);

    double vega = spot * exp(-foreignRate * tau);
    vega *= (exp(-d1 * d1 / 2.0)/(2 * sqrt(2 * asin(1))));
    vega *= sqrt(tau);

    return vega;
}

double volInitGuess(double spot, double strike, double rate, double tau)
{
    double guess = log(spot/strike) + rate*tau;
    guess = fabs(guess) * 2.0 / tau;

    return(sqrt(guess));
}

double getImplicitVolatility (double spot, double strike,
                              double rate, double tau,
                              double foreignRate, double callPrice,
                              double epsilon)
 {
        double implicit = volInitGuess(spot, strike, rate, tau);
        double theoreticPrice = callTheoreticPrice(implicit, spot,
                                                   strike, rate,
                                                   tau, foreignRate);

        while( fabs(theoreticPrice - spot * callPrice) > epsilon)
            {
                double step =  (theoreticPrice - spot * callPrice);
                step /= vega(implicit, spot, strike,rate,tau, foreignRate);

                implicit -= step;

                theoreticPrice = callTheoreticPrice(implicit, spot, strike,
                                                    rate, tau, foreignRate);
            }

        return (implicit);
}

double callDelta (double spot, double strike, double rate, double foreignRate,
                  double sigma, double tau)
{
    double d1 = getD1(spot, strike, rate, foreignRate, sigma, tau);

    double delta = exp(-foreignRate * tau);
    delta *= erfc(-d1 / sqrt(2.0)) / 2.0;

    return(delta);
}

double callRho (double spot, double strike, double rate, double foreignRate,
                  double sigma, double tau)
{
    double d2 = getD2(spot, strike, rate, foreignRate, sigma, tau);

    double rho = strike * tau * exp(-foreignRate * tau);
    rho *= erfc(-d2 / sqrt(2.0)) / 2.0;

    return(rho);
}

double callTheta (double spot, double strike, double rate, double foreignRate,
                double sigma, double tau)
{
    double d1 = getD1(spot, strike, rate, foreignRate, sigma, tau);

    double theta = callDelta(spot, strike, rate, foreignRate, sigma, tau);
    theta *= spot * foreignRate;
    theta +=
        foreignRate * callRho(spot, strike, rate, foreignRate, sigma, tau) /tau;
    theta -= exp(-foreignRate * tau) * spot * sigma *
        (exp(-d1 * d1 / 2.0)/(2 * sqrt(2 * asin(1)))) / (2.0 * sqrt(tau));

    return(theta);
}


double getD1 (double spot, double strike, double rate, double foreignRate,
           double sigma, double tau)
{
    double d1 = log(spot/strike);
    d1 += (rate - foreignRate + sigma*sigma / 2.0) * tau;
    d1 /= sigma*sqrt(tau);

    return(d1);
}

double getD2 (double spot, double strike, double rate, double foreignRate,
           double sigma, double tau)
{
    double d2 = getD1(spot, strike, rate, foreignRate, sigma, tau);
    d2 -= sigma*sqrt(tau);

    return(d2);
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
