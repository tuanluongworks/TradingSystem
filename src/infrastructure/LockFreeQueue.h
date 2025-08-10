#pragma once
#include <atomic>
#include <vector>
#include <stdexcept>
#include <optional>
#include <cstddef>

// Simple single-producer single-consumer lock-free ring buffer.
// For multi-producer use a heavier structure, but initial deterministic matching thread
// can consume exclusively while one ingress thread produces.
template <typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(std::size_t capacity) : capacity_(capacity), mask_(capacity - 1), buffer_(capacity) {
        if ((capacity_ & (capacity_ - 1)) != 0) {
            throw std::invalid_argument("capacity must be power of two");
        }
    }
    bool push(const T& item) {
        auto head = head_.load(std::memory_order_relaxed);
        auto next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false; // full
        }
        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }
    bool push(T&& item) {
        auto head = head_.load(std::memory_order_relaxed);
        auto next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[head] = std::move(item);
        head_.store(next, std::memory_order_release);
        return true;
    }
    std::optional<T> pop() {
        auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt; // empty
        }
        T item = std::move(buffer_[tail]);
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return item;
    }
    bool empty() const { return head_.load() == tail_.load(); }
private:
    const std::size_t capacity_;
    const std::size_t mask_;
    std::vector<T> buffer_;
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};
