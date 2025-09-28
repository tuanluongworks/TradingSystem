#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace trading {

/**
 * Thread-safe message queue template for inter-layer communication
 * Features:
 * - RAII locking
 * - Blocking and non-blocking operations
 * - Timeout support
 * - Size limits with backpressure handling
 */
template<typename T>
class MessageQueue {
public:
    explicit MessageQueue(size_t max_size = 1000);
    ~MessageQueue() = default;

    // Non-copyable, movable
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
    MessageQueue(MessageQueue&&) = default;
    MessageQueue& operator=(MessageQueue&&) = default;

    // Push operations
    bool push(const T& item);
    bool push(T&& item);
    bool try_push(const T& item);
    bool try_push(T&& item);
    template<typename Rep, typename Period>
    bool try_push_for(const T& item, const std::chrono::duration<Rep, Period>& timeout);

    // Pop operations
    bool pop(T& item);
    bool try_pop(T& item);
    template<typename Rep, typename Period>
    bool try_pop_for(T& item, const std::chrono::duration<Rep, Period>& timeout);

    // Query operations
    size_t size() const;
    bool empty() const;
    bool full() const;
    size_t capacity() const { return max_size_; }

    // Control operations
    void clear();
    void shutdown();

private:
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::queue<T> queue_;
    const size_t max_size_;
    bool shutdown_requested_;

    // Helper methods
    bool is_full_unlocked() const;
    bool is_empty_unlocked() const;
};

// Implementation

template<typename T>
MessageQueue<T>::MessageQueue(size_t max_size)
    : max_size_(max_size), shutdown_requested_(false) {
    if (max_size == 0) {
        throw std::invalid_argument("Queue size must be positive");
    }
}

template<typename T>
bool MessageQueue<T>::push(const T& item) {
    std::unique_lock<std::mutex> lock(mutex_);

    not_full_.wait(lock, [this] {
        return !is_full_unlocked() || shutdown_requested_;
    });

    if (shutdown_requested_) {
        return false;
    }

    queue_.push(item);
    not_empty_.notify_one();
    return true;
}

template<typename T>
bool MessageQueue<T>::push(T&& item) {
    std::unique_lock<std::mutex> lock(mutex_);

    not_full_.wait(lock, [this] {
        return !is_full_unlocked() || shutdown_requested_;
    });

    if (shutdown_requested_) {
        return false;
    }

    queue_.push(std::move(item));
    not_empty_.notify_one();
    return true;
}

template<typename T>
bool MessageQueue<T>::try_push(const T& item) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_full_unlocked() || shutdown_requested_) {
        return false;
    }

    queue_.push(item);
    not_empty_.notify_one();
    return true;
}

template<typename T>
bool MessageQueue<T>::try_push(T&& item) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_full_unlocked() || shutdown_requested_) {
        return false;
    }

    queue_.push(std::move(item));
    not_empty_.notify_one();
    return true;
}

template<typename T>
template<typename Rep, typename Period>
bool MessageQueue<T>::try_push_for(const T& item, const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    bool result = not_full_.wait_for(lock, timeout, [this] {
        return !is_full_unlocked() || shutdown_requested_;
    });

    if (!result || shutdown_requested_) {
        return false;
    }

    queue_.push(item);
    not_empty_.notify_one();
    return true;
}

template<typename T>
bool MessageQueue<T>::pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);

    not_empty_.wait(lock, [this] {
        return !is_empty_unlocked() || shutdown_requested_;
    });

    if (shutdown_requested_ && is_empty_unlocked()) {
        return false;
    }

    item = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return true;
}

template<typename T>
bool MessageQueue<T>::try_pop(T& item) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_empty_unlocked()) {
        return false;
    }

    item = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return true;
}

template<typename T>
template<typename Rep, typename Period>
bool MessageQueue<T>::try_pop_for(T& item, const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    bool result = not_empty_.wait_for(lock, timeout, [this] {
        return !is_empty_unlocked() || shutdown_requested_;
    });

    if (!result || (shutdown_requested_ && is_empty_unlocked())) {
        return false;
    }

    item = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return true;
}

template<typename T>
size_t MessageQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

template<typename T>
bool MessageQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_empty_unlocked();
}

template<typename T>
bool MessageQueue<T>::full() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_full_unlocked();
}

template<typename T>
void MessageQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<T> empty;
    queue_.swap(empty);
    not_full_.notify_all();
}

template<typename T>
void MessageQueue<T>::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_requested_ = true;
    not_empty_.notify_all();
    not_full_.notify_all();
}

template<typename T>
bool MessageQueue<T>::is_full_unlocked() const {
    return queue_.size() >= max_size_;
}

template<typename T>
bool MessageQueue<T>::is_empty_unlocked() const {
    return queue_.empty();
}

} // namespace trading