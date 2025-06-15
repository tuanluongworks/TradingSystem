#ifndef MOVINGAVERAGESTRATEGY_H
#define MOVINGAVERAGESTRATEGY_H

#include "Strategy.h"
#include <vector>

class MovingAverageStrategy : public Strategy {
public:
    MovingAverageStrategy(int shortWindow, int longWindow);
    virtual ~MovingAverageStrategy();

    virtual double calculateSignal(const std::vector<double>& prices) override;

private:
    int shortWindow;
    int longWindow;

    double calculateMovingAverage(const std::vector<double>& prices, int windowSize);
};

#endif // MOVINGAVERAGESTRATEGY_H