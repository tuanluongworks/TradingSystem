#include "OrderManager.h"
#include "DatabaseManager.h"
#include "Constants.h"
#include <algorithm>
#include <stdexcept>
#include <random>
#include <iostream>

OrderManager::OrderManager(std::shared_ptr<DatabaseManager> db) : dbManager(db) {}

OrderManager::~OrderManager() = default;

std::string OrderManager::createOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(ordersMutex);
    
    if (!validateOrder(order)) {
        throw std::invalid_argument("Invalid order parameters");
    }
    
    Order newOrder = order;
    newOrder.id = generateOrderId();
    newOrder.status = OrderStatus::PENDING;
    newOrder.timestamp = std::chrono::system_clock::now();
    
    activeOrders.push_back(newOrder);
    
    if (dbManager && dbManager->isConnected()) {
        dbManager->saveOrder(newOrder);
    }
    
    std::cout << "Order created: " << newOrder.id << " for " << newOrder.quantity 
              << " shares of " << newOrder.symbol << "\n";
    
    return newOrder.id;
}

bool OrderManager::cancelOrder(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(ordersMutex);
    
    auto it = std::find_if(activeOrders.begin(), activeOrders.end(),
        [&orderId](Order& order) { 
            return order.id == orderId && order.status == OrderStatus::PENDING; 
        });
    
    if (it != activeOrders.end()) {
        it->status = OrderStatus::CANCELLED;
        
        if (dbManager && dbManager->isConnected()) {
            dbManager->updateOrderStatus(orderId, OrderStatus::CANCELLED);
        }
        
        std::cout << "Order cancelled: " << orderId << "\n";
        return true;
    }
    
    return false;
}

bool OrderManager::updateOrder(const std::string& orderId, const Order& updatedOrder) {
    std::lock_guard<std::mutex> lock(ordersMutex);
    
    auto it = std::find_if(activeOrders.begin(), activeOrders.end(),
        [&orderId](const Order& order) { return order.id == orderId; });
    
    if (it != activeOrders.end() && it->status == OrderStatus::PENDING) {
        Order newOrder = updatedOrder;
        newOrder.id = orderId;
        newOrder.timestamp = it->timestamp;
        
        if (validateOrder(newOrder)) {
            *it = newOrder;
            
            if (dbManager && dbManager->isConnected()) {
                dbManager->saveOrder(newOrder);
            }
            
            return true;
        }
    }
    
    return false;
}

std::vector<Order> OrderManager::getActiveOrders() const {
    std::lock_guard<std::mutex> lock(ordersMutex);
    return activeOrders;
}

std::vector<Order> OrderManager::getOrdersByUserId(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(ordersMutex);
    
    std::vector<Order> userOrders;
    std::copy_if(activeOrders.begin(), activeOrders.end(), std::back_inserter(userOrders),
        [&userId](const Order& order) { return order.userId == userId; });
    
    return userOrders;
}

Order OrderManager::getOrderById(const std::string& orderId) const {
    std::lock_guard<std::mutex> lock(ordersMutex);
    
    auto it = std::find_if(activeOrders.begin(), activeOrders.end(),
        [&orderId](const Order& order) { return order.id == orderId; });
    
    if (it != activeOrders.end()) {
        return *it;
    }
    
    throw std::runtime_error("Order not found: " + orderId);
}

bool OrderManager::executeOrder(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(ordersMutex);
    
    auto it = std::find_if(activeOrders.begin(), activeOrders.end(),
        [&orderId](Order& order) { 
            return order.id == orderId && order.status == OrderStatus::PENDING; 
        });
    
    if (it != activeOrders.end()) {
        it->status = OrderStatus::FILLED;
        
        if (dbManager && dbManager->isConnected()) {
            dbManager->updateOrderStatus(orderId, OrderStatus::FILLED);
        }
        
        std::cout << "Order executed: " << orderId << "\n";
        return true;
    }
    
    return false;
}

std::string OrderManager::generateOrderId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    
    return "ORD" + std::to_string(dis(gen));
}

bool OrderManager::validateOrder(const Order& order) const {
    return order.quantity >= Constants::MIN_ORDER_QUANTITY &&
           order.quantity <= Constants::MAX_ORDER_QUANTITY &&
           order.price >= Constants::MIN_ORDER_PRICE &&
           !order.symbol.empty() &&
           !order.userId.empty();
}