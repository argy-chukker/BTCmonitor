#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <ctime>
#include "apiCallers.hpp"

double callTheoreticPrice (double sigma, double spot, double strike,
                           double rate, double tau, double foreignRate);

double vega(double sigma, double spot, double strike,
            double rate, double tau, double foreignRate);

double volInitGuess (double spot, double strike, double rate, double tau);

double getImplicitVolatility (double spot, double strike,
                              double rate, double tau, double foreignRate,
                              double callPrice, double epsilon=0.001);

void printFields();
void printValues(double& price, double& option, double& rate, double& btcRate,
                 double& strike, double& tau, double& implicit);

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

    while(true) {

        btcRate = btcRateReceiver.getRate();
        rate = usdRateReceiver.getRate();
        option = optionPricer.getOptionPrice();
        price = spotPricer.getSpot();
        tau = ((double) expiryDate - std::time(0))/31536000.0;

        implicit = getImplicitVolatility (price, strike, rate,
                                          tau, btcRate, option);

        printValues(price, option, rate, btcRate,strike, tau, implicit);

}
    curl_global_cleanup();
    endwin();
    return 0;
}

double callTheoreticPrice(double sigma, double spot, double strike,
                          double rate, double tau, double foreignRate)
{
    double d1 =
        log(spot/strike) + (rate - foreignRate + sigma*sigma / 2.0) * tau;
    d1 /= sigma*sqrt(tau);
    double d2 = d1 - sigma*sqrt(tau);
    double opt_price = spot * exp(-foreignRate*tau) * erfc(-d1/sqrt(2))/2.0;
    opt_price -= exp(-rate*tau)*strike*erfc(-d2/sqrt(2))/2.0;
    return opt_price;
}
double vega(double sigma, double spot, double strike,
            double rate, double tau, double foreignRate)
{
    double d1 =
        log(spot/strike) + (rate - foreignRate + sigma*sigma / 2.0) * tau;
    d1 /= sigma*sqrt(tau);
    double vega =
        spot*exp(-foreignRate*tau) *
        (exp(-d1*d1/2.0)/(2*sqrt(2*asin(1))))*sqrt(tau);
    return vega;
}

double volInitGuess(double spot, double strike, double rate, double tau) {
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

        while( std::abs(theoreticPrice - spot * callPrice)
               > epsilon)
            {
                implicit = implicit -  1.0*(theoreticPrice - spot * callPrice)/
                    vega(implicit, spot, strike,rate,tau, foreignRate);

                theoreticPrice = callTheoreticPrice(implicit, spot, strike,
                                                    rate, tau, foreignRate);
            }
}

void printFields() {

    mvprintw(0, 0, "%-15s\n%-15s\n%-15s\n%-15s\n%-15s\n%-15s\n%-15s\n",
             "Option Price:",
             "Spot Price:",
             "USD Rate:",
             "BTC Rate:",
             "Strike:",
             "Maturity (y):",
             "Implied vol.:"
             );

    refresh();
}

void printValues (double& price, double& option, double& rate, double& btcRate,
                  double& strike, double& tau, double& implicit) {

    mvprintw(0, 16, "%-13.9f", (price*option));
    mvprintw(1, 16, "%-13.8f", price);
    mvprintw(2, 16, "%-13.11f", rate);
    mvprintw(3, 16, "%-13.11f", btcRate);
    mvprintw(4, 16, "%-13.8f", strike);
    mvprintw(5, 16, "%-13.11f", tau);
    mvprintw(6, 16, "%-13.10f", implicit);

refresh();
}
