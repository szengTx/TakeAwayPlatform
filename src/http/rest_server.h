#pragma once

#include <httplib.h>
#include <mysqlx/xdevapi.h>

#include "common.h"
#include "thread_pool.h"
#include "db_handler.h"


namespace TakeAwayPlatform
{
    class RestServer 
    {
    public:
        RestServer(const std::string& configPath);
        ~RestServer();

        void start(int port);
        void stop();
        bool is_running() const;
        

    private:
        void init_db_pool(const Json::Value& config);

        void run_server(int port);

        std::unique_ptr<DatabaseHandler> create_db_handler();

        std::unique_ptr<DatabaseHandler> acquire_db_handler();

        void release_db_handler(std::unique_ptr<DatabaseHandler> handler);

        void setup_routes();

        Json::Value parse_json(const std::string& jsonStr);


    private:
        httplib::Server server;
        ThreadPool threadPool;
        std::vector<DBConfig> dbConfig;
        std::mutex dbPoolMutex;
        std::queue<std::unique_ptr<DatabaseHandler>> dbPool;

        std::atomic<bool> isRunning {false};
        std::atomic<bool> stopRequested {false};

        // 用于等待服务器停止的同步对象
        std::mutex stopMtx;
        std::condition_variable stopCv;

        std::thread serverThread;
    };

}