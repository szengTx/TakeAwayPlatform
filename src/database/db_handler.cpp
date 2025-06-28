#include <iostream>

#include "db_handler.h"


namespace TakeAwayPlatform
{
    DatabaseHandler::DatabaseHandler(const DBConfig& config)
    {
        connect(config);
    }

    Json::Value DatabaseHandler::query(const std::string& sql) 
    {
        try 
        {
            std::cout << "DatabaseHandler::query sql:" << sql << std::endl;
            std::cout.flush();

            mysqlx::SqlResult result = session->sql(sql).execute();
            return parse_result(result);
        } 
        catch (const mysqlx::Error& e) 
        {
            // 处理数据库错误
            std::cerr << "Database error: " << e.what() << std::endl;
            return Json::Value(Json::objectValue);
        }
    }

    bool DatabaseHandler::is_connected() const 
    {
        if (!session) {
            return false;
        }

        try {
            // 尝试执行一个简单的查询
            mysqlx::RowResult result = session->sql("SELECT 1").execute();
            mysqlx::Row row = result.fetchOne();
            return static_cast<bool>(row);
        } catch (const mysqlx::Error& e) {
            // 如果查询失败，认为连接无效
            return false;
        }
    }

    void DatabaseHandler::reconnect() 
    {
        if (session) 
        {
            session->close();
        }
        
        connect(dbConfig);
    }

    void DatabaseHandler::connect(const DBConfig& config)
    {
        dbConfig = config;
        
        try 
        {
            std::cout << "DatabaseHandler::connect host:" << config.host << std::endl;
            std::cout << "DatabaseHandler::connect port:" << config.port << std::endl;
            std::cout << "DatabaseHandler::connect user:" << config.user << std::endl;
            std::cout << "DatabaseHandler::connect password:" << config.password << std::endl;
            std::cout.flush();

            // 基于X Protocol，使用URI连接
            // 显示禁止ssl连接，因为MySQL 8.0默认不支持ssl连接
            std::string uri = "mysqlx://" + config.user + ":" + config.password + 
                            "@" + config.host + ":" + std::to_string(config.port) + 
                            "?ssl-mode=DISABLED";
            session = std::make_unique<mysqlx::Session>(uri);

            session->sql("USE " + config.database).execute();

            std::cout << "Database connection successful." << std::endl;
            std::cout.flush();
        } 
        catch (const mysqlx::Error& e) 
        {
            std::cerr << "Database connection failed: " << e.what() << std::endl;
            session.reset();
        }
    }

    Json::Value DatabaseHandler::parse_result(mysqlx::SqlResult& result) 
    {
        Json::Value json_result(Json::arrayValue);
        
        for(mysqlx::Row row : result.fetchAll()) 
        {
            Json::Value json_row;
            for(unsigned index = 0; index < row.colCount(); ++index) 
            {
                mysqlx::Value value = row[index];
                std::string column_name = result.getColumn(index).getColumnName();

                switch(value.getType()) 
                {
                    case mysqlx::Value::STRING:
                        json_row[column_name] = value.get<std::string>();
                        break;

                    case mysqlx::Value::UINT64:
                        json_row[column_name] = value.get<uint64_t>();
                        break;

                    case mysqlx::Value::INT64:
                        json_row[column_name] = value.get<int64_t>();
                        break;

                    case mysqlx::Value::FLOAT:
                        json_row[column_name] = value.get<float>();
                        break;

                    case mysqlx::Value::DOUBLE:
                        json_row[column_name] = value.get<double>();
                        break;

                    case mysqlx::Value::BOOL:
                        json_row[column_name] = value.get<bool>();
                        break;

                    default:
                        json_row[column_name] = "UNSUPPORTED_TYPE";
                }
            }

            json_result.append(json_row);
        }

        return json_result;
    }
}