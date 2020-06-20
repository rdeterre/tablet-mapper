project(tablet-mapper)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(LIBUDEV REQUIRED libudev)


add_executable(${PROJECT_NAME} src/tablet_mapper.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${GTKMM_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBINPUT_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUDEV_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${GTKMM_LIBRARY_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBINPUT_LIBRARY_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBUDEV_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${GTKMM_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBINPUT_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUDEV_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${CONAN_LIBS})

if (TESTS)
  enable_testing()
  add_executable(libinput_t src/libinput_t.cpp)
  add_test(libinput_t ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libinput_t)
endif ()
