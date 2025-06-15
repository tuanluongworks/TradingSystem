#ifndef STRATEGY_H
#define STRATEGY_H

class Strategy {
public:
    virtual ~Strategy() {}

    virtual void applyStrategy() = 0;
};

#endif // STRATEGY_H