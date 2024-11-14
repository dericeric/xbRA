cmake_minimum_required(VERSION 3.10)
project(AutoClicker)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加Unicode支持
add_definitions(-DUNICODE -D_UNICODE)
      
    
    
      
    

# 添加可执行文件
add_executable(AutoClicker src/clicker.cpp)

# ... 其他配置保持不变 ...

# 添加Windows链接库
target_link_libraries(AutoClicker PRIVATE 
    user32.lib 
    psapi.lib
)

# ... 其他配置保持不变 ...

# 设置Windows子系统
set_target_properties(AutoClicker PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
