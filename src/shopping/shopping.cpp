#include "shopping.h"
#include "../database/db_handler.h"

using namespace TakeAwayPlatform;

Shopping::Shopping(long userId, int dishId, int quantity)
    : userId(userId), dishId(dishId), quantity(quantity) {}

Json::Value Shopping::addItem() {
    Json::Value response;
    try {
        DBConfig config = {"127.0.0.1", 33060, "root", "1234", "TakeAwayDatabase"};
        DatabaseHandler db(config);
        std::string sql = "INSERT INTO shopping_cart (user_id, dish_id, quantity, created_at, updated_at) VALUES (" +
            std::to_string(userId) + ", " + std::to_string(dishId) + ", " + std::to_string(quantity) + ", NOW(), NOW()) " +
            "ON DUPLICATE KEY UPDATE quantity = quantity + VALUES(quantity), updated_at = NOW()";
        db.query(sql);
        response["status"] = "success";
        response["user_id"] = static_cast<Json::Int64>(userId);
        response["dish_id"] = dishId;
        response["quantity"] = quantity;
    } catch (std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
    }
    return response;
}

Json::Value Shopping::getCart(long userId) {
    Json::Value response;
    try {
        DBConfig config = {"127.0.0.1", 33060, "root", "1234", "TakeAwayDatabase"};
        DatabaseHandler db(config);
        std::string sql = "SELECT * FROM shopping_cart WHERE user_id = " + std::to_string(userId);
        Json::Value result = db.query(sql);
        response["user_id"] = static_cast<Json::Int64>(userId);
        response["items"] = result;
        response["status"] = "success";
    } catch (std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
    }
    return response;
}