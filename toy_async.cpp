#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <future>
#include <optional>
#include <memory>

// 泛型任务类型
using Task = std::function<void()>;

class AsyncRuntime {
        public:
        explicit AsyncRuntime(size_t thread_pool_size=8, size_t max_tasks_size=256)
        : stop_(false),max_tasks_size_(max_tasks_size) {
            // 初始化线程池
            for (size_t i = 0; i < thread_pool_size; ++i) {
                // 使用emplace_back在容器创建内部构建线程
                // 线程用于调用异步运行时的thread_worker函数
                threads_.emplace_back(&AsyncRuntime::thread_worker, this);
            }
        }

        ~AsyncRuntime() {
            stop();
            for (std::thread &thread : threads_) {
                // 已连接线程依次等待退出
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }

        // 添加任务到队列
        template <typename Func, typename... Args>
        auto add_task(Func&& func, Args&&... args)
        -> std::future<typename std::invoke_result_t<Func, Args...>> {

            using ReturnType = typename std::invoke_result_t<Func, Args...>;

            // 创建一个 promise，并获取与之关联的 future
            auto promise = std::make_shared<std::promise<ReturnType>>();
            std::future<ReturnType> future = promise->get_future();

            // 捕获 promise，并区分返回值是否为 void
            auto task = [promise, func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable {
                try {
                    if constexpr (std::is_void_v<ReturnType>) {
                        func(std::forward<Args>(args)...);  // 执行 void 类型任务
                        promise->set_value();               // 设置 void 类型的 promise
                    } else {
                        promise->set_value(func(std::forward<Args>(args)...));  // 设置返回值
                    }
                } catch (...) {
                    promise->set_exception(std::current_exception());  // 捕获异常并传递
                }
            };

            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (task_queue_.size() >= max_tasks_size_) {
                    std::cerr << "Task queue is full! Cannot add more tasks.\n";
                    throw std::runtime_error("Task queue is full!");
                }
                task_queue_.emplace(std::move(task));
            }
            cond_var_.notify_one();
            return future;
        }

        // 停止运行时
        void stop() {
            {
                // 获取锁并对异步运行时状态进行解锁
                std::unique_lock<std::mutex> lock(mutex_);
                stop_ = true;
            }
            cond_var_.notify_all();
        }

        private:
        // 工作线程函数
        void thread_worker() {
            while (true) {
                Task task;
                {
                    // 获取互斥锁
                    std::unique_lock<std::mutex> lock(mutex_);
                    // 将互斥锁交付信号量,进入信号量等待
                    cond_var_.wait(lock, [this] { return stop_ || !task_queue_.empty(); });
                    if (stop_ && task_queue_.empty()) {
                        return;
                    }
                    // 得到信号通知后,获取任务准备执行
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }
                task();  // 执行任务
            }
        }

        std::vector<std::thread> threads_;               // 线程池
        std::queue<Task> task_queue_;                    // 任务队列
        std::mutex mutex_;                               // 互斥锁
        std::condition_variable cond_var_;               // 条件变量
        std::atomic<bool> stop_;                         // 控制事件循环是否停止
        const size_t max_tasks_size_;                    // 最大任务数
};

template <typename T>
class Channel {
public:
    class Sender {
    public:
        explicit Sender(std::shared_ptr<Channel<T>> channel) : channel_(channel) {}

        void send(T value) {
            std::lock_guard<std::mutex> lock(channel_->mutex_);
            if (channel_->closed_) {
                throw std::runtime_error("Channel is closed");
            }
            channel_->queue_.push(std::move(value));
            channel_->cv_.notify_one();
        }

    private:
        std::shared_ptr<Channel<T>> channel_;
    };

    class Receiver {
    public:
        explicit Receiver(std::shared_ptr<Channel<T>> channel) : channel_(channel) {}

        std::optional<T> receive() {
            std::unique_lock<std::mutex> lock(channel_->mutex_);
            channel_->cv_.wait(lock, [this] { return channel_->closed_ || !channel_->queue_.empty(); });

            if (channel_->queue_.empty() && channel_->closed_) {
                return std::nullopt; // Channel closed and no more messages
            }

            T value = std::move(channel_->queue_.front());
            channel_->queue_.pop();
            return value;
        }

        void close() {
            channel_->close();
        }

    private:
        std::shared_ptr<Channel<T>> channel_;
    };

    static std::pair<Sender, Receiver> create() {
        auto channel = std::shared_ptr<Channel>(new Channel());
        return {Sender(channel), Receiver(channel)};
    }

private:
    Channel() = default;

    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
    }

    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;
};