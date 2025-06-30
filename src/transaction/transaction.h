#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <json/json.h>

class Transaction {
public:
    Transaction(int transactionId, int walletId, int orderId, double amount, const std::string& type, const std::string& status);
    Json::Value createTransaction();
    Json::Value getTransaction(int transactionId);

private:
    int transactionId;
    int walletId;
    int orderId;
    double amount;
    std::string type;
    std::string status;
};

#endif // TRANSACTION_H