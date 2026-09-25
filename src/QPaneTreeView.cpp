#include "QPaneTree/QPaneTreeView.h"
#include "QPaneTree/QPaneNode.h"
#include "QPaneTree/QPaneTreeLog.h"
#include "QPaneTree/QPaneTreeModel.h"
#include "QPaneTree/QPaneTreeViewAttached.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QtQml/qqml.h>

namespace QPaneTree {

QPaneTreeView::QPaneTreeView(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, false);
}

QPaneTreeView::~QPaneTreeView() = default;

void QPaneTreeView::setModel(QPaneTreeModel* m)
{
    if (m_model == m) {
        return;
    }
    for (const QMetaObject::Connection& c : std::as_const(m_modelConnections)) {
        disconnect(c);
    }
    m_modelConnections.clear();
    // 换模型 = 旧模型那些 viewId 全部作废。留着的话新模型里一个同名 viewId 会拿到
    // 旧模型那一格的内容——一个不报错的串台。
    discardAllLeaves();
    m_model = m;
    connectModel();
    emit modelChanged();
    if (isComponentComplete()) {
        rebuildImpl();
    }
}

void QPaneTreeView::connectModel()
{
    if (!m_model) {
        return;
    }
    // 某一格真的关掉了 → 它的实例该销毁。**只认 viewClosed**:structureChanged 里
    // 一个 viewId 只是换了位置，不该把内容扔掉（这正是本类要修的那件事）。
    m_modelConnections << connect(m_model, &QPaneTreeModel::viewClosed,
                                  this, [this](const QString& id) { discardLeaf(id); });
    // node 对象会在分割时换新（原地升级成 Split，同名 viewId 挂到一个新建的 leaf
    // 节点上），所以结构一变就把 attached 上的 node/nodeId 重新指一次。
    m_modelConnections << connect(m_model, &QPaneTreeModel::structureChanged,
                                  this, [this] { refreshAllAttached(); });
    m_modelConnections << connect(m_model, &QPaneTreeModel::activeLeafChanged,
                                  this, [this] { refreshAllAttached(); });
}

void QPaneTreeView::setLeafDelegate(QQmlComponent* c)
{
    if (m_leafDelegate == c) {
        return;
    }
    m_leafDelegate = c;
    // 换了 delegate 就没法复用旧实例了——它们是另一个 Component 的产物。
    discardAllLeaves();
    emit leafDelegateChanged();
    if (m_impl) {
        m_impl->setProperty("leafDelegate", QVariant::fromValue(c));
    }
}

void QPaneTreeView::setSplitDelegate(QQmlComponent* c)
{
    if (m_splitDelegate == c) {
        return;
    }
    m_splitDelegate = c;
    emit splitDelegateChanged();
    if (m_impl) {
        m_impl->setProperty("splitDelegate", QVariant::fromValue(c));
    }
}

void QPaneTreeView::setHandleDelegate(QQmlComponent* c)
{
    if (m_handleDelegate == c) {
        return;
    }
    m_handleDelegate = c;
    emit handleDelegateChanged();
    if (m_impl) {
        m_impl->setProperty("handleDelegate", QVariant::fromValue(c));
    }
}

QPaneTreeViewAttached* QPaneTreeView::qmlAttachedProperties(QObject* attachee)
{
    return new QPaneTreeViewAttached(attachee);
}

// ── leaf delegate 实例缓存 ───────────────────────────────────────────────
//
// 池子是实例的 **QObject 归属**。渲染层的插槽只拿视觉父子关系
// （QQuickItem::setParentItem() 不动 QObject 的 parent），所以插槽被销毁时
// 实例不会跟着死——~QQuickItem() 对视觉子项只是把它们的 parentItem 置空。

QQuickItem* QPaneTreeView::leafPool()
{
    if (!m_leafPool) {
        m_leafPool = new QQuickItem(this);
        m_leafPool->setObjectName(QStringLiteral("qpanetreeLeafPool"));
        // 停在池子里的实例不该被画出来，也就顺带不吃鼠标 / 悬停。
        m_leafPool->setVisible(false);
    }
    return m_leafPool;
}

QQuickItem* QPaneTreeView::leafItem(const QString& viewId) const
{
    return m_leaves.value(viewId, nullptr);
}

QQuickItem* QPaneTreeView::acquireLeaf(const QString& viewId, QQuickItem* slot)
{
    if (viewId.isEmpty()) {
        return nullptr;
    }
    QQuickItem* item = m_leaves.value(viewId, nullptr);
    if (!item) {
        if (!m_leafDelegate) {
            qCWarning(lcPaneTree) << "acquireLeaf: no leafDelegate set";
            return nullptr;
        }
        // 用 Component 自己的创建上下文——跟此前 `Loader { sourceComponent: ... }`
        // 的解析环境保持一致（宿主的 delegate 里会引用它自己文件里的 id）。
        QObject* obj = m_leafDelegate->create(m_leafDelegate->creationContext());
        item = qobject_cast<QQuickItem*>(obj);
        if (!item) {
            qCWarning(lcPaneTree) << "acquireLeaf: delegate root is not a QQuickItem for"
                                  << viewId << m_leafDelegate->errorString();
            delete obj;
            return nullptr;
        }
        // QObject 归属给池子（这才是"不跟着渲染层死"的关键），但**视觉父项从一开始
        // 就是插槽**。
        //
        // 为什么不先挂池子再搬过去：池子是 0×0 而且不可见，而 QQuickLayout 在不可见
        // 时**不做布局**。在池子里完成构建的话，delegate 里的 ColumnLayout / RowLayout
        // 会带着一份按 0 宽算出来的布局出生（实测拿到过 width = -16 的 RowLayout），
        // 搬进插槽之后尺寸对了、内部却没有重新排过 —— 表现是按钮画在错的位置上，
        // 点击(靠命中测试)照常，而悬停(靠场景坐标)落到隔壁那颗按钮上。
        item->setParent(leafPool());
        m_leaves.insert(viewId, item);
        // 先把身份写上再挂出去——宿主的 delegate 在 parentItem 变化时就可能读
        // PaneTreeView.viewId（比如去查自己那一格的状态）。
        refreshAttached(viewId, item);
        item->setParentItem(slot ? slot : leafPool());
        return item;
    }
    refreshAttached(viewId, item);
    if (slot && item->parentItem() != slot) {
        item->setParentItem(slot);
    }
    return item;
}

void QPaneTreeView::releaseLeaf(const QString& viewId, QQuickItem* slot)
{
    QQuickItem* item = m_leaves.value(viewId, nullptr);
    if (!item) {
        return;
    }
    // **身份比对**:只有还挂在这个插槽上时才摘。见头文件里的说明。
    if (slot && item->parentItem() != slot) {
        return;
    }
    item->setParentItem(leafPool());
}

void QPaneTreeView::discardLeaf(const QString& viewId)
{
    QQuickItem* item = m_leaves.take(viewId);
    if (!item) {
        return;
    }
    // 先摘出视觉树再删——正在被渲染的那一格直接 delete 会让插槽这一帧里还引用它。
    item->setParentItem(nullptr);
    item->deleteLater();
}

void QPaneTreeView::discardAllLeaves()
{
    const QStringList ids = m_leaves.keys();
    for (const QString& id : ids) {
        discardLeaf(id);
    }
}

void QPaneTreeView::refreshAttached(const QString& viewId, QQuickItem* item)
{
    if (!item) {
        return;
    }
    auto* att = qobject_cast<QPaneTreeViewAttached*>(
        qmlAttachedPropertiesObject<QPaneTreeView>(item, /*create=*/true));
    if (!att) {
        return;
    }
    QPaneNode* node = m_model ? m_model->findLeaf(viewId) : nullptr;
    att->setView(this);
    att->setModel(m_model);
    att->setViewId(viewId);
    att->setNode(node);
    att->setNodeId(node ? node->nodeId() : QString());
    att->setIsActive(m_model && node && m_model->activeLeaf() == node);
}

void QPaneTreeView::refreshAllAttached()
{
    for (auto it = m_leaves.cbegin(); it != m_leaves.cend(); ++it) {
        refreshAttached(it.key(), it.value());
    }
}

void QPaneTreeView::componentComplete()
{
    QQuickItem::componentComplete();
    rebuildImpl();
}

void QPaneTreeView::geometryChange(const QRectF& newGeo, const QRectF& oldGeo)
{
    QQuickItem::geometryChange(newGeo, oldGeo);
    resizeImpl();
}

// 创建 PaneTreeViewImpl.qml 实例当 child，把所有 delegate / model 注入。
void QPaneTreeView::rebuildImpl()
{
    if (m_impl) {
        delete m_impl;
        m_impl = nullptr;
    }
    QQmlEngine* engine = qmlEngine(this);
    if (!engine) {
        return;
    }

    // QML URI 是 PaneTree（C++ 加 Q 前缀，但 QML 类型/资源路径不带 Q）
    QQmlComponent comp(engine,
                       QUrl(QStringLiteral("qrc:/qt/qml/PaneTree/qml/PaneTreeViewImpl.qml")),
                       this);
    if (comp.isError()) {
        qCWarning(lcPaneTree) << "PaneTreeView: failed to load impl:" << comp.errorString();
        return;
    }
    QVariantMap initial;
    // 用静态类型 fromValue —— QVariant 标记成 QQmlComponent* 而不是 QObject*，
    // QML 才能把它当成 `property Component` 的合法赋值（QObject* 不自动 cast）。
    initial[QStringLiteral("view")] = QVariant::fromValue<QPaneTreeView*>(this);
    initial[QStringLiteral("model")] = QVariant::fromValue<QPaneTreeModel*>(m_model);
    initial[QStringLiteral("leafDelegate")] = QVariant::fromValue(m_leafDelegate);
    initial[QStringLiteral("splitDelegate")] = QVariant::fromValue(m_splitDelegate);
    initial[QStringLiteral("handleDelegate")] = QVariant::fromValue(m_handleDelegate);

    QObject* obj = comp.createWithInitialProperties(initial, qmlContext(this));
    m_impl = qobject_cast<QQuickItem*>(obj);
    if (!m_impl) {
        qCWarning(lcPaneTree) << "PaneTreeView: impl root is not a QQuickItem";
        delete obj;
        return;
    }
    m_impl->setParent(this);
    m_impl->setParentItem(this);
    resizeImpl();
}

void QPaneTreeView::resizeImpl()
{
    if (!m_impl) {
        return;
    }
    m_impl->setWidth(width());
    m_impl->setHeight(height());
}

} // namespace QPaneTree
