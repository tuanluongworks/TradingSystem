#ifndef DATAPROVIDER_H
#define DATAPROVIDER_H

class DataProvider {
public:
    virtual ~DataProvider() {}

    virtual void fetchMarketData() = 0;
    virtual void fetchHistoricalData() = 0;
};

#endif // DATAPROVIDER_H