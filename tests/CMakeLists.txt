# qpanetree unit tests —— Qt Test 框架。
# 通过根目录 -DQPANETREE_BUILD_TESTS=ON 启用。
# 跑：ctest --test-dir build/third_party/qpanetree/tests --output-on-failure

find_package(Qt6 6.8 REQUIRED COMPONENTS Test QuickTest QuickControls2)

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

# ── 渲染层测试（QtQuickTest）─────────────────────────────────────────────
#
# 模型层的 C++ 单测照不到 leaf delegate 的生命周期 —— 那个缺陷整个发生在
# PaneTreeViewImpl.qml + QPaneTreeView 的缓存里。这一组把库对宿主的承诺
# （一个 viewId 一份实例）钉住。
qt_add_executable(tst_qml_view tst_qml_view.cpp)
target_link_libraries(tst_qml_view PRIVATE
    Qt6::QuickTest
    Qt6::Qml
    Qt6::QuickControls2
    qpanetree
    qpanetreeplugin
)
target_compile_definitions(tst_qml_view PRIVATE
    # QuickTest 认这个宏来定位 tst_*.qml。不设的话它从进程 cwd 往下递归扫。
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml"
    QPANETREE_QML_IMPORT_PATH="${CMAKE_BINARY_DIR}/qml"
)
set_target_properties(tst_qml_view PROPERTIES MACOSX_BUNDLE FALSE)
add_test(NAME tst_qml_view COMMAND tst_qml_view)
set_tests_properties(tst_qml_view PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
