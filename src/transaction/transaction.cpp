#include "transaction.h"
#include "../database/db_handler.h"

using namespace TakeAwayPlatform;

Transaction::Transaction(int transactionId, int walletId, int orderId, double amount, const std::string& type, const std::string& status)
    : transactionId(transactionId), walletId(walletId), orderId(orderId), amount(amount), type(type), status(status) {}

Json::Value Transaction::createTransaction() {
    Json::Value response;
    try {
        DBConfig config = {"127.0.0.1", 33060, "root", "1234", "TakeAwayDatabase"};
        DatabaseHandler db(config);
        std::string sql = "INSERT INTO transactions (transaction_id, wallet_id, order_id, amount, type, status, created_at) VALUES (" +
            std::to_string(transactionId) + ", " + std::to_string(walletId) + ", " + std::to_string(orderId) + ", " +
            std::to_string(amount) + ", '" + type + "', '" + status + "', NOW())";
        db.query(sql);
        response["status"] = "success";
        response["transaction_id"] = transactionId;
    } catch (std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
    }
    return response;
}

Json::Value Transaction::getTransaction(int transactionId) {
    Json::Value response;
    try {
        DBConfig config = {"127.0.0.1", 33060, "root", "1234", "TakeAwayDatabase"};
        DatabaseHandler db(config);
        std::string sql = "SELECT * FROM transactions WHERE transaction_id = " + std::to_string(transactionId);
        Json::Value result = db.query(sql);
        if (!result.empty()) {
            response = result[0];
            response["status"] = "success";
        } else {
            response["status"] = "not_found";
        }
    } catch (std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
    }
    return response;
}