#ifndef SHOPPING_H
#define SHOPPING_H

#include <string>
#include <json/json.h>

class Shopping {
public:
    Shopping(long userId, int dishId, int quantity);
    Json::Value addItem();
    Json::Value getCart(long userId);

private:
    long userId;
    int dishId;
    int quantity;
};

#endif // SHOPPING_H