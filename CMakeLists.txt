cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_COMPILER g++)

project(
    DL_interpreter
    VERSION 0.1
    HOMEPAGE_URL "https://github.com/SibJusha/dl_interpreter"
    LANGUAGES CXX
)
    
add_compile_options(-O3 -Wall -Wextra)

add_executable(DL_interpreter 
    src/main.cpp
    src/parser.cpp 
    src/expressions.cpp
)
