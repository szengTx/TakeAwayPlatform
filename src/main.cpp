#include <csignal>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "common.h"
#include "rest_server.h"


std::atomic<bool> running(true);
std::mutex mtx;
std::condition_variable cv;


void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    std::cout.flush();

    running = false;

    cv.notify_all();  // 唤醒可能阻塞的主线程
}

int main() {
    std::cout << "Entry main.." << std::endl;
    std::cout.flush();

    // 设置信号处理
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sigaction(SIGINT, &sa, nullptr);   // CTRL+C
    sigaction(SIGTERM, &sa, nullptr);  // kill 默认信号
    sigaction(SIGQUIT, &sa, nullptr);  // CTRL+\

    try 
    {
        TakeAwayPlatform::RestServer restSrv("/opt/TakeAwayPlatform/config/config.json");
        std::cout << "Starting server on port 9090..." << std::endl;
        std::cout.flush();
        
        // 启动服务器（分离线程）
        restSrv.start(9090);
        
        // 使用条件变量等待信号，避免忙等待
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]{
                return !running || !restSrv.is_running();
            });
        }
        
        // 检测服务器是否意外停止
        if (!restSrv.is_running()) {
            std::cerr << "Server thread has stopped unexpectedly!" << std::endl;
        }
        
        // 优雅关闭
        std::cout << "Shutting down server..." << std::endl;
        std::cout.flush();
        restSrv.stop();
        
        // 等待服务器完全停止（最多10秒）
        const auto timeout = std::chrono::seconds(10);
        const auto start = std::chrono::steady_clock::now();
        
        while (restSrv.is_running() && 
               (std::chrono::steady_clock::now() - start) < timeout) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (restSrv.is_running()) {
            std::cerr << "Warning: Server did not stop within timeout" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server shutdown complete." << std::endl;
    std::cout.flush();
    
    return 0;
}