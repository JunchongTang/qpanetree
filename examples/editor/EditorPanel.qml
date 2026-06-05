// 叶子内容示例。viewId 从 PaneTreeView attached property 读取。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PaneTree

Item {
    id: root

    // 通过 attached property 拿当前 leaf 的 viewId（库内部注入）
    readonly property string viewId: PaneTreeView.viewId

    signal splitHorizontalRequested()
    signal splitVerticalRequested()
    signal closeRequested()

    Rectangle {
        anchors.fill: parent
        color: "#252526"
        border.color: "#3e3e42"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                height: 35
                color: "#2d2d2d"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 4
                    spacing: 4

                    Label {
                        text: "📄 " + root.viewId
                        color: "#cccccc"
                        font.pixelSize: 13
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
                Label {
                    anchors.centerIn: parent
                    text: "Panel: " + root.viewId
                    color: "#858585"
                    font.pixelSize: 20
                }
            }
        }
    }
}
