# 启用CTest测试功能
# enable_testing()

# 查找Lit测试框架（确保已安装）
find_package(LLVM 18.1 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM Tools: ${LLVM_TOOLS_BINARY_DIR}")

set(CMAKE_LOX_TARGET "clox" CACHE STRING "Lox Target for Testing, could be 'clox', 'jlox', default is 'clox'")
set(LOX_TEST_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "Lox Test Source Directory")
set(LOX_TEST_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "Lox Test Binary Directory")

add_custom_target(check DEPENDS test)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py @ONLY)

add_custom_target(
    test
    COMMAND
    # /home/codespace/.python/current/bin/lit -s --config-prefix /workspaces/lox-interpreter/build/tests/lit /workspaces/lox-interpreter/tests
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lox-lit ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${LOX_TARGET}    # 确保测试可执行文件已构建
)
