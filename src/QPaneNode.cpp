#include "QPaneTree/QPaneNode.h"

#include <QUuid>

namespace QPaneTree {

QPaneNode* QPaneNode::makeLeaf(const QString& viewId, QObject* parent)
{
    auto* node = new QPaneNode(parent);
    node->m_nodeType = NodeType::Leaf;
    node->m_viewId = viewId;
    return node;
}

QPaneNode* QPaneNode::makeSplit(Qt::Orientation orientation,
                                qreal ratio,
                                QPaneNode* first,
                                QPaneNode* second,
                                QObject* parent)
{
    auto* node = new QPaneNode(parent);
    node->m_nodeType = NodeType::Split;
    node->m_orientation = orientation;
    node->m_ratio = qBound(0.1, ratio, 0.9);
    node->m_first = first;
    node->m_second = second;
    if (first) {
        first->setParent(node);
    }
    if (second) {
        second->setParent(node);
    }
    return node;
}

QPaneNode::QPaneNode(QObject* parent)
    : QObject(parent)
    , m_nodeId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

QPaneNode::~QPaneNode() = default;

void QPaneNode::setRatio(qreal ratio)
{
    const qreal clamped = qBound(0.1, ratio, 0.9);
    if (qFuzzyCompare(m_ratio, clamped)) {
        return;
    }
    m_ratio = clamped;
    emit ratioChanged();
}

void QPaneNode::setFirst(QPaneNode* node)
{
    if (m_first == node) {
        return;
    }
    m_first = node;
    if (node) {
        node->setParent(this);
    }
    emit firstChanged();
}

void QPaneNode::setSecond(QPaneNode* node)
{
    if (m_second == node) {
        return;
    }
    m_second = node;
    if (node) {
        node->setParent(this);
    }
    emit secondChanged();
}

void QPaneNode::setOrientation(Qt::Orientation o)
{
    if (m_orientation == o) {
        return;
    }
    m_orientation = o;
    emit orientationChanged();
}

void QPaneNode::setViewId(const QString& id)
{
    if (m_viewId == id) {
        return;
    }
    m_viewId = id;
    emit viewIdChanged();
}

// Leaf → Split 原地升级——保持 QML 对象指针稳定，避免 binding 丢
void QPaneNode::promoteToSplit(Qt::Orientation orientation,
                               qreal ratio,
                               QPaneNode* first,
                               QPaneNode* second)
{
    m_viewId = {};
    m_orientation = orientation;
    m_ratio = qBound(0.1, ratio, 0.9);
    m_first = first;
    m_second = second;
    if (first) {
        first->setParent(this);
    }
    if (second) {
        second->setParent(this);
    }

    const NodeType oldType = m_nodeType;
    m_nodeType = NodeType::Split;
    if (oldType != m_nodeType) {
        emit nodeTypeChanged();
    }
    emit firstChanged();
    emit secondChanged();
    emit orientationChanged();
    emit ratioChanged();
}

// Split → Leaf 原地降级——丢 first/second（不 delete，由调用方接管），切回 Leaf
void QPaneNode::demoteToLeaf(const QString& viewId)
{
    m_first = nullptr;
    m_second = nullptr;
    m_viewId = viewId;

    const NodeType oldType = m_nodeType;
    m_nodeType = NodeType::Leaf;
    if (oldType != m_nodeType) {
        emit nodeTypeChanged();
    }
    emit firstChanged();
    emit secondChanged();
    emit viewIdChanged();
}

QVariantMap QPaneNode::toVariantMap() const
{
    QVariantMap map;
    if (m_nodeType == NodeType::Leaf) {
        map[QStringLiteral("type")] = QStringLiteral("leaf");
        map[QStringLiteral("viewId")] = m_viewId;
    } else {
        map[QStringLiteral("type")] = QStringLiteral("split");
        map[QStringLiteral("orientation")] = static_cast<int>(m_orientation);
        map[QStringLiteral("ratio")] = m_ratio;
        if (m_first) {
            map[QStringLiteral("first")] = m_first->toVariantMap();
        }
        if (m_second) {
            map[QStringLiteral("second")] = m_second->toVariantMap();
        }
    }
    return map;
}

QPaneNode* QPaneNode::fromVariantMap(const QVariantMap& map, QObject* parent)
{
    const QString type = map.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("leaf")) {
        return makeLeaf(map.value(QStringLiteral("viewId")).toString(), parent);
    }
    const auto orientation = static_cast<Qt::Orientation>(
        map.value(QStringLiteral("orientation"), Qt::Horizontal).toInt());
    const qreal ratio = map.value(QStringLiteral("ratio"), 0.5).toReal();
    auto* first = fromVariantMap(map.value(QStringLiteral("first")).toMap());
    auto* second = fromVariantMap(map.value(QStringLiteral("second")).toMap());
    return makeSplit(orientation, ratio, first, second, parent);
}

} // namespace QPaneTree
