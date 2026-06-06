#include "QPaneTree/QPaneTreeView.h"
#include "QPaneTree/QPaneTreeLog.h"
#include "QPaneTree/QPaneTreeModel.h"
#include "QPaneTree/QPaneTreeViewAttached.h"

#include <QQmlContext>
#include <QQmlEngine>

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
    m_model = m;
    emit modelChanged();
    if (isComponentComplete()) {
        rebuildImpl();
    }
}

void QPaneTreeView::setLeafDelegate(QQmlComponent* c)
{
    if (m_leafDelegate == c) {
        return;
    }
    m_leafDelegate = c;
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
