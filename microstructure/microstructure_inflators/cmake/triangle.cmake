if(TARGET triangle::triangle)
    return()
endif()

message(STATUS "Third-party: creating target 'triangle::triangle'")

include(FetchContent)
FetchContent_Declare(
    triangle
    GIT_REPOSITORY https://github.com/libigl/triangle.git
    GIT_TAG        d6761dd691e2e1318c83bf7773fea88d9437464a
)

FetchContent_MakeAvailable(triangle)
add_library(triangle::triangle ALIAS triangle)

target_include_directories(triangle INTERFACE "${triangle_SOURCE_DIR}")

set_target_properties(triangle PROPERTIES FOLDER ThirdParty)