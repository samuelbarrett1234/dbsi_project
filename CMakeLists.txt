cmake_minimum_required (VERSION 3.21)


project("dbsi_project" VERSION 2.0.0 LANGUAGES CXX)


include(CTest)


# see useful CMake CPP talk
# https://youtu.be/m0DwB4OvDXk?t=637
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_definitions( -DBOOST_ALL_NO_LIB )
FIND_PACKAGE(Boost 1.75.0)


add_subdirectory("lib_dbsi_project")
add_subdirectory("dbsi_project")
add_subdirectory("dbsi_project_unit_tests")
add_subdirectory("dbsi_project_integration_tests")
