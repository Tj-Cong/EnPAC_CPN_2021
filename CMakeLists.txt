cmake_minimum_required(VERSION 3.16)
project(EnPAC_CPN)

set(CMAKE_CXX_STANDARD 14)
set(LINK_DIR /usr/local/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -pthread")

link_directories(${LINK_DIR})
link_libraries(tcmalloc)

include_directories(.)

add_executable(EnPAC_CPN
        BA/atomic.h
        BA/atomic.cpp
        BA/set.h
        BA/Syntax_Tree.h
        BA/Syntax_Tree.cpp
        BA/VWAA.h
        BA/VWAA.cpp
        BA/buchi.h
        BA/buchi.cpp
        BA/tinystr.cpp
        BA/tinystr.h
        BA/tinyxml.cpp
        BA/tinyxml.h
        BA/tinyxmlerror.cpp
        BA/tinyxmlparser.cpp
        main.cpp
        base.h
        expression.h
        expression.cpp
        sort.h sort.cpp
        CPN.h CPN.cpp
        CPN_RG.h
        CPN_RG.cpp
        CPN_Product.h
        CPN_Product.cpp criteria.cpp)

target_link_libraries(EnPAC_CPN tcmalloc)
