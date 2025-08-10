#pragma once
#include "Types.h"
#include <variant>
#include <string>
#include <chrono>

// Domain events representing the lifecycle and flow in the trading pipeline.
struct NewOrderEvent { Order order; };
struct CancelOrderEvent { std::string orderId; std::string userId; };
struct ExecuteOrderEvent { std::string orderId; };
struct MarketDataUpdateEvent { MarketDataPoint data; };
struct TradeExecutionEvent { Order order; double executedPrice; double executedQuantity; };
struct ShutdownEvent { };

using TradingEvent = std::variant<NewOrderEvent, CancelOrderEvent, ExecuteOrderEvent, MarketDataUpdateEvent, TradeExecutionEvent, ShutdownEvent>;
