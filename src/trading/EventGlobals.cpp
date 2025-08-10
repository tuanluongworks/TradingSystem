#include "OrderEvents.h"
#include "../infrastructure/LockFreeQueue.h"

SPSCQueue<TradingEvent>* g_orderEventQueue = nullptr; // single definition
