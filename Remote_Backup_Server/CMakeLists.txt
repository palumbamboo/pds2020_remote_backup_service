cmake_minimum_required(VERSION 3.17)
project(Remote_Backup_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -pthread)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fPIC -fopenmp  -DBOOST_LOG_DYN_LINK")

add_executable(Remote_Backup_Server main.cpp Server.cpp Server.h Session.cpp Session.h Logger.h Logger.cpp)

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

find_package(Boost COMPONENTS system filesystem log log_setup REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(Remote_Backup_Server ${Boost_LIBRARIES})
endif()

