#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QList>
#include <QtQml/qqmlregistration.h>

#include "QPaneNode.h"

namespace QPaneTree {

// QPaneTreeModel —— 暴露给 QML 的唯一入口。
// 职责：
//   1. 持有根节点（QML property "root"）
//   2. 提供所有树操作（Q_INVOKABLE）
//   3. 树结构变化时 emit structureChanged 让 View 刷新
//   4. 内部维护 undo/redo 栈
//
// QML 用法：
//   PaneTreeModel { id: paneTree }
//   PaneTreeView { model: paneTree; ... }
class QPaneTreeModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PaneTreeModel)

    Q_PROPERTY(QPaneTree::QPaneNode* root READ root NOTIFY rootChanged)
    Q_PROPERTY(int leafCount READ leafCount NOTIFY structureChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)

public:
    explicit QPaneTreeModel(QObject* parent = nullptr);
    ~QPaneTreeModel() override;

    QPaneNode* root() const { return m_root; }
    int leafCount() const;
    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }

    // 把 targetViewId 所在的 Leaf 一分为二。返回新 leaf 的 viewId。
    // orientation : Qt.Horizontal / Qt.Vertical
    // newViewId   : 新面板视图 ID；为空则自动生成
    // after       : true = 新面板在右/下，false = 在左/上
    Q_INVOKABLE QString split(const QString& targetViewId,
                              Qt::Orientation orientation,
                              const QString& newViewId = {},
                              bool after = true);

    // 关闭 targetViewId 所在 Leaf。允许关到 0；宿主用 leafCount 判断是否限制。
    Q_INVOKABLE bool close(const QString& targetViewId);

    // 把 sourceViewId 移到 targetViewId 处。position = "before"/"after"。
    Q_INVOKABLE bool move(const QString& sourceViewId,
                          const QString& targetViewId,
                          const QString& position = QStringLiteral("after"));

    // SplitView 拖完同步比例。nodeId 是 SplitNode 的 nodeId。
    Q_INVOKABLE void updateRatio(const QString& nodeId, qreal ratio);

    // 重置为单 Leaf —— 清空 undo 栈、彻底重来。
    Q_INVOKABLE void reset(const QString& viewId = QStringLiteral("default"));

    // 仅在 root 为空时创建一个根 leaf。保留 undo 栈。返回 true 表示创建了。
    // 用于"关到空后再开新面板"的场景——配合 leafCount/root 判断使用。
    Q_INVOKABLE bool createRootLeaf(const QString& viewId);

    // 切某个 Split 节点的方向
    Q_INVOKABLE bool flipOrientation(const QString& nodeId);

    // 查询
    Q_INVOKABLE QPaneTree::QPaneNode* findLeaf(const QString& viewId) const;
    Q_INVOKABLE QPaneTree::QPaneNode* findNode(const QString& nodeId) const;
    Q_INVOKABLE QStringList allViewIds() const;

    // Undo / Redo
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // 序列化
    Q_INVOKABLE QVariantMap saveState() const;
    Q_INVOKABLE bool restoreState(const QVariantMap& state);

    // 调试：打印整棵树到 lcPaneTree 日志范畴（默认级别 warning，需要显式打开
    //   QT_LOGGING_RULES="panetree.debug=true" 才能看到）。
    // path 用 0=first / 1=second 表示，root 是 ""。
    Q_INVOKABLE void dump() const;

signals:
    void rootChanged();
    void structureChanged();
    void ratioChanged(const QString& nodeId, qreal ratio);
    void historyChanged();
    void viewClosed(const QString& viewId);
    void viewCreated(const QString& viewId);

private:
    QPaneNode* findLeafWithParent(const QString& viewId,
                                  QPaneNode*& outParent,
                                  bool& outIsFirst) const;
    QPaneNode* findNodeById(QPaneNode* root, const QString& nodeId) const;
    QPaneNode* findLeafById(QPaneNode* root, const QString& viewId) const;
    void collectViewIds(QPaneNode* node, QStringList& out) const;
    int countLeaves(QPaneNode* node) const;

    void pushUndoState();
    void setRoot(QPaneNode* newRoot);
    bool restoreStateInternal(const QVariantMap& state);
    void demoteNodeToLeaf(QPaneNode* node, const QString& viewId);

    QPaneNode* m_root = nullptr;
    QList<QVariantMap> m_undoStack;
    QList<QVariantMap> m_redoStack;
    static constexpr int kMaxHistory = 50;
};

} // namespace QPaneTree
