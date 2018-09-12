file(RELATIVE_PATH dir ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/hello)

# Set up the command to run the tutorial command on your own tutorial file that generates a
# tutorial in the subdirectory of this file
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/hello/.tute_config" 
"set(TUTE_COMMAND \"python;\
${CMAKE_SOURCE_DIR}/projects/sel4-tutorials/template.py;\
--tut-file;${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld.md;\
--out-dir;\${output_dir};\
--input-files;\${input_files};\
--output-files;\${output_files};\")")

# generate tutorial sources into directory
GenerateTutorial(${dir})

# Add the tutorial directory the same as any other project
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/hello ${CMAKE_CURRENT_BINARY_DIR}/hello)
