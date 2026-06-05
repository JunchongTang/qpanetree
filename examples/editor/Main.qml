// panetree 完整使用 demo：水平/竖直分屏 + 关闭 + 撤销/重做 + 持久化。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import PaneTree

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: "panetree demo"
    color: "#1e1e1e"

    Settings {
        id: settings
        property var splitLayout
    }

    Component.onCompleted: {
        if (settings.splitLayout) paneTree.restoreState(settings.splitLayout)
        else                      paneTree.reset("editor-1")
    }
    Component.onDestruction: settings.splitLayout = paneTree.saveState()

    PaneTreeModel {
        id: paneTree
        // 业务侧 demo 钩子。库本身的日志走 QLoggingCategory "panetree"，默认关。
        // 用 QT_LOGGING_RULES="panetree.debug=true" 打开看 dump() 输出。
        onViewCreated: (vid) => console.log("Panel created:", vid)
        onViewClosed: (vid) => console.log("Panel closed:", vid)
    }

    header: ToolBar {
        height: 36
        background: Rectangle { color: "#2d2d2d" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8; anchors.rightMargin: 8
            spacing: 4

            ToolButton {
                text: "⊞ 水平分屏"
                onClicked: {
                    // 空树先建根 leaf；非空就 split 第一个 leaf。
                    if (!paneTree.createRootLeaf("editor-1"))
                        paneTree.split(paneTree.allViewIds()[0], Qt.Horizontal)
                }
            }
            ToolButton {
                text: "⊟ 垂直分屏"
                onClicked: {
                    if (!paneTree.createRootLeaf("editor-1"))
                        paneTree.split(paneTree.allViewIds()[0], Qt.Vertical)
                }
            }
            Rectangle { width: 1; height: 20; color: "#555" }
            ToolButton {
                text: "↩ 撤销"; enabled: paneTree.canUndo
                onClicked: paneTree.undo()
            }
            ToolButton {
                text: "↪ 重做"; enabled: paneTree.canRedo
                onClicked: paneTree.redo()
            }
            Rectangle { width: 1; height: 20; color: "#555" }
            ToolButton {
                text: "重置"
                onClicked: paneTree.reset("editor-1")
            }
            Item { Layout.fillWidth: true }
            Label {
                text: "面板数: " + paneTree.leafCount
                color: "#858585"
                font.pixelSize: 12
            }
        }
    }

    PaneTreeView {
        id: paneView
        anchors.fill: parent
        model: paneTree

        leafDelegate: Component {
            EditorPanel {
                onSplitHorizontalRequested: paneTree.split(viewId, Qt.Horizontal)
                onSplitVerticalRequested:   paneTree.split(viewId, Qt.Vertical)
                // demo 不限制 close —— 可以一直关到空。宿主想限制就加：
                //   enabled: paneTree.leafCount > 1
                onCloseRequested: paneTree.close(viewId)
            }
        }
    }

    // 空树时显示 empty-state 提示（覆盖在 PaneTreeView 上）
    Rectangle {
        anchors.fill: paneView
        visible: !paneTree.root
        color: "#1e1e1e"
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 12
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "No panels"
                color: "#cccccc"
                font.pixelSize: 18
            }
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: "+ 新建初始面板"
                onClicked: paneTree.createRootLeaf("editor-1")
            }
        }
    }
}
