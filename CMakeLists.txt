project(tablet-mapper)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

add_executable(${PROJECT_NAME} src/tablet_mapper.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${GTKMM_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${GTKMM_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${GTKMM_LIBRARIES})
