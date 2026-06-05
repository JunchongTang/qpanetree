# qpanetree editor demo
qt_add_executable(qpanetree-editor-demo
    main.cpp
)

qt_add_qml_module(qpanetree-editor-demo
    URI PaneTreeEditorDemo
    VERSION 1.0
    QML_FILES
        Main.qml
        EditorPanel.qml
)

set_target_properties(qpanetree-editor-demo PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(qpanetree-editor-demo PRIVATE
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    qpanetree
    qpanetreeplugin
)
