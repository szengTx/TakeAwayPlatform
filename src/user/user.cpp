/*
 * Copyright (C), 2025-2030, 华中师范大学计算机学院
 * FileName: user.cpp
 * Author: sz
 * Date: 2025-6-27
 * Description: 外卖平台用户端功能实现
 * History:
 * <author>   <time>      <version>    <desc>
 * sz        2025-6-27   1.0          初始文件
 */

#include "user.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <random>
#include <functional>
#include <chrono>
#include <stdexcept>

namespace TakeAwayPlatform
{
    // 全局会话管理器实例
    UserSession g_userSession;

    // ==================== UserInfo 实现 ====================
    Json::Value UserInfo::toJson() const {
        Json::Value json;
        json["user_id"] = static_cast<int64_t>(user_id);
        json["user_name"] = user_name;
        json["email"] = email;
        json["phone"] = phone;
        json["role"] = static_cast<int>(role);
        json["avatar_url"] = avatar_url;
        json["create_at"] = static_cast<int64_t>(create_at);
        return json;
    }

    UserInfo UserInfo::fromJson(const Json::Value& json) {
        UserInfo user;
        user.user_id = json.get("user_id", 0).asInt64();
        user.user_name = json.get("user_name", "").asString();
        user.email = json.get("email", "").asString();
        user.phone = json.get("phone", "").asString();
        user.role = static_cast<UserRole>(json.get("role", 0).asInt());
        user.avatar_url = json.get("avatar_url", "").asString();
        user.create_at = static_cast<std::time_t>(json.get("create_at", 0).asInt64());
        return user;
    }

    // ==================== WalletInfo 实现 ====================
    Json::Value WalletInfo::toJson() const {
        Json::Value json;
        json["wallet_id"] = wallet_id;
        json["user_id"] = static_cast<int64_t>(user_id);
        json["balance"] = balance;
        json["status"] = static_cast<int>(status);
        json["created_at"] = static_cast<int64_t>(created_at);
        return json;
    }

    WalletInfo WalletInfo::fromJson(const Json::Value& json) {
        WalletInfo wallet;
        wallet.wallet_id = json.get("wallet_id", 0).asInt();
        wallet.user_id = json.get("user_id", 0).asInt64();
        wallet.balance = json.get("balance", 0.0).asDouble();
        wallet.status = static_cast<WalletStatus>(json.get("status", 0).asInt());
        wallet.created_at = static_cast<std::time_t>(json.get("created_at", 0).asInt64());
        return wallet;
    }

    // ==================== RechargeRecord 实现 ====================
    Json::Value RechargeRecord::toJson() const {
        Json::Value json;
        json["recharge_id"] = recharge_id;
        json["user_id"] = static_cast<int64_t>(user_id);
        json["amount"] = amount;
        json["transaction_id"] = transaction_id;
        json["status"] = status;
        json["paid_at"] = static_cast<int64_t>(paid_at);
        json["created_at"] = static_cast<int64_t>(created_at);
        return json;
    }

    RechargeRecord RechargeRecord::fromJson(const Json::Value& json) {
        RechargeRecord record;
        record.recharge_id = json.get("recharge_id", 0).asInt();
        record.user_id = json.get("user_id", 0).asInt64();
        record.amount = json.get("amount", 0.0).asDouble();
        record.transaction_id = json.get("transaction_id", 0).asInt();
        record.status = json.get("status", "").asString();
        record.paid_at = static_cast<std::time_t>(json.get("paid_at", 0).asInt64());
        record.created_at = static_cast<std::time_t>(json.get("created_at", 0).asInt64());
        return record;
    }

    // ==================== UserManager 实现 ====================
    UserManager::UserManager(std::unique_ptr<DatabaseHandler> dbHandler) 
        : m_dbHandler(std::move(dbHandler)) {
    }

    Json::Value UserManager::getWalletInfo(int64_t user_id) {
        try {
            std::string sql = "SELECT wallet_id, user_id, balance, status, created_at "
                             "FROM wallet WHERE user_id = " + std::to_string(user_id);
            
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                return createResponse(false, "钱包不存在");
            }

            const auto& walletRow = result[0];
            Json::Value walletData;
            walletData["wallet_id"] = static_cast<int>(std::stoi(walletRow["wallet_id"].asString()));
            walletData["user_id"] = static_cast<int64_t>(std::stoll(walletRow["user_id"].asString()));
            walletData["balance"] = std::stod(walletRow["balance"].asString());
            walletData["status"] = walletRow["status"].asString();
            walletData["created_at"] = walletRow["created_at"].asString();

            return createResponse(true, "获取钱包信息成功", walletData);

        } catch (const std::exception& e) {
            return createResponse(false, "获取钱包信息失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::registerUser(const std::string& username, const std::string& password, 
                                         const std::string& email, const std::string& phone) {
        try {
            // 输入验证
            if (username.empty() || password.empty() || email.empty() || phone.empty()) {
                return createResponse(false, "所有字段都是必填的");
            }

            if (!isValidEmail(email)) {
                return createResponse(false, "邮箱格式不正确");
            }

            if (!isValidPhone(phone)) {
                return createResponse(false, "手机号格式不正确");
            }

            // 检查用户名、邮箱、手机号是否已存在
            if (userExists(username)) {
                return createResponse(false, "用户名已存在");
            }

            if (emailExists(email)) {
                return createResponse(false, "邮箱已被注册");
            }

            if (phoneExists(phone)) {
                return createResponse(false, "手机号已被注册");
            }

            // 密码加密
            std::string hashedPassword = hashPassword(password);

            // 插入用户数据到applicant表
            std::string insertUserSql = 
                "INSERT INTO applicant (user_name, password, email, phone, role, create_at) "
                "VALUES ('" + username + "', '" + hashedPassword + "', '" + email + "', '"
                + phone + "', 'customer', NOW())";

            auto insertResult = m_dbHandler->query(insertUserSql);
            if (insertResult.empty()) {
                return createResponse(false, "注册失败：数据库错误");
            }

            // 获取插入的用户ID
            std::string getUserIdSql = "SELECT LAST_INSERT_ID() as user_id";
            auto result = m_dbHandler->query(getUserIdSql);
            
            if (result.empty()) {
                return createResponse(false, "注册失败：无法获取用户ID");
            }

            int64_t userId = std::stoll(result[0]["user_id"].asString());

            // 为新用户创建钱包
            if (!createWalletForUser(userId)) {
                return createResponse(false, "注册失败：钱包创建失败");
            }

            // 记录注册日志
            recordLoginAction(userId, "register", "success");

            // 构造返回数据
            Json::Value userData;
            userData["user_id"] = static_cast<int64_t>(userId);
            userData["user_name"] = username;
            userData["email"] = email;
            userData["phone"] = phone;
            userData["role"] = "customer";

            return createResponse(true, "注册成功", userData);

        } catch (const std::exception& e) {
            return createResponse(false, "注册失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::loginUser(const std::string& username, const std::string& password) {
        try {
            // 查询用户信息
            std::string sql = "SELECT user_id, user_name, password, email, phone, role, avatar_url "
                             "FROM applicant WHERE user_name = '" + username + "'";
            
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                recordLoginAction(0, "login", "failed");
                return createResponse(false, "用户名或密码错误");
            }

            const auto& userRow = result[0];
            std::string storedPassword = userRow["password"].asString();
            
            // 验证密码
            if (!verifyPassword(password, storedPassword)) {
                int64_t userId = std::stoll(userRow["user_id"].asString());
                recordLoginAction(userId, "login", "failed");
                return createResponse(false, "用户名或密码错误");
            }

            int64_t userId = std::stoll(userRow["user_id"].asString());
            
            // 创建会话
            std::string sessionToken = g_userSession.createSession(userId, username);
            
            // 记录登录日志
            recordLoginAction(userId, "login", "success");

            // 构造返回数据
            Json::Value userData;
            userData["user_id"] = static_cast<int64_t>(userId);
            userData["user_name"] = userRow["user_name"].asString();
            userData["email"] = userRow["email"].asString();
            userData["phone"] = userRow["phone"].asString();
            userData["role"] = userRow["role"].asString();
            userData["avatar_url"] = userRow["avatar_url"].asString();
            userData["session_token"] = sessionToken;

            return createResponse(true, "登录成功", userData);

        } catch (const std::exception& e) {
            return createResponse(false, "登录失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::getUserInfo(int64_t user_id) {
        try {
            std::string sql = "SELECT user_id, user_name, email, phone, role, avatar_url, create_at "
                             "FROM applicant WHERE user_id = " + std::to_string(user_id);
            
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                return createResponse(false, "用户不存在");
            }

            const auto& userRow = result[0];
            Json::Value userData;
            userData["user_id"] = static_cast<int64_t>(std::stoll(userRow["user_id"].asString()));
            userData["user_name"] = userRow["user_name"].asString();
            userData["email"] = userRow["email"].asString();
            userData["phone"] = userRow["phone"].asString();
            userData["role"] = userRow["role"].asString();
            userData["avatar_url"] = userRow["avatar_url"].asString();
            userData["create_at"] = userRow["create_at"].asString();

            return createResponse(true, "获取用户信息成功", userData);

        } catch (const std::exception& e) {
            return createResponse(false, "获取用户信息失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::getRechargeHistory(int64_t user_id, int page, int pageSize) {
        try {
            int offset = (page - 1) * pageSize;
            
            std::string sql = "SELECT recharge_id, user_id, amount, transaction_id, status, paid_at, created_at "
                             "FROM recharge_record WHERE user_id = " + std::to_string(user_id) + " "
                             "ORDER BY created_at DESC "
                             "LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
            
            auto result = m_dbHandler->query(sql);
            
            Json::Value records(Json::arrayValue);
            for (const auto& row : result) {
                Json::Value record;
                record["recharge_id"] = static_cast<int>(std::stoi(row["recharge_id"].asString()));
                record["user_id"] = static_cast<int64_t>(std::stoll(row["user_id"].asString()));
                record["amount"] = std::stod(row["amount"].asString());
                record["transaction_id"] = static_cast<int>(std::stoi(row["transaction_id"].asString()));
                record["status"] = row["status"].asString();
                record["paid_at"] = row["paid_at"].asString();
                record["created_at"] = row["created_at"].asString();
                records.append(record);
            }

            Json::Value resultData;
            resultData["records"] = records;
            resultData["page"] = page;
            resultData["page_size"] = pageSize;

            return createResponse(true, "获取充值历史成功", resultData);

        } catch (const std::exception& e) {
            return createResponse(false, "获取充值历史失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::getOrderHistory(int64_t user_id, int page, int pageSize) {
        try {
            int offset = (page - 1) * pageSize;
            
            std::string sql = "SELECT o.order_id, o.order_number, o.merchant_id, o.total_amount, "
                             "o.status, o.created_at, m.shop_name "
                             "FROM orders o "
                             "LEFT JOIN merchants m ON o.merchant_id = m.merchant_id "
                             "WHERE o.user_id = " + std::to_string(user_id) + " "
                             "ORDER BY o.created_at DESC "
                             "LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
            
            auto result = m_dbHandler->query(sql);
            
            Json::Value orders(Json::arrayValue);
            for (const auto& row : result) {
                Json::Value order;
                order["order_id"] = static_cast<int>(std::stoi(row["order_id"].asString()));
                order["order_number"] = row["order_number"].asString();
                order["merchant_id"] = static_cast<int64_t>(std::stoll(row["merchant_id"].asString()));
                order["shop_name"] = row["shop_name"].asString();
                order["total_amount"] = std::stod(row["total_amount"].asString());
                order["status"] = row["status"].asString();
                order["created_at"] = row["created_at"].asString();
                orders.append(order);
            }

            Json::Value resultData;
            resultData["orders"] = orders;
            resultData["page"] = page;
            resultData["page_size"] = pageSize;

            return createResponse(true, "获取订单历史成功", resultData);

        } catch (const std::exception& e) {
            return createResponse(false, "获取订单历史失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::logoutUser(int64_t user_id) {
        try {
            // 记录登出日志
            recordLoginAction(user_id, "logout", "success");
            
            return createResponse(true, "登出成功");
        } catch (const std::exception& e) {
            return createResponse(false, "登出失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::rechargeBalance(int64_t user_id, double amount) {
        try {
            if (amount <= 0) {
                return createResponse(false, "充值金额必须大于0");
            }

            // 更新钱包余额
            std::string updateSql = "UPDATE wallet SET balance = balance + " + std::to_string(amount) +
                                   " WHERE user_id = " + std::to_string(user_id);
            
            auto updateResult = m_dbHandler->query(updateSql);
            if (updateResult.empty()) {
                return createResponse(false, "充值失败：数据库更新错误");
            }

            // 记录充值记录（简化版，实际应该配合支付系统）
            std::string insertSql = "INSERT INTO recharge_record (user_id, amount, transaction_id, status, paid_at, created_at) "
                                   "VALUES (" + std::to_string(user_id) + ", " + std::to_string(amount) + 
                                   ", 0, 'completed', NOW(), NOW())";
            
            m_dbHandler->query(insertSql);

            // 获取更新后的余额
            Json::Value walletInfo = getWalletInfo(user_id);
            
            return createResponse(true, "充值成功", walletInfo["data"]);

        } catch (const std::exception& e) {
            return createResponse(false, "充值失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::getBalance(int64_t user_id) {
        try {
            std::string sql = "SELECT balance FROM wallet WHERE user_id = " + std::to_string(user_id);
            
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                return createResponse(false, "钱包不存在");
            }

            Json::Value balanceData;
            balanceData["balance"] = std::stod(result[0]["balance"].asString());

            return createResponse(true, "获取余额成功", balanceData);

        } catch (const std::exception& e) {
            return createResponse(false, "获取余额失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::updateUserInfo(int64_t user_id, const Json::Value& updateData) {
        try {
            std::vector<std::string> updates;
            
            if (updateData.isMember("user_name") && !updateData["user_name"].asString().empty()) {
                updates.push_back("user_name = '" + updateData["user_name"].asString() + "'");
            }
            
            if (updateData.isMember("email") && !updateData["email"].asString().empty()) {
                std::string email = updateData["email"].asString();
                if (!isValidEmail(email)) {
                    return createResponse(false, "邮箱格式不正确");
                }
                updates.push_back("email = '" + email + "'");
            }
            
            if (updateData.isMember("phone") && !updateData["phone"].asString().empty()) {
                std::string phone = updateData["phone"].asString();
                if (!isValidPhone(phone)) {
                    return createResponse(false, "手机号格式不正确");
                }
                updates.push_back("phone = '" + phone + "'");
            }
            
            if (updateData.isMember("avatar_url")) {
                updates.push_back("avatar_url = '" + updateData["avatar_url"].asString() + "'");
            }
            
            if (updates.empty()) {
                return createResponse(false, "没有需要更新的字段");
            }
            
            std::string sql = "UPDATE applicant SET ";
            for (size_t i = 0; i < updates.size(); ++i) {
                if (i > 0) sql += ", ";
                sql += updates[i];
            }
            sql += " WHERE user_id = " + std::to_string(user_id);
            
            auto updateResult = m_dbHandler->query(sql);
            if (updateResult.empty()) {
                return createResponse(false, "更新失败：数据库错误");
            }
            
            return getUserInfo(user_id);
            
        } catch (const std::exception& e) {
            return createResponse(false, "更新用户信息失败：" + std::string(e.what()));
        }
    }

    Json::Value UserManager::changePassword(int64_t user_id, const std::string& oldPassword, 
                                           const std::string& newPassword) {
        try {
            // 验证旧密码
            std::string sql = "SELECT password FROM applicant WHERE user_id = " + std::to_string(user_id);
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                return createResponse(false, "用户不存在");
            }
            
            std::string storedPassword = result[0]["password"].asString();
            if (!verifyPassword(oldPassword, storedPassword)) {
                return createResponse(false, "原密码不正确");
            }
            
            // 更新密码
            std::string hashedNewPassword = hashPassword(newPassword);
            std::string updateSql = "UPDATE applicant SET password = '" + hashedNewPassword + 
                                   "' WHERE user_id = " + std::to_string(user_id);
            
            auto updateResult = m_dbHandler->query(updateSql);
            if (updateResult.empty()) {
                return createResponse(false, "密码修改失败：数据库错误");
            }
            
            // 记录密码修改日志
            recordLoginAction(user_id, "password_change", "success");
            
            return createResponse(true, "密码修改成功");
            
        } catch (const std::exception& e) {
            return createResponse(false, "密码修改失败：" + std::string(e.what()));
        }
    }

    // ==================== 辅助方法实现 ====================
    std::string UserManager::hashPassword(const std::string& password) const {
        std::hash<std::string> hasher;
        size_t hash = hasher(password + "salt_key_2025");
        return std::to_string(hash);
    }

    bool UserManager::verifyPassword(const std::string& password, const std::string& hashedPassword) const {
        return hashPassword(password) == hashedPassword;
    }

    bool UserManager::isValidEmail(const std::string& email) const {
        std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        return std::regex_match(email, emailRegex);
    }

    bool UserManager::isValidPhone(const std::string& phone) const {
        std::regex phoneRegex(R"(^1[3-9]\d{9}$)");
        return std::regex_match(phone, phoneRegex);
    }

    bool UserManager::userExists(const std::string& username) const {
        try {
            std::string sql = "SELECT COUNT(*) as count FROM applicant WHERE user_name = '" + username + "'";
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                std::cerr << "DEBUG: userExists query returned empty result" << std::endl;
                return false;
            }
            
            std::string countStr = result[0]["count"].asString();
            std::cerr << "DEBUG: Count result: '" << countStr << "'" << std::endl;
            
            if (countStr.empty()) {
                std::cerr << "DEBUG: Count string is empty" << std::endl;
                return false;
            }
            
            int count = std::stoi(countStr);
            return count > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "ERROR in userExists: " << e.what() << std::endl;
            return false; // 出错时假设用户不存在，让注册继续
        }
    }

    bool UserManager::emailExists(const std::string& email) const {
        try {
            std::string sql = "SELECT COUNT(*) as count FROM applicant WHERE email = '" + email + "'";
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                std::cerr << "DEBUG: emailExists query returned empty result" << std::endl;
                return false;
            }
            
            std::string countStr = result[0]["count"].asString();
            std::cerr << "DEBUG: Email count result: '" << countStr << "'" << std::endl;
            
            if (countStr.empty()) {
                return false;
            }
            
            int count = std::stoi(countStr);
            return count > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "ERROR in emailExists: " << e.what() << std::endl;
            return false;
        }
    }

    bool UserManager::phoneExists(const std::string& phone) const {
        try {
            std::string sql = "SELECT COUNT(*) as count FROM applicant WHERE phone = '" + phone + "'";
            auto result = m_dbHandler->query(sql);
            
            if (result.empty()) {
                std::cerr << "DEBUG: phoneExists query returned empty result" << std::endl;
                return false;
            }
            
            std::string countStr = result[0]["count"].asString();
            std::cerr << "DEBUG: Phone count result: '" << countStr << "'" << std::endl;
            
            if (countStr.empty()) {
                return false;
            }
            
            int count = std::stoi(countStr);
            return count > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "ERROR in phoneExists: " << e.what() << std::endl;
            return false;
        }
    }

    Json::Value UserManager::createResponse(bool success, const std::string& message, 
                                           const Json::Value& data) const {
        Json::Value response;
        response["success"] = success;
        response["message"] = message;
        response["timestamp"] = static_cast<int64_t>(std::time(nullptr));
        
        if (!data.isNull()) {
            response["data"] = data;
        }
        
        return response;
    }

    void UserManager::recordLoginAction(int64_t user_id, const std::string& action, const std::string& status) const {
        try {
            std::string sql = "INSERT INTO log_records (user_id, action_type, status, created_at) "
                             "VALUES (" + std::to_string(user_id) + ", '" + action + "', '" + status + "', NOW())";
            m_dbHandler->query(sql);
        } catch (const std::exception& e) {
            std::cerr << "记录登录日志失败: " << e.what() << std::endl;
        }
    }

    bool UserManager::createWalletForUser(int64_t user_id) const {
        try {
            std::string sql = "INSERT INTO wallet (user_id, balance, status, created_at) "
                             "VALUES (" + std::to_string(user_id) + ", 0.00, 'active', NOW())";
            auto result = m_dbHandler->query(sql);
            return !result.empty();
        } catch (const std::exception& e) {
            std::cerr << "创建钱包失败: " << e.what() << std::endl;
            return false;
        }
    }

    // ==================== UserSession 实现 ====================
    UserSession::UserSession() {
        // 启动清理过期会话的定时器（简化实现）
    }

    std::string UserSession::createSession(int64_t user_id, const std::string& username) {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        
        std::string token = generateSessionToken();
        
        SessionInfo session;
        session.user_id = user_id;
        session.username = username;
        session.created_at = std::time(nullptr);
        session.last_access = std::time(nullptr);
        
        m_sessions[token] = session;
        
        return token;
    }

    bool UserSession::validateSession(const std::string& sessionToken, int64_t& user_id) {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        
        auto it = m_sessions.find(sessionToken);
        if (it == m_sessions.end()) {
            return false;
        }
        
        // 检查会话是否过期
        std::time_t now = std::time(nullptr);
        if (now - it->second.last_access > SESSION_TIMEOUT) {
            m_sessions.erase(it);
            return false;
        }
        
        // 更新最后访问时间
        it->second.last_access = now;
        user_id = it->second.user_id;
        
        return true;
    }

    void UserSession::destroySession(const std::string& sessionToken) {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        m_sessions.erase(sessionToken);
    }

    void UserSession::cleanExpiredSessions() {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        
        std::time_t now = std::time(nullptr);
        auto it = m_sessions.begin();
        
        while (it != m_sessions.end()) {
            if (now - it->second.last_access > SESSION_TIMEOUT) {
                it = m_sessions.erase(it);
            } else {
                ++it;
            }
        }
    }

    int UserSession::getOnlineUserCount() const {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        return static_cast<int>(m_sessions.size());
    }

    std::string UserSession::generateSessionToken() const {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) {
            ss << std::hex << dis(gen);
        }
        
        return ss.str();
    }

}
