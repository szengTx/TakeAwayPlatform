#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <json/json.h>

class Order {
public:
    Order(int orderId, const std::string& orderNumber, long userId, long merchantId, double totalAmount,
          const std::string& status, const std::string& deliveryAddress, const std::string& contactPhone, const std::string& items);
    Json::Value createOrder();
    Json::Value getOrder(int orderId);

private:
    int orderId;
    std::string orderNumber;
    long userId;
    long merchantId;
    double totalAmount;
    std::string status;
    std::string deliveryAddress;
    std::string contactPhone;
    std::string items;
};

#endif // ORDER_H