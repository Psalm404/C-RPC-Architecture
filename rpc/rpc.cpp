#include <iostream>
#include <chrono>
#include <windows.h>
int main() {
    using namespace std::chrono;

    // 获取当前时间点
    system_clock::time_point start = system_clock::now();

    // 进行一些耗时操作
    Sleep(2000);

    // 获取后续的时间点
    system_clock::time_point end = system_clock::now();

    // 计算时间间隔
    duration<double> duration = duration_cast<std::chrono::duration<double>>(end - start);

    // 输出时间间隔
    std::cout << "Elapsed time: " << duration.count() << " seconds\n";

    return 0;
}