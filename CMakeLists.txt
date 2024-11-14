cmake_minimum_required(VERSION 3.10)
project(AutoClicker)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(AutoClicker src/clicker.cpp)

# 添加Windows链接库
target_link_libraries(AutoClicker PRIVATE user32.lib)

# 设置Windows子系统
set_target_properties(AutoClicker PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
)