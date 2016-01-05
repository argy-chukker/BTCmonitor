#include "include/financeMetrics.hpp"

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
