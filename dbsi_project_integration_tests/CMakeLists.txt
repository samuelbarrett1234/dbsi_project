add_test(NAME dbsi_project_integration_tests
    COMMAND ${CMAKE_COMMAND} -E compare_files temp/family_guy_output.txt family_guy_answers.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dbsi_project_integration_tests
)
