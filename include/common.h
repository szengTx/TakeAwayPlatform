/*
 * Copyright (C), 2025-2030, 华中师范大学计算机学院
 * FileName: common.h
 * Author: 汪浩
 * Date: 2025-5-25
 * Description: 通用配置文件类
 * History:
 * <author>   <time>      <version>    <desc>
 * 汪浩        2025-2-25   1.0          初始文件
 */
 #pragma once


#include <string>
#include <json/json.h>


namespace TakeAwayPlatform 
{
    // 配置加载
    Json::Value load_config(const std::string& path);


    // 数据库配置结构
    struct DBConfig {
        std::string host;
        int port;
        std::string user;
        std::string password;
        std::string database;
    };

}