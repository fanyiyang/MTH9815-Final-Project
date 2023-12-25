cmake_minimum_required(VERSION 3.26)
project(tradingsystem)

set(CMAKE_CXX_STANDARD 20)
set(Boost_INCLUDE_DIR /Users/fan/Documents/boost_1_83_0)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(.)

add_executable(tradingsystem
        executionservice.hpp
        historicaldataservice.hpp
        inquiryservice.hpp
        marketdataservice.hpp
        positionservice.hpp
        pricingservice.hpp
        products.hpp
        riskservice.hpp
        soa.hpp
        streamingservice.hpp
        tradebookingservice.hpp
        main.cpp
        functions.hpp
        algostreamingservice.hpp)
