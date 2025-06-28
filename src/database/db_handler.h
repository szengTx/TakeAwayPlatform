#pragma once

#include "common.h"
#include <mysqlx/xdevapi.h>

namespace TakeAwayPlatform
{

    class DatabaseHandler 
    {
    public:
        DatabaseHandler(const DBConfig& config);

        Json::Value query(const std::string& sql);

        bool is_connected() const;

        void reconnect();

        
    private:
        void connect(const DBConfig& config);

        Json::Value parse_result(mysqlx::SqlResult& result);


    private:
        DBConfig dbConfig;
        std::unique_ptr<mysqlx::Session> session;
    };

}