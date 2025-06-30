#include <iostream>
#include <thread>

#include "rest_server.h"
#include "../user/user.h"
#include "../order/order.h"
#include "../shopping/shopping.h"
#include "../transaction/transaction.h"

namespace TakeAwayPlatform
{
    RestServer::RestServer(const std::string& configPath) : threadPool(std::thread::hardware_concurrency()) 
    {
        std::cout << "RestServer starting." << std::endl;
        std::cout.flush();

        Json::Value config = load_config(configPath);

        std::cout << "RestServer load config success." << std::endl;
        std::cout.flush();
        
        // 初始化数据库连接池
        init_db_pool(config["database"]);

        std::cout << "RestServer instance created." << std::endl;
        std::cout.flush();
    }

    RestServer::~RestServer() 
    {
        stop();
    }

    void RestServer::start(int port) 
    {
        if (isRunning) {
            std::cerr << "Server is already running." << std::endl;
            std::cout.flush();
            return;
        }
        
        isRunning = true;
        stopRequested = false;
        
        // 成员变量保存线程
        serverThread = std::thread([this, port] {
            this->run_server(port);
        });
        
        std::cout << "Server starting on port " << port << "..." << std::endl;
        std::cout.flush();
    }

    void RestServer::run_server(int port) 
    {
        try 
        {
            // 设置路由
            setup_routes();
            
            std::cout << "HTTP server listening on port " << port << std::endl;
            std::cout.flush();

            if (!server.listen("0.0.0.0", port)) {
                std::cerr << "Failed to start server on port " << port << std::endl;
            }
            
            std::cout << "HTTP server exited listen loop." << std::endl;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Server error in worker thread: " << e.what() << std::endl;
            std::cout.flush();
        }
        
        // 服务器已停止，更新状态
        isRunning = false;
        
        // 通知等待的线程
        stopCv.notify_one();
        std::cout << "Server worker thread exiting." << std::endl;
        std::cout.flush();
    }

    void RestServer::stop() 
    {
        if (!isRunning) {
            std::cout << "Server already stop." << std::endl;
            std::cout.flush();
            return;
        }
        
        std::cout << "Requesting server stop..." << std::endl;
        std::cout.flush();
        stopRequested = true;
        
        // 通知服务器停止
        server.stop();
        
        // 等待服务器实际停止
        std::unique_lock<std::mutex> lock(stopMtx);
        if (stopCv.wait_for(lock, std::chrono::seconds(5), [this] {
            return !isRunning;
        }))
        {
            if (serverThread.joinable()) 
            {
                serverThread.join();
            }

            std::cout << "Server stopped successfully." << std::endl;
            std::cout.flush();
        } 
        else 
        {
            std::cerr << "Warning: Server did not stop within timeout." << std::endl;
            std::cout.flush();

            if (serverThread.joinable()) 
            {
                serverThread.detach(); // 最后手段，避免死锁
            }
        }

        // 清理数据库连接池
        std::lock_guard<std::mutex> dbLock(dbPoolMutex);
        while (!dbPool.empty()) {
            dbPool.pop();
        }
    }

    bool RestServer::is_running() const 
    { 
        return isRunning;
    }

    void RestServer::init_db_pool(const Json::Value& config) 
    {
        std::cout << "host: " << config["host"].asString() << std::endl;
        std::cout << "port: " << config["port"].asInt() << std::endl;
        std::cout << "user: " << config["user"].asString() << std::endl;
        std::cout << "password: " << config["password"].asString() << std::endl;
        std::cout << "name: " << config["name"].asString() << std::endl;
        std::cout.flush();
        
        int pool_size = config.get("pool_size", 10).asInt();

        dbConfig.push_back({
            config["host"].asString(),
            config["port"].asInt(),
            config["user"].asString(),
            config["password"].asString(),
            config["name"].asString()
        });

        for (int index = 0; index < pool_size; ++index) {
            dbPool.push(create_db_handler());
        }
    }

    std::unique_ptr<DatabaseHandler> RestServer::create_db_handler() 
    {
        return std::make_unique<DatabaseHandler>(dbConfig[0]);
    }

    std::unique_ptr<DatabaseHandler> RestServer::acquire_db_handler() 
    {
        std::lock_guard<std::mutex> lock(dbPoolMutex);
        if (dbPool.empty()) {
            return create_db_handler();
        }
        
        auto handler = std::move(dbPool.front());
        dbPool.pop();
        return handler;
    }

    void RestServer::release_db_handler(std::unique_ptr<DatabaseHandler> handler) 
    {
        std::lock_guard<std::mutex> lock(dbPoolMutex);
        dbPool.push(std::move(handler));
    }

    void RestServer::setup_routes() 
    {
        // 设置路由
        server.Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("TakeAwayPlatform is running!", "text/plain");
        });
        
        server.Get("/health", [this](const httplib::Request&, httplib::Response& res) {
            if (this->is_running() && !this->stopRequested) {
                res.set_content("OK", "text/plain");
            } else {
                res.set_content("SHUTTING_DOWN", "text/plain");
                res.status = 503; // Service Unavailable
            }
        });

// ...existing code...


        // ==================== 用户相关接口 ====================
        
        // 用户注册
        server.Post("/api/user/register", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                Json::Value requestData = parse_json(req.body);
                
                std::string username = requestData.get("username", "").asString();
                std::string password = requestData.get("password", "").asString();
                std::string email = requestData.get("email", "").asString();
                std::string phone = requestData.get("phone", "").asString();
                
                auto db_handler = acquire_db_handler();
                UserManager userManager(std::move(db_handler));
                
                Json::Value result = userManager.registerUser(username, password, email, phone);
                res.set_content(result.toStyledString(), "application/json");
                
            } catch (const std::exception& e) {
                Json::Value error;
                error["success"] = false;
                error["message"] = "服务器错误: " + std::string(e.what());
                error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                res.set_content(error.toStyledString(), "application/json");
                res.status = 500;
            }
        });
        
        // 用户登录
        server.Post("/api/user/login", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                Json::Value requestData = parse_json(req.body);
                
                std::string username = requestData.get("username", "").asString();
                std::string password = requestData.get("password", "").asString();
                
                auto db_handler = acquire_db_handler();
                UserManager userManager(std::move(db_handler));
                
                Json::Value result = userManager.loginUser(username, password);
                res.set_content(result.toStyledString(), "application/json");
                
            } catch (const std::exception& e) {
                Json::Value error;
                error["success"] = false;
                error["message"] = "服务器错误: " + std::string(e.what());
                error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                res.set_content(error.toStyledString(), "application/json");
                res.status = 500;
            }
        });

        // 用户退出
        server.Post("/api/user/logout", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                std::string authHeader = req.get_header_value("Authorization");
                if (authHeader.empty()) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "未提供授权令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                int64_t user_id;
                if (!g_userSession.validateSession(authHeader, user_id)) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "无效的会话令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                g_userSession.destroySession(authHeader);

                auto db_handler = acquire_db_handler();
                UserManager userManager(std::move(db_handler));
                
                Json::Value result = userManager.logoutUser(user_id);
                res.set_content(result.toStyledString(), "application/json");
                
            } catch (const std::exception& e) {
                Json::Value error;
                error["success"] = false;
                error["message"] = "服务器错误: " + std::string(e.what());
                error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                res.set_content(error.toStyledString(), "application/json");
                res.status = 500;
            }
        });

        // 获取用户信息
        server.Get("/api/user/info", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                std::string authHeader = req.get_header_value("Authorization");
                if (authHeader.empty()) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "未提供授权令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                int64_t user_id;
                if (!g_userSession.validateSession(authHeader, user_id)) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "无效的会话令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                auto db_handler = acquire_db_handler();
                UserManager userManager(std::move(db_handler));
                
                Json::Value result = userManager.getUserInfo(user_id);
                res.set_content(result.toStyledString(), "application/json");
                
            } catch (const std::exception& e) {
                Json::Value error;
                error["success"] = false;
                error["message"] = "服务器错误: " + std::string(e.what());
                error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                res.set_content(error.toStyledString(), "application/json");
                res.status = 500;
            }
        });

        // 获取钱包信息
        server.Get("/api/user/wallet", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                std::string authHeader = req.get_header_value("Authorization");
                if (authHeader.empty()) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "未提供授权令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                int64_t user_id;
                if (!g_userSession.validateSession(authHeader, user_id)) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "无效的会话令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                auto db_handler = acquire_db_handler();
                UserManager userManager(std::move(db_handler));
                
                Json::Value result = userManager.getWalletInfo(user_id);
                res.set_content(result.toStyledString(), "application/json");
                
            } catch (const std::exception& e) {
                Json::Value error;
                error["success"] = false;
                error["message"] = "服务器错误: " + std::string(e.what());
                error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                res.set_content(error.toStyledString(), "application/json");
                res.status = 500;
            }
        });

        // 账户充值
        server.Post("/api/user/recharge", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                std::string authHeader = req.get_header_value("Authorization");
                if (authHeader.empty()) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "未提供授权令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                int64_t user_id;
                if (!g_userSession.validateSession(authHeader, user_id)) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "无效的会话令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                Json::Value requestData = parse_json(req.body);
                double amount = requestData.get("amount", 0.0).asDouble();

                auto db_handler = acquire_db_handler();
                UserManager userManager(std::move(db_handler));
                
                Json::Value result = userManager.rechargeBalance(user_id, amount);
                res.set_content(result.toStyledString(), "application/json");
                
            } catch (const std::exception& e) {
                Json::Value error;
                error["success"] = false;
                error["message"] = "服务器错误: " + std::string(e.what());
                error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                res.set_content(error.toStyledString(), "application/json");
                res.status = 500;
            }
        });

        // 获取订单历史
        server.Get("/api/user/orders", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                std::string authHeader = req.get_header_value("Authorization");
                if (authHeader.empty()) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "未提供授权令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                int64_t user_id;
                if (!g_userSession.validateSession(authHeader, user_id)) {
                    Json::Value error;
                    error["success"] = false;
                    error["message"] = "无效的会话令牌";
                    error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                    res.set_content(error.toStyledString(), "application/json");
                    res.status = 401;
                    return;
                }

                // 获取分页参数
                int page = 1;
                int page_size = 10;
                
                if (req.has_param("page")) {
                    page = std::stoi(req.get_param_value("page"));
                }
                if (req.has_param("page_size")) {
                    page_size = std::stoi(req.get_param_value("page_size"));
                }
                


                auto db_handler = acquire_db_handler();
                UserManager userManager(std::move(db_handler));
                
                Json::Value result = userManager.getOrderHistory(user_id, page, page_size);
                res.set_content(result.toStyledString(), "application/json");
                
            } catch (const std::exception& e) {
                Json::Value error;
                error["success"] = false;
                error["message"] = "服务器错误: " + std::string(e.what());
                error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
                res.set_content(error.toStyledString(), "application/json");
                res.status = 500;
            }
        });
        // ==================== 订单相关接口 ====================






// 创建订单
server.Post("/orders", [](const httplib::Request& req, httplib::Response& res) {
    Json::Value orderData;
    Json::Reader reader;
    if (!reader.parse(req.body, orderData)) {
        res.set_content("{\"status\":\"error\",\"message\":\"Invalid JSON\"}", "application/json");
        res.status = 400;
        return;
    }
    Order order(
        orderData["order_id"].asInt(),
        orderData["order_number"].asString(),
        orderData["user_id"].asInt64(),
        orderData["merchant_id"].asInt64(),
        orderData["total_amount"].asDouble(),
        orderData["status"].asString(),
        orderData["delivery_address"].asString(),
        orderData["contact_phone"].asString(),
        orderData["items"].asString()
    );
    Json::Value response = order.createOrder();
    res.set_content(response.toStyledString(), "application/json");
});

// 查询订单
server.Get(R"(/orders/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
    int orderId = std::stoi(req.matches[1]);
    Order order(orderId, "", 0, 0, 0, "", "", "", "");
    Json::Value response = order.getOrder(orderId);
    res.set_content(response.toStyledString(), "application/json");
});

// 添加商品到购物车
server.Post("/shopping", [](const httplib::Request& req, httplib::Response& res) {
    Json::Value shoppingData;
    Json::Reader reader;
    if (!reader.parse(req.body, shoppingData)) {
        res.set_content("{\"status\":\"error\",\"message\":\"Invalid JSON\"}", "application/json");
        res.status = 400;
        return;
    }
    Shopping shopping(
        shoppingData["user_id"].asInt64(),
        shoppingData["dish_id"].asInt(),
        shoppingData["quantity"].asInt()
    );
    Json::Value response = shopping.addItem();
    res.set_content(response.toStyledString(), "application/json");
});

// 查询购物车
server.Get(R"(/shopping/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
    long userId = std::stoll(req.matches[1]);
    Shopping shopping(userId, 0, 0);
    Json::Value response = shopping.getCart(userId);
    res.set_content(response.toStyledString(), "application/json");
});

// 创建交易
server.Post("/transactions", [](const httplib::Request& req, httplib::Response& res) {
    Json::Value transactionData;
    Json::Reader reader;
    if (!reader.parse(req.body, transactionData)) {
        res.set_content("{\"status\":\"error\",\"message\":\"Invalid JSON\"}", "application/json");
        res.status = 400;
        return;
    }
    Transaction transaction(
        transactionData["transaction_id"].asInt(),
        transactionData["wallet_id"].asInt(),
        transactionData["order_id"].asInt(),
        transactionData["amount"].asDouble(),
        transactionData["type"].asString(),
        transactionData["status"].asString()
    );
    Json::Value response = transaction.createTransaction();
    res.set_content(response.toStyledString(), "application/json");
});

// 查询交易
server.Get(R"(/transactions/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
    int transactionId = std::stoi(req.matches[1]);
    Transaction transaction(transactionId, 0, 0, 0, "", "");
    Json::Value response = transaction.getTransaction(transactionId);
    res.set_content(response.toStyledString(), "application/json");
});
// ...existing code...
        // ==================== 原有的示例接口 ====================
        
        // 示例路由：获取所有菜品（使用线程池处理）
        server.Get("/menu", [&](const httplib::Request&, httplib::Response& res) 
        {
            threadPool.enqueue([this, &res] {
                auto db_handler = acquire_db_handler();
                Json::Value menu = db_handler->query("SELECT * FROM menu_items");
                release_db_handler(std::move(db_handler));
                
                res.set_content(menu.toStyledString(), "application/json");
            });
        });

        // 示例路由：创建订单（使用线程池处理）
        server.Post("/order", [&](const httplib::Request& req, httplib::Response& res) 
        {
            threadPool.enqueue([this, req, &res] {
                Json::Value order = parse_json(req.body);
                auto db_handler = acquire_db_handler();
                
                // 验证订单数据...
                // 插入数据库...
                
                release_db_handler(std::move(db_handler));
                res.set_content("{\"status\":\"created\"}", "application/json");
            });
        });
    }

    Json::Value RestServer::parse_json(const std::string& jsonStr) 
    {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        std::istringstream json_stream(jsonStr);
        
        if (!Json::parseFromStream(builder, json_stream, &root, &errors)) 
        {
            throw std::runtime_error("JSON parse error: " + errors);
        }

        return root;
    }
}