### README

---

# 异步任务调度器与消息通道

本项目是一个用于任务调度和执行的异步运行时库，支持多线程环境下的任务队列管理，并通过通道提供线程间安全的消息传递。

## 功能特点

- **AsyncRuntime**：基于线程池的任务调度器，支持异步执行任务。
- **任务调度**：可将带有指定参数的任务加入队列，并返回 `std::future` 以获取任务执行结果。
- **安全关闭**：允许安全地停止所有线程并等待退出。
- **通道消息传递**：基于通道的消息传递机制，提供线程安全的 `Sender` 和 `Receiver`，用于任务间通信。
- **异常处理**：任务可抛出异常，通过 `std::future` 获取异常并处理。

## 开始使用

### 先决条件

- C++17 或更高版本

### 使用方法

1. **初始化 AsyncRuntime**：创建 `AsyncRuntime` 实例，可指定线程池大小和任务队列的最大容量。
2. **提交任务**：使用 `add_task` 方法将任务添加到运行时，每个任务都可以带有函数和参数，结果可通过 `std::future` 获取。
3. **创建通道**：使用 `Channel::create()` 创建 `Sender` 和 `Receiver`，用于跨线程通信。

### 示例

```cpp
#include "AsyncRuntime.h"
#include "Channel.h"

// 任务执行
AsyncRuntime runtime(4);  // 创建大小为4的线程池
auto future = runtime.add_task([] { return "Hello from AsyncRuntime!"; });
std::cout << future.get() << std::endl;  // 输出: Hello from AsyncRuntime!

// 通道通信
auto [sender, receiver] = Channel<int>::create();
std::thread producer([&sender] { sender.send(42); });
std::thread consumer([&receiver] {
    auto received = receiver.receive();
    if (received) {
        std::cout << "Received: " << *received << std::endl;  // 输出: Received: 42
    }
});

producer.join();
consumer.join();
```

### 注意事项

- `AsyncRuntime` 的任务队列最多只能容纳 `max_tasks_size` 个任务，超过此限制的任务将被拒绝。
- 关闭 `Channel` 后将停止消息发送，并通知所有等待的接收者通道已关闭。

### 许可证

本项目基于 MIT 许可证开源。