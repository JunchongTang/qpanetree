// 叶子内容示例。所有数据从 PaneTreeView attached property 读取。
//
// 这个面板顺带演示库的一条核心承诺:**一个 viewId 一份 delegate 实例,活到这个
// viewId 关掉为止**。分屏 / 最大化 / 关掉相邻格都不会重建它。
//
// 面板里放了三样东西,分别代表三种状态,分屏之后看它们的表现:
//
//   · **实例 #N · 已存活 Ns** —— 纯实例状态。重建就会换号、归零;
//   · **草稿(不保存)**       —— 纯视图状态。重建就会清空;
//   · **Notes(自动保存)**    —— 存在 `node.data` 里。**重建也还在** ——
//     分屏时库会把 data 迁到新节点上。它是对照组:只看它的话缺陷是隐形的。

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

    //! 第几份 delegate 实例(由 Main.qml 在创建时递增着注入)。
    property int instanceSeq: 0
    //! 这份实例出生的时刻。**不是这一格出生的时刻** —— 两者分家正是本示例要演的事。
    readonly property double bornAt: Date.now()
    property int aliveSeconds: 0
    Timer {
        running: true; repeat: true; interval: 1000
        onTriggered: root.aliveSeconds = Math.floor((Date.now() - root.bornAt) / 1000)
    }

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

                    // ── delegate 实例的身份与寿命 ────────────────────────────
                    // 分屏 / 最大化之后这一行**不该变**。变了就说明 delegate 被重建
                    // 了 —— 库 1.1 之前正是如此(渲染层树一变就整棵重来)。
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "实例 #" + root.instanceSeq + " · 已存活 " + root.aliveSeconds + " 秒"
                        color: "#569cd6"
                        font.pixelSize: 12
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.maximumWidth: 260
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        text: "分屏 / 最大化 / 关掉相邻格之后,上面这行和下面的草稿都不该变。"
                        color: "#666"
                        font.pixelSize: 10
                    }

                    // 纯视图状态：不写进 node.data，重建就没。
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "草稿 (不保存):"
                        color: "#666"
                        font.pixelSize: 11
                    }
                    TextField {
                        Layout.preferredWidth: 240
                        Layout.alignment: Qt.AlignHCenter
                        placeholderText: "随便打点字，然后分屏…"
                    }

                    // 演示 data 持久化：用户在 panel 里输东西，应用重启后还在。
                    // **对照组** —— 它存在 node.data 里，分屏时库把 data 迁到新节点上，
                    // 所以就算 delegate 被重建它也还在。只看这一栏是发现不了缺陷的。
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
