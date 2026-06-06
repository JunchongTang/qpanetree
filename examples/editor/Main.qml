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
                    if (paneTree.createRootLeaf("editor-1")) return
                    const target = paneTree.activeLeaf
                        ? paneTree.activeLeaf.viewId
                        : paneTree.allViewIds()[0]
                    paneTree.split(target, Qt.Horizontal)
                }
            }
            ToolButton {
                text: "⊟ 垂直分屏"
                onClicked: {
                    if (paneTree.createRootLeaf("editor-1")) return
                    const target = paneTree.activeLeaf
                        ? paneTree.activeLeaf.viewId
                        : paneTree.allViewIds()[0]
                    paneTree.split(target, Qt.Vertical)
                }
            }
            Rectangle { width: 1; height: 20; color: "#555" }
            ToolButton {
                text: "← 前一个"
                ToolTip.text: "focusPrevious()"; ToolTip.visible: hovered
                onClicked: paneTree.focusPrevious()
            }
            ToolButton {
                text: "→ 后一个"
                ToolTip.text: "focusNext()"; ToolTip.visible: hovered
                onClicked: paneTree.focusNext()
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
                    + (paneTree.maximizedLeaf ? " · 最大化" : "")
                    + (paneTree.activeLeaf ? " · active=" + paneTree.activeLeaf.viewId : "")
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
                onSplitVerticalRequested: paneTree.split(viewId, Qt.Vertical)
                onMaximizeRequested: paneTree.toggleMaximize(node)
                onActivateRequested: paneTree.activeLeaf = node
                // demo 不限制 close —— 可以一直关到空。宿主想限制就加：
                //   enabled: paneTree.leafCount > 1
                onCloseRequested: paneTree.close(viewId)
            }
        }

        // 三点药丸样式的 handle —— PaneTreeView.orientation attached 由库注入
        handleDelegate: Component {
            Rectangle {
                id: handleRoot
                readonly property int _orient: PaneTreeView.orientation
                readonly property bool _horiz: _orient === Qt.Horizontal
                implicitWidth: _horiz ? 7 : 0
                implicitHeight: _horiz ? 0 : 7
                color: "#2d2d2d"

                // 中线
                Rectangle {
                    anchors.centerIn: parent
                    width: handleRoot._horiz ? 1 : parent.width
                    height: handleRoot._horiz ? parent.height : 1
                    color: "#3e3e42"
                    opacity: 0.8
                }
                // 中段药丸
                Rectangle {
                    anchors.centerIn: parent
                    width: handleRoot._horiz ? 5 : 30
                    height: handleRoot._horiz ? 30 : 5
                    radius: 3
                    color: SplitHandle.pressed ? "#569cd6"
                         : SplitHandle.hovered ? "#3e3e42"
                         :                       "#2d2d2d"
                    border.width: 0.5
                    border.color: "#555"
                    Behavior on color { ColorAnimation { duration: 100 } }
                    // 三个点
                    Item {
                        anchors.centerIn: parent
                        width: handleRoot._horiz ? 1 : 11
                        height: handleRoot._horiz ? 11 : 1
                        Repeater {
                            model: 3
                            delegate: Rectangle {
                                required property int index
                                width: 1
                                height: 1
                                radius: 0.5
                                x: handleRoot._horiz ? 0 : index * 5
                                y: handleRoot._horiz ? index * 5 : 0
                                color: "#858585"
                            }
                        }
                    }
                }
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
