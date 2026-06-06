#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>
#include <Qt>

namespace QPaneTree {

class QPaneNode;
class QPaneTreeModel;
class QPaneTreeView;

// PaneTreeView 的 attached property —— 注入给每个 delegate item，提供该层节点
// 的标识（viewId / nodeId / node）+ split delegate 用到的 orientation / ratio /
// commitRatio()。
//
// 库内部 QML 实现层（PaneTreeViewImpl.qml）在为每一层 leaf / split 实例化
// delegate 时，通过 `item.PaneTreeView.node = ...` 这样把字段写到 attached 上。
// 用户的 delegate 读 `PaneTreeView.viewId` 等就拿到了。
//
// 写访问限制在库内部 QML 使用——用户应只读这些字段（除了 commitRatio() 调用）。
class QPaneTreeViewAttached : public QObject
{
    Q_OBJECT
    // QML_ANONYMOUS：让 QML 类型系统把这个类作为合法的 attached type 注册。
    // 没有这个，`item.PaneTreeView.view = x` 看起来生效但 QML 实际上每次访问
    // 都返回一个新实例——写入立刻被丢弃，再读就是 undefined。
    QML_ANONYMOUS

    Q_PROPERTY(QPaneTree::QPaneTreeView* view READ view WRITE setView NOTIFY changed)
    Q_PROPERTY(QPaneTree::QPaneNode* node READ node WRITE setNode NOTIFY changed)
    Q_PROPERTY(QPaneTree::QPaneTreeModel* model READ model WRITE setModel NOTIFY changed)
    Q_PROPERTY(QString viewId READ viewId WRITE setViewId NOTIFY changed)
    Q_PROPERTY(QString nodeId READ nodeId WRITE setNodeId NOTIFY changed)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY changed)
    Q_PROPERTY(qreal ratio READ ratio WRITE setRatio NOTIFY changed)
    // 仅 leaf delegate 上有效——是否是 PaneTreeModel.activeLeaf
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive NOTIFY changed)

public:
    explicit QPaneTreeViewAttached(QObject* attachee);

    QPaneTreeView* view() const { return m_view; }
    QPaneNode* node() const { return m_node; }
    QPaneTreeModel* model() const { return m_model; }
    QString viewId() const { return m_viewId; }
    QString nodeId() const { return m_nodeId; }
    Qt::Orientation orientation() const { return m_orientation; }
    qreal ratio() const { return m_ratio; }
    bool isActive() const { return m_isActive; }

    void setView(QPaneTreeView* v);
    void setNode(QPaneNode* n);
    void setModel(QPaneTreeModel* m);
    void setViewId(const QString& v);
    void setNodeId(const QString& v);
    void setOrientation(Qt::Orientation o);
    void setRatio(qreal r);
    void setIsActive(bool a);

    // 用户在 splitDelegate.onResizingChanged 里调：
    //   onResizingChanged: if (!resizing) PaneTreeView.commitRatio(this)
    // 参数 splitContainer 一般传 SplitView 实例（this）；从它的
    // contentChildren[0] 取实际尺寸算 ratio 写回 model。
    Q_INVOKABLE void commitRatio(QQuickItem* splitContainer);

signals:
    void changed();

private:
    QPointer<QPaneTreeView> m_view;
    QPointer<QPaneNode> m_node;
    QPointer<QPaneTreeModel> m_model;
    QString m_viewId;
    QString m_nodeId;
    Qt::Orientation m_orientation = Qt::Horizontal;
    qreal m_ratio = 0.5;
    bool m_isActive = false;
};

} // namespace QPaneTree
