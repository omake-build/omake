# omake 的 CMake 项目
add_executable(omake 
    "scr/omake.cpp" 
    "include/omake.h" 
    "scr/parser.cpp" 
    "scr/builder.cpp" 
    "scr/stats.cpp"
)

# 设置头文件包含目录
target_include_directories(omake PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置C++标准
set_target_properties(omake PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# MSVC特定设置
if(MSVC)
    target_compile_options(omake PRIVATE 
        /std:c++17
        /Zc:__cplusplus
    )
else()
    target_compile_options(omake PRIVATE -std=c++17)
endif()

# 添加文件系统库链接
if(MSVC)
    # 使用标准库自动链接，不需要显式链接
else()
    target_link_libraries(omake PRIVATE stdc++fs)
endif()

# 调试配置添加符号信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        
    else()
        target_compile_options(omake PRIVATE -g)
    endif()
endif()