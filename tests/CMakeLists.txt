add_executable(tests ./main.c)
target_link_libraries(tests z80)
add_custom_command(TARGET tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/roms/ $<TARGET_FILE_DIR:tests>/roms)

add_test(NAME prelim COMMAND ./tests "./roms/prelim.com")
add_test(NAME zexdoc COMMAND ./tests "./roms/zexdoc.cim")
