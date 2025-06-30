cmake_minimum_required(VERSION 3.12)
project(TakeAwayPlatform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置项目目录
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(SOURCE_DIR ${PROJECT_ROOT}/src)
set(INCLUDE_DIR ${PROJECT_ROOT}/include)
set(CONFIG_DIR ${PROJECT_ROOT}/config)

# 设置头文件路径
include_directories(
    ${INCLUDE_DIR}
    ${SOURCE_DIR}/database
    ${SOURCE_DIR}/http
    ${SOURCE_DIR}/user 
)

# 添加 cpp-httplib
set(CPP_HTTPLIB_INCLUDE_DIR ${PROJECT_ROOT}/lib/cpp-httplib/include)
include_directories(${CPP_HTTPLIB_INCLUDE_DIR})

# 添加 MySQL Connector
set(MYSQL_CONNECTOR_ROOT ${PROJECT_ROOT}/lib/mysql-connector)
include_directories(
    ${MYSQL_CONNECTOR_ROOT}/include
    ${MYSQL_CONNECTOR_ROOT}/include/jdbc
    ${MYSQL_CONNECTOR_ROOT}/include/mysqlx
)

# 添加 JSONCPP
set(JSONCPP_ROOT ${PROJECT_ROOT}/lib/jsoncpp)
include_directories(${JSONCPP_ROOT}/src/include)

# 添加 JSONCPP 源文件
file(GLOB_RECURSE JSONCPP_SOURCES
    ${JSONCPP_ROOT}/src/src/lib_json/*.cpp
    ${JSONCPP_ROOT}/src/src/lib_json/*.h
)

# 创建 JSONCPP 静态库
add_library(jsoncpp_lib STATIC ${JSONCPP_SOURCES})

# 添加 MySQL Connector 库路径
link_directories(${MYSQL_CONNECTOR_ROOT}/lib)

# 添加源文件
file(GLOB_RECURSE SOURCES 
    ${SOURCE_DIR}/*.cpp
    ${SOURCE_DIR}/utils/*.cpp
    ${SOURCE_DIR}/*.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE 
        jsoncpp_lib  # 链接 JSONCPP 库
        mysqlcppconn8
        ssl
        crypto
        z
        pthread     # 链接 cpp-httplib 所需的 pthread 库
)

# 设置运行时库路径
set(CMAKE_INSTALL_RPATH "${MYSQL_CONNECTOR_ROOT}/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

#sz新添加输出目录output
# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/lib)

# 为不同配置设置输出目录
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/output/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/output/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/output/lib)
endforeach()

# 安装配置文件
install(DIRECTORY ${CONFIG_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/config)