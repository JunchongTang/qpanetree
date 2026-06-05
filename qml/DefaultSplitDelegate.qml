// 默认 split delegate——用户没传 splitDelegate 时用这个。
// 库内部把 PaneTreeView attached（orientation / ratio / node / view）写到这个
// SplitView 的 root。Loader 通过外层 split.id 读这些 attached（attached 不会
// 自动从父继承到子）。
// 库会向这个 SplitView append 两个子节点（first / second 递归 PaneTreeViewImpl）。

import QtQuick
import QtQuick.Controls
import PaneTree

SplitView {
    id: split

    orientation: PaneTreeView.orientation

    // SplitView.handle 是 Component，每个 handle 槽位实例化一次。
    handle: Loader {
        // 用户传了 handleDelegate 就用用户的，否则用默认。
        sourceComponent: (split.PaneTreeView.view && split.PaneTreeView.view.handleDelegate)
            ? split.PaneTreeView.view.handleDelegate
            : _defaultHandleComp
        // 把 split 的 orientation 注入到 handle item 的 attached——用户的
        // handleDelegate 通过 `PaneTreeView.orientation` 读取。
        onLoaded: if (item)
            item.PaneTreeView.orientation = Qt.binding(() => split.orientation)
    }

    // 默认 handle 样式（用户没传 handleDelegate 时用）
    Component {
        id: _defaultHandleComp
        Rectangle {
            implicitWidth:  PaneTreeView.orientation === Qt.Horizontal ? 4 : 0
            implicitHeight: PaneTreeView.orientation === Qt.Vertical   ? 4 : 0
            color: SplitHandle.pressed ? "#569cd6"
                 : SplitHandle.hovered ? "#3e3e42"
                 :                       "#2d2d2d"
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    // 拖完写比例回 model
    onResizingChanged: if (!resizing) PaneTreeView.commitRatio(split)
}
