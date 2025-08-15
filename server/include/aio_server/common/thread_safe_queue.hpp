#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
public:
    // 向队列中添加一个任务
    void push(T value) {
        // 1. 获取锁。std::lock_guard会在析构时自动解锁，非常安全。
        std::lock_guard<std::mutex> lock(mutex_);
        // 2. 将任务添加到内部的std::queue中
        queue_.push(std::move(value));
        // 3. 通知一个正在等待的消费者线程，“有新任务了，快醒来！”
        cond_.notify_one();
    }

    // 从队列中取出一个任务
    // 如果队列为空，则会阻塞等待
    T wait_and_pop() {
        // 1. 创建一个unique_lock。与lock_guard不同，它允许我们手动解锁和重新加锁。
        std::unique_lock<std::mutex> lock(mutex_);
        // 2. ⭐最关键的部分⭐
        // wait()会检查lambda条件：如果队列为空(true)，它会原子地：
        //    a) 解锁互斥锁
        //    b) 让当前线程进入睡眠状态
        // 当被notify_one()唤醒时，它会：
        //    c) 重新加锁
        //    d) 再次检查lambda条件，防止“伪唤醒”。如果条件不满足，继续睡。
        cond_.wait(lock, [this] { return !queue_.empty(); });

        // 3. 一旦被唤醒且队列不为空，取出任务
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

private:
    std::queue<T> queue_;             // 底层的非线程安全队列
    mutable std::mutex mutex_;        // 用于保护队列访问的互斥锁
    std::condition_variable cond_;  // 用于消费者等待的条件变量
};