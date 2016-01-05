#include <cmath>

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
