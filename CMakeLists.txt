# qpanetree —— 递归分割面板的 QML/C++ 库。
#
# 提供 QPaneNode (节点) / QPaneTreeModel (模型) / QPaneTreeView (QQuickItem 视图)。
# C++ 类放在 QPaneTree 命名空间下，QML URI 是 PaneTree（QML 类型名不带 Q 前缀）。
#
# 用法：
#   add_subdirectory(third_party/qpanetree)
#   target_link_libraries(yourapp PRIVATE qpanetree qpanetreeplugin)
#
# Examples 由 QPANETREE_BUILD_EXAMPLES 控制（默认 OFF）。

# 作为 standalone 工程时（直接 cmake -S third_party/qpanetree/）需要 cmake_min
# + project()；作为 add_subdirectory 集成时父工程已经搞过这些，跳过。
if(CMAKE_CURRENT_LIST_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.21)
    project(qpanetree CXX)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Qml)

option(QPANETREE_BUILD_EXAMPLES "Build qpanetree example apps" OFF)
option(QPANETREE_BUILD_TESTS "Build qpanetree unit tests" OFF)

qt_add_library(qpanetree STATIC)

qt_add_qml_module(qpanetree
    URI PaneTree
    VERSION 1.0
    SOURCES
        include/QPaneTree/QPaneNode.h               src/QPaneNode.cpp
        include/QPaneTree/QPaneTreeModel.h          src/QPaneTreeModel.cpp
        include/QPaneTree/QPaneTreeView.h           src/QPaneTreeView.cpp
        include/QPaneTree/QPaneTreeViewAttached.h   src/QPaneTreeViewAttached.cpp
        include/QPaneTree/QPaneTreeLog.h            src/QPaneTreeLog.cpp
    QML_FILES
        qml/PaneTreeViewImpl.qml
        qml/DefaultSplitDelegate.qml
        qml/DefaultHandleDelegate.qml
    RESOURCE_PREFIX "/qt/qml"
)

target_include_directories(qpanetree
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        # 让 qt_add_qml_module 生成的代码（#include <QPaneNode.h>）能找到头
        ${CMAKE_CURRENT_SOURCE_DIR}/include/QPaneTree
)

target_link_libraries(qpanetree PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

if(QPANETREE_BUILD_EXAMPLES)
    add_subdirectory(examples/editor)
endif()

if(QPANETREE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
