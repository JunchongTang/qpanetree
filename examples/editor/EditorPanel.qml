// 叶子内容示例。所有数据从 PaneTreeView attached property 读取。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PaneTree

Item {
    id: root

    // attached property —— 库注入
    readonly property string viewId: PaneTreeView.viewId
    readonly property bool isActive: PaneTreeView.isActive
    readonly property var node: PaneTreeView.node

    signal splitHorizontalRequested()
    signal splitVerticalRequested()
    signal maximizeRequested()
    signal closeRequested()
    signal activateRequested()

    // 点击 panel 任意位置激活
    TapHandler {
        onTapped: root.activateRequested()
    }

    Rectangle {
        anchors.fill: parent
        color: "#252526"
        // active 时画亮色边框
        border.color: root.isActive ? "#569cd6" : "#3e3e42"
        border.width: root.isActive ? 2 : 1

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                height: 35
                color: root.isActive ? "#37373d" : "#2d2d2d"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 4
                    spacing: 4

                    Label {
                        text: (root.isActive ? "● " : "  ") + "📄 " + root.viewId
                        color: root.isActive ? "#ffffff" : "#cccccc"
                        font.pixelSize: 13
                        font.bold: root.isActive
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    ToolButton {
                        text: "⧉"
                        implicitWidth: 24; implicitHeight: 24
                        font.pixelSize: 14
                        ToolTip.text: "水平分屏"; ToolTip.visible: hovered
                        onClicked: root.splitHorizontalRequested()
                    }
                    ToolButton {
                        text: "⬛"
                        implicitWidth: 24; implicitHeight: 24
                        font.pixelSize: 10
                        ToolTip.text: "垂直分屏"; ToolTip.visible: hovered
                        onClicked: root.splitVerticalRequested()
                    }
                    ToolButton {
                        text: "⛶"
                        implicitWidth: 24; implicitHeight: 24
                        font.pixelSize: 14
                        ToolTip.text: "最大化 / 还原"; ToolTip.visible: hovered
                        onClicked: root.maximizeRequested()
                    }
                    ToolButton {
                        text: "✕"
                        implicitWidth: 24; implicitHeight: 24
                        font.pixelSize: 12
                        ToolTip.text: "关闭"; ToolTip.visible: hovered
                        onClicked: root.closeRequested()
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Panel: " + root.viewId
                        color: "#858585"
                        font.pixelSize: 20
                    }
                    // 演示 data 持久化：用户在 panel 里输东西，应用重启后还在
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Notes (auto-saved):"
                        color: "#666"
                        font.pixelSize: 11
                    }
                    TextField {
                        Layout.preferredWidth: 240
                        Layout.alignment: Qt.AlignHCenter
                        placeholderText: "type something..."
                        text: root.node && root.node.data ? (root.node.data.note || "") : ""
                        onEditingFinished: if (root.node) {
                            const d = root.node.data || ({})
                            d.note = text
                            root.node.data = d
                        }
                    }
                }
            }
        }
    }
}
