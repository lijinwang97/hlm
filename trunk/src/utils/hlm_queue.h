#ifndef HLMQUEUE_H
#define HLMQUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class HlmQueue {
   public:
    void push(T value);
    T pop();
    bool empty() const;
    std::size_t size() const;

   private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cond_var_;
};

template <typename T>
void HlmQueue<T>::push(T value) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(value));
    cond_var_.notify_one();
}

template <typename T>
T HlmQueue<T>::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [this]() { return !queue_.empty(); });
    T value = std::move(queue_.front());
    queue_.pop();
    return value;
}

template <typename T>
bool HlmQueue<T>::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

template <typename T>
std::size_t HlmQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

#endif  // HLMQUEUE_H
