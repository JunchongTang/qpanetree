#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <Qt>

namespace QPaneTree {

// QPaneNode —— 树的单个节点。两种 type：
//   Leaf  : 没有子节点，持 viewId（由业务层决定渲染什么）
//   Split : 有 first / second 两个子节点 + orientation + ratio
//
// 所有变更走 QPaneTreeModel。节点本身不直接修改树（保持只读视图给 QML）。
class QPaneNode : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PaneNode)
    QML_UNCREATABLE("PaneNode is managed by PaneTreeModel")

    Q_PROPERTY(NodeType nodeType READ nodeType NOTIFY nodeTypeChanged)
    Q_PROPERTY(QString nodeId READ nodeId CONSTANT)
    Q_PROPERTY(QString viewId READ viewId NOTIFY viewIdChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation NOTIFY orientationChanged)
    Q_PROPERTY(qreal ratio READ ratio NOTIFY ratioChanged)
    Q_PROPERTY(QPaneNode* first READ first NOTIFY firstChanged)
    Q_PROPERTY(QPaneNode* second READ second NOTIFY secondChanged)
    Q_PROPERTY(bool isLeaf READ isLeaf NOTIFY nodeTypeChanged)
    Q_PROPERTY(bool isSplit READ isSplit NOTIFY nodeTypeChanged)
    // 业务可读写的自定义数据。任意 QVariant —— 通常是 QVariantMap 装多个字段。
    // 持久化时自动 round-trip。
    Q_PROPERTY(QVariant data READ data WRITE setData NOTIFY dataChanged)

public:
    enum class NodeType { Leaf, Split };
    Q_ENUM(NodeType)

    static QPaneNode* makeLeaf(const QString& viewId, QObject* parent = nullptr);
    static QPaneNode* makeSplit(Qt::Orientation orientation,
                                qreal ratio,
                                QPaneNode* first,
                                QPaneNode* second,
                                QObject* parent = nullptr);

    ~QPaneNode() override;

    NodeType nodeType() const { return m_nodeType; }
    QString nodeId() const { return m_nodeId; }
    bool isLeaf() const { return m_nodeType == NodeType::Leaf; }
    bool isSplit() const { return m_nodeType == NodeType::Split; }
    QString viewId() const { return m_viewId; }
    Qt::Orientation orientation() const { return m_orientation; }
    qreal ratio() const { return m_ratio; }
    QPaneNode* first() const { return m_first; }
    QPaneNode* second() const { return m_second; }
    QVariant data() const { return m_data; }

    // 内部写入——只该被 QPaneTreeModel 调
    void setRatio(qreal ratio);
    void setFirst(QPaneNode* node);
    void setSecond(QPaneNode* node);
    void setOrientation(Qt::Orientation o);
    void setViewId(const QString& id);
    void setData(const QVariant& data);

    // Leaf ↔ Split 原地切换——保持 QML 对象指针稳定，binding 不丢
    void promoteToSplit(Qt::Orientation orientation,
                        qreal ratio,
                        QPaneNode* first,
                        QPaneNode* second);
    void demoteToLeaf(const QString& viewId, const QVariant& data = {});

    // 序列化
    QVariantMap toVariantMap() const;
    static QPaneNode* fromVariantMap(const QVariantMap& map, QObject* parent = nullptr);

signals:
    void nodeTypeChanged();
    void viewIdChanged();
    void orientationChanged();
    void ratioChanged();
    void firstChanged();
    void secondChanged();
    void dataChanged();

private:
    explicit QPaneNode(QObject* parent = nullptr);

    NodeType m_nodeType = NodeType::Leaf;
    QString m_nodeId;
    QString m_viewId;
    Qt::Orientation m_orientation = Qt::Horizontal;
    qreal m_ratio = 0.5;
    QPaneNode* m_first = nullptr;
    QPaneNode* m_second = nullptr;
    QVariant m_data;
};

} // namespace QPaneTree
