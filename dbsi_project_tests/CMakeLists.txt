cmake_minimum_required (VERSION 3.21)


# Add source to this project's executable.
add_executable (dbsi_project_tests
    "main.cpp"
)


target_include_directories(dbsi_project_tests PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(dbsi_project_tests PRIVATE ${Boost_LIBRARIES})
add_test(NAME dbsi_project_tests COMMAND dbsi_project_tests)
