find_package(Python3 3.12 REQUIRED)
find_package(LLVM 18.1 REQUIRED CONFIG)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lox-lit.in ${CMAKE_BINARY_DIR}/bin/lox-lit @ONLY)
execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/bin/lox-lit)
