/*
 * Copyright (C), 2025-2030, 华中师范大学计算机学院
 * FileName: user.h
 * Author: sz
 * Date: 2025-6-27
 * Description: 外卖平台用户端功能模块
 * History:
 * <author>   <time>      <version>    <desc>
 * sz        2025-6-27   1.0          初始文件
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <ctime>
#include <json/json.h>

#include "common.h"
#include "db_handler.h"

namespace TakeAwayPlatform
{
    // 用户角色枚举（对应数据库中的role字段）
    enum class UserRole {
        CUSTOMER = 0,    // 顾客
        MERCHANT = 1,    // 商家
        ADMIN = 2        // 管理员
    };

    // 钱包状态枚举
    enum class WalletStatus {
        ACTIVE = 0,      // 活跃
        FROZEN = 1,      // 冻结
        CLOSED = 2       // 关闭
    };

    // 用户基本信息（对应applicant表）
    struct UserInfo {
        int64_t user_id;
        std::string user_name;      // 对应user_name字段
        std::string email;
        std::string phone;
        UserRole role;
        std::string avatar_url;
        std::time_t create_at;

        Json::Value toJson() const;
        static UserInfo fromJson(const Json::Value& json);
    };

    // 钱包信息（对应wallet表）
    struct WalletInfo {
        int wallet_id;
        int64_t user_id;
        double balance;
        WalletStatus status;
        std::time_t created_at;

        Json::Value toJson() const;
        static WalletInfo fromJson(const Json::Value& json);
    };

    // 充值记录（对应recharge_record表）
    struct RechargeRecord {
        int recharge_id;
        int64_t user_id;
        double amount;
        int transaction_id;
        std::string status;  // pending, completed, failed
        std::time_t paid_at;
        std::time_t created_at;

        Json::Value toJson() const;
        static RechargeRecord fromJson(const Json::Value& json);
    };

    // 用户管理类
    class UserManager 
    {
    public:
        explicit UserManager(std::unique_ptr<DatabaseHandler> dbHandler);
        ~UserManager() = default;

        // 用户注册和登录
        Json::Value registerUser(const std::string& username, const std::string& password, 
                                const std::string& email, const std::string& phone);
        Json::Value loginUser(const std::string& username, const std::string& password);
        Json::Value logoutUser(int64_t user_id);

        // 用户信息管理
        Json::Value getUserInfo(int64_t user_id);
        Json::Value updateUserInfo(int64_t user_id, const Json::Value& updateData);
        Json::Value changePassword(int64_t user_id, const std::string& oldPassword, 
                                 const std::string& newPassword);

        // 钱包管理
        Json::Value getWalletInfo(int64_t user_id);
        Json::Value rechargeBalance(int64_t user_id, double amount);
        Json::Value getBalance(int64_t user_id);
        Json::Value getRechargeHistory(int64_t user_id, int page = 1, int pageSize = 10);

        // 订单相关
        Json::Value getOrderHistory(int64_t user_id, int page = 1, int pageSize = 10);

    private:
        // 辅助方法
        std::string hashPassword(const std::string& password) const;
        bool verifyPassword(const std::string& password, const std::string& hashedPassword) const;
        bool isValidEmail(const std::string& email) const;
        bool isValidPhone(const std::string& phone) const;
        bool userExists(const std::string& username) const;
        bool emailExists(const std::string& email) const;
        bool phoneExists(const std::string& phone) const;
        
        Json::Value createResponse(bool success, const std::string& message, 
                                 const Json::Value& data = Json::Value::null) const;
        
        void recordLoginAction(int64_t user_id, const std::string& action, const std::string& status) const;
        bool createWalletForUser(int64_t user_id) const;

    private:
        std::unique_ptr<DatabaseHandler> m_dbHandler;
    };

    // 用户会话管理类
    class UserSession 
    {
    public:
        UserSession();
        ~UserSession() = default;

        // 会话管理
        std::string createSession(int64_t user_id, const std::string& username);
        bool validateSession(const std::string& sessionToken, int64_t& user_id);
        void destroySession(const std::string& sessionToken);
        void cleanExpiredSessions();

        // 获取当前在线用户数
        int getOnlineUserCount() const;

    private:
        struct SessionInfo {
            int64_t user_id;
            std::string username;
            std::time_t created_at;
            std::time_t last_access;
        };

        std::unordered_map<std::string, SessionInfo> m_sessions;
        mutable std::mutex m_sessionMutex;  // 添加 mutable
        static const int SESSION_TIMEOUT = 3600; // 1小时超时

        std::string generateSessionToken() const;
    };

    // 全局会话管理器实例
    extern UserSession g_userSession;
}
