#pragma once

#include <QHash>
#include <QQuickItem>
#include <QQmlComponent>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "QPaneTreeViewAttached.h"

namespace QPaneTree {

class QPaneTreeModel;

// QPaneTreeView —— 递归分割面板视图。C++ 壳子负责注册类型 + 持有属性，递归
// 渲染由内嵌 QML 实现（PaneTreeViewImpl.qml）完成。
//
// 用法（QML）：
//   PaneTreeView {
//       model: paneTreeModel
//       leafDelegate: Component { MyEditor { viewId: PaneTreeView.viewId } }
//       handleDelegate: Component { Rectangle { ... } }  // 可选
//       splitDelegate: Component { SplitView { ... } }   // 可选
//   }
//
// ── leaf delegate 的生命周期:**一个 viewId 一份实例，活到这个 viewId 关掉为止**
//
// 这是本库对宿主的承诺，也是 v1.1 修掉的一个真缺陷。此前 leaf delegate 长在递归
// 渲染层里，而递归层在**树结构一变就整棵重建**（分割、关掉相邻格、进出最大化都
// 算）——于是一次分割会把幸存那一格的 delegate 销毁重建。对一个编辑器/终端类宿主，
// 这意味着"分个屏，正在跑的活就没了"。
//
// 讽刺的是模型层早就把 viewId 当稳定身份在承诺:`setRoot()` 对两边都存在的 viewId
// **不发** viewClosed+viewCreated 配对，注释写的就是"避免宿主侧的 registry 把
// model 销毁重建"。渲染层此前跟这条承诺相矛盾。
//
// 现在 delegate 实例由本类按 viewId 缓存（见 acquireLeaf / releaseLeaf），停在一个
// 隐藏的池子里，由当前渲染出来的插槽 reparent 过去。宿主**不需要**自己再建一套池子。
class QPaneTreeView : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PaneTreeView)
    QML_ATTACHED(QPaneTreeViewAttached)

    Q_PROPERTY(QPaneTree::QPaneTreeModel* model
               READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* leafDelegate
               READ leafDelegate WRITE setLeafDelegate NOTIFY leafDelegateChanged)
    Q_PROPERTY(QQmlComponent* splitDelegate
               READ splitDelegate WRITE setSplitDelegate NOTIFY splitDelegateChanged)
    Q_PROPERTY(QQmlComponent* handleDelegate
               READ handleDelegate WRITE setHandleDelegate NOTIFY handleDelegateChanged)

public:
    explicit QPaneTreeView(QQuickItem* parent = nullptr);
    ~QPaneTreeView() override;

    QPaneTreeModel* model() const { return m_model; }
    QQmlComponent* leafDelegate() const { return m_leafDelegate; }
    QQmlComponent* splitDelegate() const { return m_splitDelegate; }
    QQmlComponent* handleDelegate() const { return m_handleDelegate; }

    void setModel(QPaneTreeModel* m);
    void setLeafDelegate(QQmlComponent* c);
    void setSplitDelegate(QQmlComponent* c);
    void setHandleDelegate(QQmlComponent* c);

    // QML_ATTACHED 钩子——给某个 item 返回它的 attached 实例
    static QPaneTreeViewAttached* qmlAttachedProperties(QObject* attachee);

    /*!
        \brief 取 \a viewId 的 leaf delegate 实例，挂到 \a slot 上。

        缓存里有就直接 reparent 过去，没有就用 leafDelegate 建一份。**同一个 viewId
        永远是同一个实例**——这正是本类存在的理由。

        由库内部的递归渲染层调用（PaneTreeViewImpl.qml）。宿主一般用不到，但它是
        public 的:宿主想按 viewId 拿到那一格的 item 时，用 leafItem() 更合适。
    */
    Q_INVOKABLE QQuickItem* acquireLeaf(const QString& viewId, QQuickItem* slot);

    /*!
        \brief 把 \a viewId 的实例从 \a slot 上摘下来，停回池子。

        **带着 slot 比对:只有还挂在它身上时才摘。** 渲染层重建时新旧两个插槽会同时
        存在一瞬而顺序不保证——无条件摘会把已经挂到新插槽上的实例抢回池子，表现是
        那一格突然空白。
    */
    Q_INVOKABLE void releaseLeaf(const QString& viewId, QQuickItem* slot);

    //! \brief \a viewId 那一格的 delegate 实例;没有(或已关闭)返回 nullptr。
    //! 宿主想直接够到某一格的内容时用这个，不必自己维护 registry。
    Q_INVOKABLE QQuickItem* leafItem(const QString& viewId) const;

signals:
    void modelChanged();
    void leafDelegateChanged();
    void splitDelegateChanged();
    void handleDelegateChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF& newGeo, const QRectF& oldGeo) override;

private:
    void rebuildImpl();
    void resizeImpl();

    // leaf 实例缓存
    QQuickItem* leafPool();
    void discardLeaf(const QString& viewId);
    void discardAllLeaves();
    //! 把 attached 上的 node / nodeId / isActive 按模型现状刷一遍。
    //! **实例比渲染层活得长，所以这些字段不能绑到渲染层的作用域上**——绑过去的话
    //! 渲染层一销毁绑定就悬空。改由本类在模型变化时推。
    void refreshAttached(const QString& viewId, QQuickItem* item);
    void refreshAllAttached();
    void connectModel();

    QPointer<QPaneTreeModel> m_model;
    QQmlComponent* m_leafDelegate = nullptr;
    QQmlComponent* m_splitDelegate = nullptr;
    QQmlComponent* m_handleDelegate = nullptr;

    // 内嵌 QML 实现的根 item（PaneTreeViewImpl.qml 实例化结果）
    QQuickItem* m_impl = nullptr;

    // viewId → leaf delegate 实例。池子是它们的 QObject 归属，所以渲染层销毁插槽
    // 时它们不会跟着死（QQuickItem::setParentItem() 只改视觉父子关系）。
    QHash<QString, QQuickItem*> m_leaves;
    QQuickItem* m_leafPool = nullptr;
    QList<QMetaObject::Connection> m_modelConnections;
};

} // namespace QPaneTree
