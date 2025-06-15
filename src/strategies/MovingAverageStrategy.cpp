#include "MovingAverageStrategy.h"
#include <vector>
#include <numeric>

MovingAverageStrategy::MovingAverageStrategy(int period) : period(period) {}

double MovingAverageStrategy::calculateSignal(const std::vector<double>& prices) {
    if (prices.size() < period) {
        throw std::invalid_argument("Not enough data to calculate moving average.");
    }

    double sum = std::accumulate(prices.end() - period, prices.end(), 0.0);
    return sum / period;
}