#include "QPaneTree/QPaneTreeModel.h"
#include "QPaneTree/QPaneTreeLog.h"

#include <QUuid>
#include <functional>

namespace QPaneTree {

QPaneTreeModel::QPaneTreeModel(QObject* parent)
    : QObject(parent)
{
    reset();
}

QPaneTreeModel::~QPaneTreeModel() = default;

int QPaneTreeModel::leafCount() const
{
    return countLeaves(m_root);
}

QString QPaneTreeModel::split(const QString& targetViewId,
                              Qt::Orientation orientation,
                              const QString& newViewId,
                              bool after)
{
    QPaneNode* targetLeaf = findLeafById(m_root, targetViewId);
    if (!targetLeaf) {
        qCWarning(lcPaneTree) << "split: viewId not found:" << targetViewId;
        return {};
    }
    pushUndoState();

    const QString actualNewId = newViewId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : newViewId;

    auto* originalLeaf = QPaneNode::makeLeaf(targetViewId);
    auto* newLeaf = QPaneNode::makeLeaf(actualNewId);

    // target Leaf 原地升级为 Split——QML 对象指针保持稳定，binding 不丢
    targetLeaf->promoteToSplit(orientation,
                               0.5,
                               after ? originalLeaf : newLeaf,
                               after ? newLeaf : originalLeaf);

    emit structureChanged();
    emit viewCreated(actualNewId);
    return actualNewId;
}

// 关闭 targetViewId。库不强制"留一个 leaf"——宿主自己根据 leafCount 决定
// 关闭按钮是否禁用。允许关到 0：m_root = nullptr，View 渲染空白，宿主可
// 显示自己的 empty-state（"open a file..."）。
bool QPaneTreeModel::close(const QString& targetViewId)
{
    QPaneNode* parent = nullptr;
    bool isFirst = false;
    QPaneNode* target = findLeafWithParent(targetViewId, parent, isFirst);
    if (!target) {
        qCWarning(lcPaneTree) << "close: viewId not found:" << targetViewId;
        return false;
    }
    pushUndoState();

    if (parent == nullptr) {
        // target 就是 root（树只有一个 leaf）。清空模型。
        m_root = nullptr;
        target->setParent(nullptr);
        target->deleteLater();
        emit rootChanged();
        emit structureChanged();
        emit viewClosed(targetViewId);
        return true;
    }

    QPaneNode* sibling = isFirst ? parent->second() : parent->first();

    if (parent == m_root) {
        // parent 是根 split——sibling 直接升新根
        sibling->setParent(this);
        m_root = sibling;
        parent->setParent(nullptr);
        parent->deleteLater();
        emit rootChanged();
        emit structureChanged();
    } else {
        // parent 不是根——把 sibling 的内容搬到 parent，删除 sibling
        if (sibling->isLeaf()) {
            demoteNodeToLeaf(parent, sibling->viewId());
        } else {
            parent->promoteToSplit(sibling->orientation(),
                                   sibling->ratio(),
                                   sibling->first(),
                                   sibling->second());
        }
        sibling->setParent(nullptr);
        sibling->deleteLater();
        emit structureChanged();
    }

    emit viewClosed(targetViewId);
    return true;
}

bool QPaneTreeModel::move(const QString& sourceViewId,
                          const QString& targetViewId,
                          const QString& position)
{
    if (sourceViewId == targetViewId) {
        return false;
    }
    pushUndoState();

    QPaneNode* sourceLeaf = findLeafById(m_root, sourceViewId);
    QPaneNode* targetLeaf = findLeafById(m_root, targetViewId);
    if (!sourceLeaf || !targetLeaf) {
        return false;
    }

    close(sourceViewId);
    const bool after = (position != QLatin1String("before"));
    split(targetViewId, Qt::Horizontal, sourceViewId, after);
    emit structureChanged();
    return true;
}

void QPaneTreeModel::updateRatio(const QString& nodeId, qreal ratio)
{
    QPaneNode* node = findNodeById(m_root, nodeId);
    if (!node || node->isLeaf()) {
        return;
    }
    node->setRatio(ratio);
    emit ratioChanged(nodeId, ratio);
}

void QPaneTreeModel::reset(const QString& viewId)
{
    if (m_root) {
        m_root->deleteLater();
    }
    m_root = QPaneNode::makeLeaf(viewId, this);
    m_undoStack.clear();
    m_redoStack.clear();
    emit rootChanged();
    emit structureChanged();
    emit historyChanged();
    emit viewCreated(viewId);
}

bool QPaneTreeModel::createRootLeaf(const QString& viewId)
{
    if (m_root) {
        return false;
    }
    pushUndoState();
    m_root = QPaneNode::makeLeaf(viewId, this);
    emit rootChanged();
    emit structureChanged();
    emit viewCreated(viewId);
    return true;
}

bool QPaneTreeModel::flipOrientation(const QString& nodeId)
{
    QPaneNode* node = findNodeById(m_root, nodeId);
    if (!node || node->isLeaf()) {
        return false;
    }
    pushUndoState();
    node->setOrientation(node->orientation() == Qt::Horizontal
                         ? Qt::Vertical
                         : Qt::Horizontal);
    return true;
}

void QPaneTreeModel::undo()
{
    if (m_undoStack.isEmpty()) {
        return;
    }
    m_redoStack.push_back(saveState());
    const QVariantMap state = m_undoStack.takeLast();
    restoreStateInternal(state);
    emit historyChanged();
}

void QPaneTreeModel::redo()
{
    if (m_redoStack.isEmpty()) {
        return;
    }
    m_undoStack.push_back(saveState());
    const QVariantMap state = m_redoStack.takeLast();
    restoreStateInternal(state);
    emit historyChanged();
}

QVariantMap QPaneTreeModel::saveState() const
{
    return m_root ? m_root->toVariantMap() : QVariantMap{};
}

bool QPaneTreeModel::restoreState(const QVariantMap& state)
{
    pushUndoState();
    return restoreStateInternal(state);
}

bool QPaneTreeModel::restoreStateInternal(const QVariantMap& state)
{
    if (state.isEmpty()) {
        return false;
    }
    QPaneNode* newRoot = QPaneNode::fromVariantMap(state, this);
    if (!newRoot) {
        return false;
    }
    setRoot(newRoot);
    return true;
}

QPaneNode* QPaneTreeModel::findLeaf(const QString& viewId) const
{
    return findLeafById(m_root, viewId);
}

QPaneNode* QPaneTreeModel::findNode(const QString& nodeId) const
{
    return findNodeById(m_root, nodeId);
}

QStringList QPaneTreeModel::allViewIds() const
{
    QStringList ids;
    collectViewIds(m_root, ids);
    return ids;
}

QPaneNode* QPaneTreeModel::findLeafById(QPaneNode* node, const QString& viewId) const
{
    if (!node) {
        return nullptr;
    }
    if (node->isLeaf()) {
        return node->viewId() == viewId ? node : nullptr;
    }
    if (auto* f = findLeafById(node->first(), viewId)) {
        return f;
    }
    if (auto* s = findLeafById(node->second(), viewId)) {
        return s;
    }
    return nullptr;
}

QPaneNode* QPaneTreeModel::findNodeById(QPaneNode* node, const QString& nodeId) const
{
    if (!node) {
        return nullptr;
    }
    if (node->nodeId() == nodeId) {
        return node;
    }
    if (auto* f = findNodeById(node->first(), nodeId)) {
        return f;
    }
    if (auto* s = findNodeById(node->second(), nodeId)) {
        return s;
    }
    return nullptr;
}

QPaneNode* QPaneTreeModel::findLeafWithParent(const QString& viewId,
                                              QPaneNode*& outParent,
                                              bool& outIsFirst) const
{
    std::function<QPaneNode*(QPaneNode*, QPaneNode*, bool)> dfs =
        [&](QPaneNode* node, QPaneNode* parent, bool isFirst) -> QPaneNode* {
            if (!node) {
                return nullptr;
            }
            if (node->isLeaf() && node->viewId() == viewId) {
                outParent = parent;
                outIsFirst = isFirst;
                return node;
            }
            if (node->isSplit()) {
                if (auto* f = dfs(node->first(), node, true)) {
                    return f;
                }
                if (auto* s = dfs(node->second(), node, false)) {
                    return s;
                }
            }
            return nullptr;
        };
    return dfs(m_root, nullptr, false);
}

void QPaneTreeModel::collectViewIds(QPaneNode* node, QStringList& out) const
{
    if (!node) {
        return;
    }
    if (node->isLeaf()) {
        out << node->viewId();
        return;
    }
    collectViewIds(node->first(), out);
    collectViewIds(node->second(), out);
}

int QPaneTreeModel::countLeaves(QPaneNode* node) const
{
    if (!node) {
        return 0;
    }
    if (node->isLeaf()) {
        return 1;
    }
    return countLeaves(node->first()) + countLeaves(node->second());
}

void QPaneTreeModel::pushUndoState()
{
    m_undoStack.push_back(saveState());
    if (m_undoStack.size() > kMaxHistory) {
        m_undoStack.removeFirst();
    }
    m_redoStack.clear();
    emit historyChanged();
}

void QPaneTreeModel::setRoot(QPaneNode* newRoot)
{
    if (m_root) {
        m_root->setParent(nullptr);
        m_root->deleteLater();
    }
    m_root = newRoot;
    if (m_root) {
        m_root->setParent(this);
    }
    emit rootChanged();
    emit structureChanged();
}

void QPaneTreeModel::demoteNodeToLeaf(QPaneNode* node, const QString& viewId)
{
    node->demoteToLeaf(viewId);
}

namespace {

void dumpNode(QPaneNode* node, const QString& path, int depth)
{
    if (!node) {
        qCDebug(lcPaneTree).noquote() << QString(depth * 2, ' ') + path + " (null)";
        return;
    }
    const QString prefix = QString(depth * 2, ' ') + (path.isEmpty() ? "/" : path);
    const QString shortId = node->nodeId().left(8);
    if (node->isLeaf()) {
        qCDebug(lcPaneTree).noquote()
            << prefix + QString("  Leaf  vid=%1  id=%2")
                   .arg(node->viewId())
                   .arg(shortId);
    } else {
        const char* orient = node->orientation() == Qt::Horizontal ? "H" : "V";
        qCDebug(lcPaneTree).noquote()
            << prefix + QString("  Split%1  ratio=%2  id=%3")
                   .arg(orient)
                   .arg(node->ratio(), 0, 'f', 2)
                   .arg(shortId);
        dumpNode(node->first(), path + "/0", depth + 1);
        dumpNode(node->second(), path + "/1", depth + 1);
    }
}

} // anonymous namespace

void QPaneTreeModel::dump() const
{
    qCDebug(lcPaneTree).noquote() << "── PaneTreeModel dump ──────────";
    dumpNode(m_root, QString(), 0);
    qCDebug(lcPaneTree).noquote() << "────────────────────────────────";
}

} // namespace QPaneTree
