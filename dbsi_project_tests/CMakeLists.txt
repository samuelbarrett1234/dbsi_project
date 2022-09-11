# Add source to this project's executable.
add_executable (dbsi_project_tests
    "main.cpp"
)
target_include_directories(dbsi_project_tests PUBLIC "${PROJECT_SOURCE_DIR}/lib_dbsi_project")
target_include_directories(dbsi_project_tests PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(dbsi_project_tests PUBLIC lib_dbsi_project)
target_link_libraries(dbsi_project_tests PRIVATE ${Boost_LIBRARIES})
add_test(NAME dbsi_project_tests COMMAND dbsi_project_tests)
