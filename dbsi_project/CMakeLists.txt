# CMakeList.txt : CMake project for dbsi_project, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (dbsi_project "dbsi_project.cpp"  "dbsi_rdf_index.h" "dbsi_iterator.h" "dbsi_nlj.h"  "dbsi_dictionary.h" "dbsi_turtle.h" "dbsi_query.h" "dbsi_dictionary_utils.h")
target_compile_features(dbsi_project PRIVATE cxx_std_17)

# TODO: Add tests and install targets if needed.
