#include <fstream>
#include <sstream>
#include <iostream>

#include "common.h"


namespace TakeAwayPlatform
{
    Json::Value load_config(const std::string& path) 
    {
        std::cout << "Loading config, path:" << path << std::endl;
        std::cout.flush();

        std::ifstream configFile(path);
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to open config file: " + path);
        }

        std::cout << "Opening config success." << std::endl;
        std::cout.flush();

        std::stringstream buffer;
        buffer << configFile.rdbuf();
        configFile.close();

        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errors;
        
        if (!Json::parseFromStream(builder, buffer, &root, &errors)) {
            throw std::runtime_error("JSON parse error: " + errors);
        }
        
        return root;
    }
}

