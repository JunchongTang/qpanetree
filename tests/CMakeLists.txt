# qpanetree unit tests —— Qt Test 框架。
# 通过根目录 -DQPANETREE_BUILD_TESTS=ON 启用。
# 跑：ctest --test-dir build/third_party/qpanetree/tests --output-on-failure

find_package(Qt6 6.8 REQUIRED COMPONENTS Test)

function(_qpanetree_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        Qt6::Test
        Qt6::Core
        qpanetree
    )
    set_target_properties(${name} PROPERTIES MACOSX_BUNDLE FALSE)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

_qpanetree_add_test(tst_panenode)
_qpanetree_add_test(tst_panetreemodel)
