// 默认分割条样式——用户没传 handleDelegate 时用这个。
// 库内部会把 PaneTreeView.orientation attached property 写到 root，可读取。

import QtQuick
import QtQuick.Controls
import PaneTree

Rectangle {
    id: handleRoot

    // PaneTreeView.orientation attached 由库注入。这里冗余声明 property 仅用于
    // qmlls 安静——运行时 attached property 一直可用。
    property int orientationHint: PaneTreeView.orientation

    implicitWidth:  orientationHint === Qt.Horizontal ? 4 : 0
    implicitHeight: orientationHint === Qt.Vertical   ? 4 : 0

    color: SplitHandle.pressed ? "#569cd6"
         : SplitHandle.hovered ? "#3e3e42"
         :                       "#2d2d2d"

    Behavior on color { ColorAnimation { duration: 100 } }
}
