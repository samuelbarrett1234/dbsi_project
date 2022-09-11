# Add source to this project's executable.
add_executable (dbsi_project_unit_tests
    "main.cpp" "dbsi_query_tests.cpp"
)
target_include_directories(dbsi_project_unit_tests
    PUBLIC "${PROJECT_SOURCE_DIR}/lib_dbsi_project"
)
target_include_directories(dbsi_project_unit_tests PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(dbsi_project_unit_tests PUBLIC lib_dbsi_project)
target_link_libraries(dbsi_project_unit_tests PRIVATE ${Boost_LIBRARIES})
add_test(NAME dbsi_project_unit_tests COMMAND dbsi_project_unit_tests)
