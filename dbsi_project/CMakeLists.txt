add_executable(dbsi_project "main.cpp")
target_include_directories(dbsi_project
    PUBLIC "${PROJECT_SOURCE_DIR}/lib_dbsi_project"
)
target_link_libraries(dbsi_project PUBLIC lib_dbsi_project)
