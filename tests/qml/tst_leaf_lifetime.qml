// leaf delegate 的生命周期 —— 本库对宿主的核心承诺:
//
//   **一个 viewId 一份 delegate 实例,活到这个 viewId 关掉为止。**
//
// v1.1 之前不是这样:递归渲染层在树结构一变时整棵重建,于是一次分割会把幸存
// 那一格的 delegate 销毁重建。对编辑器 / 终端类宿主,那等于"分个屏,正在跑的活
// 就没了"。模型层早就把 viewId 当稳定身份在承诺(setRoot 对两边都存在的 id 不发
// close+create 配对),渲染层此前跟这条自相矛盾。
//
// **这些测试比的是对象身份。** "分割之后还找得到一份 delegate"在有缺陷的版本里
// 也是成立的 —— 重建出来的新实例照样在那儿。身份才是"没重建"的唯一证据。

import QtQuick
import QtQuick.Layouts
import QtTest
import PaneTree

Item {
    id: host
    width: 800
    height: 600

    Component {
        id: leafComp
        Rectangle {
            // 每建一份就 ++,用来在身份之外再看一眼"到底建了几次"。
            Component.onCompleted: host.built++
            Component.onDestruction: host.destroyed++
            color: "steelblue"
            // 宿主典型用法:delegate 直接读 attached。
            property string vid: PaneTreeView.viewId
            property bool active: PaneTreeView.isActive
        }
    }

    property int built: 0
    property int destroyed: 0

    /*!
        带布局的 delegate。**专门用来钉"停泊往返之后布局还对不对"。**

        这是实现缓存时真撞上的一个坑:第一版把实例先建在池子里(0×0 且不可见)再搬进
        插槽,而 QQuickLayout **在不可见时不做布局** —— delegate 带着一份按 0 宽算出来的
        布局出生(实测拿到过 width = -16 的 RowLayout),搬进去之后外框尺寸对了、内部却
        没重排过。表现极其隐蔽:点击靠命中测试,照常;悬停靠场景坐标,落到隔壁那颗
        按钮上。用一个纯 Rectangle 当 delegate 是照不出来的。
    */
    Component {
        id: layoutLeafComp
        Item {
            property alias edge: edgeRect
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true; implicitHeight: 20 }
                    // 贴右缘的那一颗 —— 布局没排对的话它的 x 就不对。
                    Rectangle { id: edgeRect; implicitWidth: 20; implicitHeight: 20 }
                }
                Item { Layout.fillWidth: true; Layout.fillHeight: true }
            }
        }
    }

    Component {
        id: rigComp
        Item {
            id: rig
            anchors.fill: parent
            property alias model: m
            property alias view: v
            property Component leaf: leafComp
            PaneTreeModel { id: m }
            PaneTreeView {
                id: v
                anchors.fill: parent
                model: m
                leafDelegate: rig.leaf
            }
        }
    }

    TestCase {
        id: testCase
        name: "LeafLifetime"
        when: windowShown

        function makeRig(rootViewId, leafComponent) {
            const rig = createTemporaryObject(rigComp, host,
                                              leafComponent ? { leaf: leafComponent } : {})
            verify(rig, "rig 创建失败")
            // 模型自带一个 viewId 为 "default" 的根 leaf,所以是 reset 不是
            // createRootLeaf(后者在已有根时返回 false)。
            rig.model.reset(rootViewId)
            // delegate 是在渲染层跑起来之后才 acquire 的。
            tryVerify(function () { return rig.view.leafItem(rootViewId) !== null }, 2000,
                      "根 leaf 的 delegate 没建出来")
            // **计数基线放在这儿**:上面那一格的 delegate 已经建过了,后面的数字只算
            // 本次测试自己的动作。
            host.built = 0
            host.destroyed = 0
            return rig
        }

        function test_theSameViewIdKeepsTheSameInstanceAcrossASplit() {
            const rig = makeRig("a")
            const before = rig.view.leafItem("a")

            rig.model.split("a", Qt.Horizontal, "b")
            tryVerify(function () { return rig.view.leafItem("b") !== null }, 2000,
                      "新开那一格的 delegate 没建出来")

            verify(before === rig.view.leafItem("a"), "分割重建了幸存那一格的 delegate")
            compare(host.built, 1, "只该多建一份(新开的那一格)")
            compare(host.destroyed, 0, "分割不该销毁任何 delegate")
        }

        function test_closingOnePaneKeepsTheOtherInstance() {
            const rig = makeRig("a")
            rig.model.split("a", Qt.Horizontal, "b")
            tryVerify(function () { return rig.view.leafItem("b") !== null }, 2000)
            const survivor = rig.view.leafItem("a")

            verify(rig.model.close("b"))
            tryVerify(function () { return rig.view.leafItem("b") === null }, 2000,
                      "关掉的那一格,delegate 还留在缓存里")

            verify(survivor === rig.view.leafItem("a"), "关掉相邻格重建了幸存者的 delegate")
        }

        function test_maximizeAndRestoreKeepTheInstance() {
            // 最大化换的是**顶层 node**,整棵渲染树重来一遍 —— 比分割动得还狠。
            const rig = makeRig("a")
            rig.model.split("a", Qt.Horizontal, "b")
            tryVerify(function () { return rig.view.leafItem("b") !== null }, 2000)
            const a = rig.view.leafItem("a")
            const b = rig.view.leafItem("b")

            rig.model.toggleMaximize(rig.model.findLeaf("a"))
            wait(100)
            verify(rig.model.maximizedLeaf !== null, "没进最大化")
            verify(a === rig.view.leafItem("a"), "最大化重建了那一格的 delegate")
            // 另一格这会儿没被渲染,但它**必须还活着** —— 它承载的会话不该被最大化
            // 别的格子这个动作杀掉。
            verify(b === rig.view.leafItem("b"), "最大化把没被渲染的那一格销毁了")

            rig.model.toggleMaximize(rig.model.findLeaf("a"))
            wait(100)
            verify(rig.model.maximizedLeaf === null, "没退出最大化")
            verify(a === rig.view.leafItem("a"))
            verify(b === rig.view.leafItem("b"))
        }

        function test_aRestoredLayoutReusesTheInstancesOfSurvivingViewIds() {
            // setRoot 那条路(restoreState / undo / redo)。模型已经承诺"两边都存在
            // 的 viewId 视作延续",渲染层要跟上。
            const rig = makeRig("a")
            rig.model.split("a", Qt.Horizontal, "b")
            tryVerify(function () { return rig.view.leafItem("b") !== null }, 2000)
            const a = rig.view.leafItem("a")
            const state = rig.model.saveState()

            rig.model.close("b")                 // 动一下再恢复回去
            tryVerify(function () { return rig.view.leafItem("b") === null }, 2000)

            verify(rig.model.restoreState(state))
            tryVerify(function () { return rig.view.leafItem("b") !== null }, 2000,
                      "恢复之后 b 那一格没回来")
            verify(a === rig.view.leafItem("a"), "恢复布局重建了一直都在的那一格")
        }

        function test_theDelegateIsMountedVisibleAndSized() {
            // 另一半:实例活着但没挂到屏幕上(或者宽高停在 0)是这套做法特有的坏法 ——
            // 上面那些身份断言照样绿,而用户看到的是一片空白。
            const rig = makeRig("a")
            const a = rig.view.leafItem("a")
            verify(a.visible, "delegate 没挂进可见的父链")
            verify(a.width > 0 && a.height > 0, "delegate 的宽高是 0")

            rig.model.split("a", Qt.Horizontal, "b")
            wait(100)
            verify(a.visible, "分割之后 delegate 留在池子里了")
            verify(a.width > 0 && a.height > 0, "分割之后 delegate 的宽高塌成了 0")
            verify(a.width < rig.width, "横向分割之后 delegate 还占着整幅宽")
        }

        function test_theInnerLayoutIsStillCorrectAfterASplit() {
            // 分割会把幸存那一格**停泊一趟再挂回来**（渲染层整棵重建）。它的内部布局
            // 必须跟着新宽度重排 —— 见 layoutLeafComp 的注释,这条是实打实撞出来的。
            const rig = makeRig("a", layoutLeafComp)
            const a = rig.view.leafItem("a")
            tryVerify(function () { return a.width > 0 && a.edge.width === 20 }, 2000,
                      "delegate 没排出来")
            compare(Math.round(a.edge.mapToItem(a, 0, 0).x), Math.round(a.width - 20),
                    "前置条件:那一颗该贴着右缘")

            rig.model.split("a", Qt.Horizontal, "b")
            wait(100)

            verify(a.width > 0 && a.width < rig.width, "分割之后宽度不对:" + a.width)
            compare(Math.round(a.edge.mapToItem(a, 0, 0).x), Math.round(a.width - 20),
                    "分割之后内部布局没跟着新宽度重排 —— 外框对了,里面是旧的")
        }

        function test_attachedPropertiesTrackTheModelNotTheRenderPass() {
            // attached 上的 node / isActive 由 QPaneTreeView 按模型推(v1.1 之前是
            // 绑到渲染层作用域的,渲染层一销毁绑定就悬空)。分割之后 node 对象会换新
            // —— 目标 leaf 原地升级成 Split,同名 viewId 挂到一个**新建**的 leaf 上。
            const rig = makeRig("a")
            const a = rig.view.leafItem("a")
            compare(a.vid, "a")
            verify(a.active, "只有一格时它就该是活动格")

            rig.model.split("a", Qt.Horizontal, "b")
            wait(100)
            compare(a.vid, "a", "分割之后 viewId 丢了")
            compare(a.PaneTreeView.node, rig.model.findLeaf("a"),
                    "分割之后 attached 上的 node 还指着旧节点")

            rig.model.activeLeaf = rig.model.findLeaf("b")
            wait(100)
            verify(!a.active, "切了活动格,a 还自称是活动的")
            verify(rig.view.leafItem("b").active)
        }
    }
}
