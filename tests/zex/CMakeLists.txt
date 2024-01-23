add_executable(zex-tests ./main.c)
target_link_libraries(zex-tests z80)
add_custom_command(TARGET zex-tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/zex/roms/ ${CMAKE_CURRENT_BINARY_DIR}/roms)

add_test(NAME prelim COMMAND ./zex-tests "./roms/prelim.com" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME zexdoc COMMAND ./zex-tests "./roms/zexdoc.cim" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
