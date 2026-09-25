// 递归渲染层。C++ PaneTreeView 加载这个 QML 作为内部 child；splitDelegate
// 里也会动态 createObject 这个 QML 当 first / second 子树。
//
// 接受属性：
//   view             —— PaneTreeView*（C++ 壳）
//   model            —— PaneTreeModel*
//   leafDelegate     —— Component（必填）
//   splitDelegate    —— Component（可选，默认走 DefaultSplitDelegate）
//   handleDelegate   —— Component（可选）
//   node             —— 当前节点；顶层默认 model.root，递归层由父传入
//
// 工作流：
//   Leaf  → **插槽**：向 view 要这个 viewId 的 delegate 实例（QPaneTreeView 按
//           viewId 缓存，见那边的头注），reparent 进来。**不在这儿建、也不在这儿毁**
//           —— 本文件在树结构一变时整棵重建，delegate 建在这儿的话一次分割就会把
//           幸存那一格的内容销毁重建。
//   Split → Loader 加载 splitDelegate；后续 dynamically createObject 两份
//           PaneTreeViewImpl.qml 作为它的 children（SplitView 自动按 orientation
//           layout 这两个 child）

import QtQuick
import QtQuick.Controls
import PaneTree

Item {
    id: root

    // 每个节点(窗格/子树)在父 SplitView 里的最小像素尺寸 —— 防止拖到内容(会话栏
    // 等)重叠的窄度。SplitView 按自身 orientation 取 minimumWidth 或 minimumHeight,
    // 并在**拖动过程中**实时夹住(非松手才生效)。非 SplitView 子项时该 attached 被忽略。
    SplitView.minimumWidth: 220
    SplitView.minimumHeight: 140

    property var view
    property var model
    property Component leafDelegate
    property Component splitDelegate
    property Component handleDelegate
    // 顶层 node：maximizedLeaf 非 null 时取它（渲染该 leaf 占满），否则正常树根。
    // 递归子层的 node 由 splitWrap 传入，不走这个 fallback。
    property var node: model
        ? (model.maximizedLeaf ? model.maximizedLeaf : model.root)
        : null

    Loader {
        id: bodyLoader
        anchors.fill: parent
        // 纯绑定——root.node 变化或 root.node.isSplit（节点类型切换）变化时
        // 自动重算。**不要**用 Connections + imperative 赋值，那会永久破坏
        // 此绑定，后续 root.node 改变就不再触发 sourceComponent 切换。
        //
        // node 为 null（model 没有 root，或递归层父节点降级）时直接不加载，
        // 避免渲染出空 viewId 的鬼 leaf。
        sourceComponent: !root.node
                         ? null
                         : root.node.isSplit ? splitWrap : leafWrap
    }

    // ── Leaf slot ────────────────────────────────────────────────────────
    //
    // 只是个**挂载点**。实例归 QPaneTreeView 按 viewId 缓存，这里 acquire / release。
    //
    // attached property 也不在这儿写了：实例比本文件活得长，`Qt.binding(() => root.node)`
    // 这种绑到渲染层作用域的写法在本文件销毁之后就悬空。改由 QPaneTreeView 在模型
    // 变化时推（refreshAttached）—— node/isActive 本来就是模型的事。
    Component {
        id: leafWrap
        Item {
            id: slot
            anchors.fill: parent

            readonly property string vid: root.node ? root.node.viewId : ""
            //! 当前挂着的那份实例。给下面两条尺寸 Binding 用。
            property Item leaf: null
            //! 已经 acquire 过的 viewId。节点被换成另一个 view（move / restoreState）
            //! 时要先把上一个放回去，否则它会一直挂在一个已经不属于它的插槽上。
            property string mountedVid: ""

            function _mount() {
                if (slot.mountedVid === slot.vid)
                    return
                if (slot.mountedVid !== "" && root.view)
                    root.view.releaseLeaf(slot.mountedVid, slot)
                slot.mountedVid = slot.vid
                slot.leaf = (root.view && slot.vid !== "")
                            ? root.view.acquireLeaf(slot.vid, slot) : null
            }

            onVidChanged: _mount()
            Component.onCompleted: _mount()
            Component.onDestruction: if (root.view && slot.mountedVid !== "")
                                         root.view.releaseLeaf(slot.mountedVid, slot)

            // 挂载期间由插槽驱动实例的尺寸。
            //
            // **restoreMode: RestoreNone** —— 插槽销毁时 Binding 跟着没，默认的
            // RestoreBindingOrValue 会把宽高退回 0。让宽度在分割那一瞬间过一趟 0，
            // 宿主里的终端就会按 0 列回流一次（vim / less 这类全屏程序当场刷花）。
            Binding {
                target: slot.leaf
                property: "width"
                value: slot.width
                when: slot.leaf !== null
                restoreMode: Binding.RestoreNone
            }
            Binding {
                target: slot.leaf
                property: "height"
                value: slot.height
                when: slot.leaf !== null
                restoreMode: Binding.RestoreNone
            }
        }
    }

    // ── Split wrapper ────────────────────────────────────────────────────
    // 用户 splitDelegate（或默认 DefaultSplitDelegate）作 root，库再 append
    // 两个递归 PaneTreeViewImpl 当 children。SplitView 按 orientation layout。
    //
    // 用 Loader 包裹动态创建的子 impl —— sourceComponent 改变（树重构）时，
    // Loader 自动同步销毁 item，没有 deleteLater 的异步延迟空窗。
    Component {
        id: splitWrap
        Loader {
            id: splitContainerLoader
            anchors.fill: parent

            // 用户传了 splitDelegate → sourceComponent；否则 source URL 加载默认。
            Component.onCompleted: {
                if (root.splitDelegate) {
                    splitContainerLoader.sourceComponent = root.splitDelegate
                } else {
                    splitContainerLoader.source =
                        Qt.resolvedUrl("DefaultSplitDelegate.qml")
                }
            }

            // splitWrap 销毁时同步清掉动态创建的 child impls——Loader.item 由
            // QML 自己管理（destroy() 会报 indestructible），动态 createObject
            // 出来的 impls 必须显式 destroy 才能立刻消失（否则 deleteLater
            // 期间它们的 binding 在 first/second 变 null 时渲染空 leaf）。
            Component.onDestruction: _disposeChildren()

            onLoaded: if (item) {
                // 1. attached 注入到 splitDelegate root
                item.PaneTreeView.view = root.view
                item.PaneTreeView.model = root.model
                item.PaneTreeView.node = Qt.binding(() => root.node)
                item.PaneTreeView.nodeId = Qt.binding(() => {
                    return root.node ? root.node.nodeId : ""
                })
                item.PaneTreeView.orientation = Qt.binding(() => {
                    return root.node ? root.node.orientation : Qt.Horizontal
                })
                item.PaneTreeView.ratio = Qt.binding(() => {
                    return root.node ? root.node.ratio : 0.5
                })

                // 2. 动态创建两个 PaneTreeViewImpl 作为 splitDelegate 子节点
                _spawnRecursiveChildren(item)
            }

            // 持有动态子的引用，方便在 destruction 时主动 destroy()。
            property var _childImpls: []

            function _spawnRecursiveChildren(container) {
                _disposeChildren()

                const comp = Qt.createComponent(
                    Qt.resolvedUrl("PaneTreeViewImpl.qml"))
                if (comp.status === Component.Error) {
                    console.warn("[panetree] failed to load impl:",
                                 comp.errorString())
                    return
                }
                const horiz = root.node.orientation === Qt.Horizontal

                const c1 = comp.createObject(container, {
                    view: root.view,
                    model: root.model,
                    leafDelegate: root.leafDelegate,
                    splitDelegate: root.splitDelegate,
                    handleDelegate: root.handleDelegate,
                    node: Qt.binding(() => {
                        return root.node ? root.node.first : null
                    })
                })
                if (c1) {
                    if (horiz) {
                        c1.SplitView.preferredWidth =
                            Qt.binding(() => container.width *
                                       (root.node ? root.node.ratio : 0.5))
                    } else {
                        c1.SplitView.preferredHeight =
                            Qt.binding(() => container.height *
                                       (root.node ? root.node.ratio : 0.5))
                    }
                    _childImpls.push(c1)
                }

                const c2 = comp.createObject(container, {
                    view: root.view,
                    model: root.model,
                    leafDelegate: root.leafDelegate,
                    splitDelegate: root.splitDelegate,
                    handleDelegate: root.handleDelegate,
                    node: Qt.binding(() => {
                        return root.node ? root.node.second : null
                    })
                })
                if (c2) {
                    if (horiz) {
                        c2.SplitView.fillWidth = true
                    } else {
                        c2.SplitView.fillHeight = true
                    }
                    _childImpls.push(c2)
                }
            }

            function _disposeChildren() {
                for (var i = 0; i < _childImpls.length; ++i) {
                    if (_childImpls[i]) {
                        _childImpls[i].destroy()
                    }
                }
                _childImpls = []
            }
        }
    }
}
