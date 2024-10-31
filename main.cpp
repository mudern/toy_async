#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include "./toy_async.cpp"

// 模拟一个耗时任务：睡眠1秒
void time_consuming_task(int id) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Time-consuming task " << id << " completed.\n";
}

// 模拟一个计算密集任务：计算大量平方根并求和
double compute_intensive_task(int id) {
    double result = 0.0;
    for (int i = 0; i < 1e6; ++i) {
        result += std::sqrt(i);
    }
//    std::cout << "Compute-intensive task " << id << " completed.\n";
    return result;
}

// channel测试
void channel_test(){
    auto [tx, rx] = Channel<int>::create();

    // Sender thread
    std::thread sender([tx]() mutable {
        for (int i = 0; i < 5; ++i) {
            tx.send(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::thread sender_2([tx]() mutable {
        for (int i = 5; i < 10; ++i) {
            tx.send(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // Receiver thread
    std::thread receiver([&rx]() {
        while (true) {
            auto value = rx.receive();
            if (!value) {
                std::cout << "Channel closed, no more messages." << std::endl;
                break;
            }
            std::cout << "Received: " << *value << std::endl;
        }
    });

    sender.join();
    sender_2.join();
    rx.close();
    receiver.join();
}

int main() {
    AsyncRuntime runtime(8,512);  // 创建一个线程池，8个线程

    std::vector<std::future<void>> time_futures;
    std::vector<std::future<double>> compute_futures;

    // 交替提交任务，总共512个（256个耗时 + 256个计算密集）
    for (int i = 0; i < 512; ++i) {
        if (i % 2 == 0) {
            // 偶数任务：耗时任务
            time_futures.push_back(runtime.add_task(time_consuming_task, i / 2));
        } else {
            // 奇数任务：计算密集任务
            compute_futures.push_back(runtime.add_task(compute_intensive_task, i / 2));
        }
    }

    // 等待所有耗时任务完成
    for (auto& future : time_futures) {
        future.get();
    }

    // 等待所有计算密集任务完成，并统计结果
    double total_result = 0.0;
    for (auto& future : compute_futures) {
        total_result += future.get();
    }

    std::cout << "All tasks completed. Total result of compute-intensive tasks: "
              << total_result << "\n";

    runtime.stop();  // 停止线程池

    channel_test();

    return 0;
}