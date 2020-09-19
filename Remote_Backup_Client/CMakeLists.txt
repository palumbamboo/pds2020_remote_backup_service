cmake_minimum_required(VERSION 3.17)
project(Remote_Backup_Client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(Boost COMPONENTS system filesystem program_options REQUIRED)
if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIR})
endif()

add_executable(Remote_Backup_Client main.cpp FileWatcher.cpp FileWatcher.h Client.cpp Client.h UploadQueue.cpp UploadQueue.h Message.cpp Message.h FileToUpload.cpp FileToUpload.h)
target_link_libraries( Remote_Backup_Client ${Boost_LIBRARIES})