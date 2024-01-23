find_package(Lua)

if(%{Lua_FOUND})
    add_custom_command(
        OUTPUT fuse-tests.c
        COMMAND lua
        ARGS "${CMAKE_CURRENT_SOURCE_DIR}/generate.lua"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests.expected"
            "fuse-tests.c"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests.in"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests.expected"
                "${CMAKE_CURRENT_SOURCE_DIR}/generate.lua"
        COMMENT "Generating fuse-tests.c")
    add_executable(fuse-tests main.c fuse-tests.c)
    target_include_directories(fuse-tests PRIVATE .)
    target_link_libraries(fuse-tests z80)
    target_compile_options(fuse-tests PRIVATE -Wall -Wextra)

    add_test(NAME fuse COMMAND ./fuse-tests)
endif()