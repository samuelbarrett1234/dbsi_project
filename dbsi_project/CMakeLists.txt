cmake_minimum_required (VERSION 3.21)


# Add source to this project's executable.
add_executable (dbsi_project
    "dbsi_project.cpp" "dbsi_dictionary.cpp" "dbsi_dictionary_utils.cpp" "dbsi_turtle.cpp"
    "dbsi_rdf_index.cpp" "dbsi_nlj.cpp" "dbsi_query.cpp" "dbsi_parse_helper.cpp"
    "dbsi_types.cpp"
)
