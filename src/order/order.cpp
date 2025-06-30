#include "order.h"
#include "../database/db_handler.h"

using namespace TakeAwayPlatform;

Order::Order(int orderId, const std::string& orderNumber, long userId, long merchantId, double totalAmount,
             const std::string& status, const std::string& deliveryAddress, const std::string& contactPhone, const std::string& items)
    : orderId(orderId), orderNumber(orderNumber), userId(userId), merchantId(merchantId), totalAmount(totalAmount),
      status(status), deliveryAddress(deliveryAddress), contactPhone(contactPhone), items(items) {}

Json::Value Order::createOrder() {
    Json::Value response;
    try {
        DBConfig config = {"127.0.0.1", 33060, "root", "1234", "TakeAwayDatabase"};
        DatabaseHandler db(config);
        std::string sql = "INSERT INTO orders (order_id, order_number, user_id, merchant_id, total_amount, status, delivery_address, contact_phone, items, created_at, updated_at) VALUES (" +
            std::to_string(orderId) + ", '" + orderNumber + "', " + std::to_string(userId) + ", " + std::to_string(merchantId) + ", " +
            std::to_string(totalAmount) + ", '" + status + "', '" + deliveryAddress + "', '" + contactPhone + "', '" + items + "', NOW(), NOW())";
        db.query(sql);
        response["status"] = "success";
        response["order_id"] = orderId;
    } catch (std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
    }
    return response;
}

Json::Value Order::getOrder(int orderId) {
    Json::Value response;
    try {
        DBConfig config = {"127.0.0.1", 33060, "root", "1234", "TakeAwayDatabase"};
        DatabaseHandler db(config);
        std::string sql = "SELECT * FROM orders WHERE order_id = " + std::to_string(orderId);
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