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
    size_t size() const;

   private:
    mutable mutex mutex_;
    queue<T> queue_;
    condition_variable cond_var_;
};

template <typename T>
void HlmQueue<T>::push(T value) {
    lock_guard<mutex> lock(mutex_);
    queue_.push(move(value));
    cond_var_.notify_one();
}

template <typename T>
T HlmQueue<T>::pop() {
    unique_lock<mutex> lock(mutex_);
    cond_var_.wait(lock, [this]() { return !queue_.empty(); });
    T value = move(queue_.front());
    queue_.pop();
    return value;
}

template <typename T>
bool HlmQueue<T>::empty() const {
    lock_guard<mutex> lock(mutex_);
    return queue_.empty();
}

template <typename T>
size_t HlmQueue<T>::size() const {
    lock_guard<mutex> lock(mutex_);
    return queue_.size();
}

#endif  // HLMQUEUE_H
