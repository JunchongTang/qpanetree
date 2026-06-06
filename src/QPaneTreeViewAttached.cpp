#include "QPaneTree/QPaneTreeViewAttached.h"
#include "QPaneTree/QPaneNode.h"
#include "QPaneTree/QPaneTreeModel.h"

#include <QQmlListReference>
#include <QQuickItem>
#include <QVariant>

namespace QPaneTree {

QPaneTreeViewAttached::QPaneTreeViewAttached(QObject* attachee)
    : QObject(attachee)
{
}

void QPaneTreeViewAttached::setNode(QPaneNode* n)
{
    if (m_node == n) {
        return;
    }
    m_node = n;
    emit changed();
}

void QPaneTreeViewAttached::setModel(QPaneTreeModel* m)
{
    if (m_model == m) {
        return;
    }
    m_model = m;
    emit changed();
}

void QPaneTreeViewAttached::setViewId(const QString& v)
{
    if (m_viewId == v) {
        return;
    }
    m_viewId = v;
    emit changed();
}

void QPaneTreeViewAttached::setNodeId(const QString& v)
{
    if (m_nodeId == v) {
        return;
    }
    m_nodeId = v;
    emit changed();
}

void QPaneTreeViewAttached::setOrientation(Qt::Orientation o)
{
    if (m_orientation == o) {
        return;
    }
    m_orientation = o;
    emit changed();
}

void QPaneTreeViewAttached::setRatio(qreal r)
{
    if (qFuzzyCompare(m_ratio, r)) {
        return;
    }
    m_ratio = r;
    emit changed();
}

void QPaneTreeViewAttached::setIsActive(bool a)
{
    if (m_isActive == a) {
        return;
    }
    m_isActive = a;
    emit changed();
}

// 拖完同步 ratio——从 splitContainer.contentChildren[0] 取实际尺寸算比例，
// 写回 model.updateRatio(nodeId, ratio)。
// 之所以让用户传 splitContainer：因为 splitDelegate 可以是任意容器（SplitView
// 是默认，但用户可换），attached 不能假设 attachee 就是那个容器。
void QPaneTreeViewAttached::commitRatio(QQuickItem* splitContainer)
{
    if (!splitContainer || !m_node || !m_model) {
        return;
    }

    // 取 SplitView 的 contentChildren 第一项的实际尺寸——SplitView 把数据 child
    // 放在这个属性里（不含内部 handle 项）。Item 子类没有此属性时回退到 children。
    QQmlListReference cc(splitContainer, "contentChildren");
    QQuickItem* firstChild = nullptr;
    if (cc.isValid() && cc.count() > 0) {
        firstChild = qobject_cast<QQuickItem*>(cc.at(0));
    } else {
        const auto kids = splitContainer->childItems();
        if (!kids.isEmpty()) {
            firstChild = kids.first();
        }
    }
    if (!firstChild) {
        return;
    }

    const bool horiz = m_orientation == Qt::Horizontal;
    const qreal total = horiz ? splitContainer->width() : splitContainer->height();
    if (total <= 0) {
        return;
    }
    const qreal firstSize = horiz ? firstChild->width() : firstChild->height();
    m_model->updateRatio(m_nodeId, firstSize / total);
}

} // namespace QPaneTree
