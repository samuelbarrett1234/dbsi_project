add_executable(dbsi_project "main.cpp")
target_include_directories(dbsi_project
    PUBLIC "${PROJECT_SOURCE_DIR}/lib_dbsi_project"
)
target_link_libraries(dbsi_project PUBLIC lib_dbsi_project)


add_custom_command(
    TARGET dbsi_project
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running integration tests on $<TARGET_FILE_NAME:dbsi_project>..."
    COMMAND dbsi_project -i "LOAD family_guy.ttl" -f family_guy_test.txt > temp/family_guy_output.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dbsi_project_integration_tests
)
