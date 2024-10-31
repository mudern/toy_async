//
// Created by 穆琰鑫 on 2024/10/31.
//

#include <iostream>
#include <chrono>
#include <vector>
#include <future>
#include <thread>
#include <numeric>
#include <cmath>
#include "./toy_async.cpp"

// 新的计算密集型任务：计算 1 到 n 的平方和并进行大量数学计算
int compute_heavy_task(int n) {
    double result = 0.0;
    for (int i = 1; i <= n; ++i) {
        result += std::sin(i) * std::cos(i) * std::sqrt(i);
    }
    return static_cast<int>(result);
}

int main() {
    const size_t num_tasks = 1000000;
    const size_t thread_pool_size = 8;
    AsyncRuntime runtime(thread_pool_size,num_tasks);
    std::vector<std::future<int>> futures;

    // 记录开始时间
    auto start_time = std::chrono::high_resolution_clock::now();

    // 添加计算密集型任务到运行时
    for (size_t i = 0; i < num_tasks; ++i) {
        futures.emplace_back(runtime.add_task(compute_heavy_task, 1000));
    }

    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }

    // 记录结束时间
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Completed " << num_tasks << " tasks in " << duration.count() << " ms." << std::endl;

    runtime.stop();
    return 0;
}
