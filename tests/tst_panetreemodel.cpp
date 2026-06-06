#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>

#include "QPaneTree/QPaneNode.h"
#include "QPaneTree/QPaneTreeModel.h"

using namespace QPaneTree;

class TstPaneTreeModel : public QObject
{
    Q_OBJECT

private slots:
    // ── 初始状态 / reset ────────────────────────────────────────────────
    void initialState_hasDefaultLeaf();
    void reset_replacesRoot();
    void reset_resetsHistory();
    void reset_setsActiveLeaf();

    // ── split ───────────────────────────────────────────────────────────
    void split_returnsNewViewId();
    void split_inPlacePromote_keepsTargetNodeId();
    void split_originalLeafInheritsData();
    void split_invalidViewId_returnsEmpty();
    void split_emitsStructureChanged();

    // ── close ───────────────────────────────────────────────────────────
    void close_leafInSubtree_demotesParent();
    void close_lastLeaf_clearsRoot();
    void close_invalidViewId_returnsFalse();
    void close_emitsViewClosed();
    void close_demotedParentInheritsSiblingData();

    // ── createRootLeaf ──────────────────────────────────────────────────
    void createRootLeaf_whenEmpty_succeeds();
    void createRootLeaf_whenNotEmpty_fails();

    // ── updateRatio / flipOrientation ───────────────────────────────────
    void updateRatio_updatesNode();
    void flipOrientation_swapsHV();

    // ── 查询 / 遍历 ────────────────────────────────────────────────────
    void findLeaf_byViewId();
    void findNode_byNodeId();
    void allViewIds_DFSOrder();
    void allLeaves_DFSOrder();
    void parentOf_siblingOf();

    // ── undo / redo ─────────────────────────────────────────────────────
    void undo_revertsSplit();
    void redo_replaysSplit();
    void canUndo_canRedo_flags();

    // ── 持久化 ──────────────────────────────────────────────────────────
    void saveState_restoreState_roundTrip();
    void restoreState_preservesData();

    // ── activeLeaf ──────────────────────────────────────────────────────
    void setActiveLeaf_setsAndEmits();
    void setActiveLeaf_rejectsNonLeaf();
    void setActiveLeaf_rejectsForeignNode();
    void activeLeaf_fallsBackWhenClosed();

    // ── maximizedLeaf ───────────────────────────────────────────────────
    void setMaximizedLeaf_setsAndEmits();
    void toggleMaximize_togglesOnOff();
    void maximizedLeaf_clearsWhenLeafClosed();

    // ── focus 导航 ─────────────────────────────────────────────────────
    void focusNext_cyclesForward();
    void focusPrevious_cyclesBackward();
    void focusNext_singleLeaf_staysSame();
};

// 拆出一个标准 3-leaf 树用于多个测试。
//   root SplitH
//   ├── L  = Leaf "L"
//   └── SplitV
//       ├── R  = Leaf "R"   (target "R" 升级为 SplitV 时，originalLeaf 保留这个 viewId)
//       └── BR = Leaf "BR"  (新建的 newLeaf)
// leaves DFS 顺序 = [L, R, BR]。
static void buildThreeLeafTree(QPaneTreeModel& model)
{
    model.reset("L");
    model.split("L", Qt::Horizontal, "R", /*after=*/true);
    model.split("R", Qt::Vertical, "BR", /*after=*/true);
}

// ─── 初始状态 / reset ─────────────────────────────────────────────────────

void TstPaneTreeModel::initialState_hasDefaultLeaf()
{
    QPaneTreeModel model;
    QVERIFY(model.root() != nullptr);
    QVERIFY(model.root()->isLeaf());
    QCOMPARE(model.root()->viewId(), QStringLiteral("default"));
    QCOMPARE(model.leafCount(), 1);
    QVERIFY(!model.canUndo());
    QVERIFY(!model.canRedo());
}

void TstPaneTreeModel::reset_replacesRoot()
{
    QPaneTreeModel model;
    auto* origRoot = model.root();
    model.reset("new-root");
    QVERIFY(model.root() != origRoot);
    QCOMPARE(model.root()->viewId(), QStringLiteral("new-root"));
}

void TstPaneTreeModel::reset_resetsHistory()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal);
    QVERIFY(model.canUndo());
    model.reset("x");
    QVERIFY(!model.canUndo());
    QVERIFY(!model.canRedo());
}

void TstPaneTreeModel::reset_setsActiveLeaf()
{
    QPaneTreeModel model;
    QCOMPARE(model.activeLeaf(), model.root());
    model.reset("after-reset");
    QCOMPARE(model.activeLeaf(), model.root());
}

// ─── split ────────────────────────────────────────────────────────────────

void TstPaneTreeModel::split_returnsNewViewId()
{
    QPaneTreeModel model;
    const QString id = model.split("default", Qt::Horizontal, "right", true);
    QCOMPARE(id, QStringLiteral("right"));
    QCOMPARE(model.leafCount(), 2);
    QVERIFY(model.findLeaf("default") != nullptr);
    QVERIFY(model.findLeaf("right") != nullptr);
}

void TstPaneTreeModel::split_inPlacePromote_keepsTargetNodeId()
{
    QPaneTreeModel model;
    QPaneNode* target = model.root();
    const QString targetNodeId = target->nodeId();
    model.split("default", Qt::Horizontal);
    // root 是同一指针（promotion in-place），但现在是 Split
    QCOMPARE(model.root(), target);
    QCOMPARE(model.root()->nodeId(), targetNodeId);
    QVERIFY(model.root()->isSplit());
}

void TstPaneTreeModel::split_originalLeafInheritsData()
{
    QPaneTreeModel model;
    model.root()->setData(QStringLiteral("orig-payload"));
    model.split("default", Qt::Horizontal, "new-side", true);
    auto* preserved = model.findLeaf("default");
    QVERIFY(preserved != nullptr);
    QCOMPARE(preserved->data().toString(), QStringLiteral("orig-payload"));
    auto* fresh = model.findLeaf("new-side");
    QVERIFY(fresh != nullptr);
    QVERIFY(!fresh->data().isValid());
}

void TstPaneTreeModel::split_invalidViewId_returnsEmpty()
{
    QPaneTreeModel model;
    const QString id = model.split("not-there", Qt::Horizontal);
    QVERIFY(id.isEmpty());
    QCOMPARE(model.leafCount(), 1);
}

void TstPaneTreeModel::split_emitsStructureChanged()
{
    QPaneTreeModel model;
    QSignalSpy structSpy(&model, &QPaneTreeModel::structureChanged);
    QSignalSpy createdSpy(&model, &QPaneTreeModel::viewCreated);
    model.split("default", Qt::Horizontal, "x");
    QCOMPARE(structSpy.count(), 1);
    QCOMPARE(createdSpy.count(), 1);
}

// ─── close ───────────────────────────────────────────────────────────────

void TstPaneTreeModel::close_leafInSubtree_demotesParent()
{
    QPaneTreeModel model;
    buildThreeLeafTree(model);
    // 关 "BR" → 右边的 SplitV 应被降级为 leaf "R"（sibling 的 viewId）
    QVERIFY(model.close("BR"));
    QCOMPARE(model.leafCount(), 2);
    QVERIFY(model.findLeaf("R") != nullptr);
    QVERIFY(model.findLeaf("L") != nullptr);
    QVERIFY(model.findLeaf("BR") == nullptr);
    // 顶层仍是 SplitH，root.second 现在是 leaf "R"
    QVERIFY(model.root()->isSplit());
    QCOMPARE(model.root()->second()->viewId(), QStringLiteral("R"));
}

void TstPaneTreeModel::close_lastLeaf_clearsRoot()
{
    QPaneTreeModel model;
    QVERIFY(model.close("default"));
    QVERIFY(model.root() == nullptr);
    QCOMPARE(model.leafCount(), 0);
}

void TstPaneTreeModel::close_invalidViewId_returnsFalse()
{
    QPaneTreeModel model;
    QVERIFY(!model.close("nope"));
    QCOMPARE(model.leafCount(), 1);
}

void TstPaneTreeModel::close_emitsViewClosed()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal, "extra");
    QSignalSpy spy(&model, &QPaneTreeModel::viewClosed);
    model.close("extra");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QStringLiteral("extra"));
}

void TstPaneTreeModel::close_demotedParentInheritsSiblingData()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal, "extra");
    // 让 "default" leaf 携带 data；关 "extra"，sibling="default"，parent 降级
    // 为 leaf，应带上 default 的 viewId+data
    auto* defLeaf = model.findLeaf("default");
    defLeaf->setData(QStringLiteral("def-data"));
    // 当前 model.split 在 root 上做 promote——parent == root，走 sibling 升新根分支
    // 该分支 sibling 直接成为新 root，data 跟在 sibling 上，自然保留。
    model.close("extra");
    QCOMPARE(model.root()->viewId(), QStringLiteral("default"));
    QCOMPARE(model.root()->data().toString(), QStringLiteral("def-data"));
}

// ─── createRootLeaf ──────────────────────────────────────────────────────

void TstPaneTreeModel::createRootLeaf_whenEmpty_succeeds()
{
    QPaneTreeModel model;
    model.close("default"); // 清空
    QVERIFY(model.root() == nullptr);
    QVERIFY(model.createRootLeaf("fresh"));
    QCOMPARE(model.root()->viewId(), QStringLiteral("fresh"));
}

void TstPaneTreeModel::createRootLeaf_whenNotEmpty_fails()
{
    QPaneTreeModel model;
    QVERIFY(!model.createRootLeaf("ignored"));
    QCOMPARE(model.root()->viewId(), QStringLiteral("default"));
}

// ─── updateRatio / flipOrientation ───────────────────────────────────────

void TstPaneTreeModel::updateRatio_updatesNode()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal);
    QPaneNode* split = model.root();
    QVERIFY(split->isSplit());
    model.updateRatio(split->nodeId(), 0.3);
    QCOMPARE(split->ratio(), 0.3);
}

void TstPaneTreeModel::flipOrientation_swapsHV()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal);
    QPaneNode* split = model.root();
    QCOMPARE(split->orientation(), Qt::Horizontal);
    QVERIFY(model.flipOrientation(split->nodeId()));
    QCOMPARE(split->orientation(), Qt::Vertical);
}

// ─── 查询 / 遍历 ────────────────────────────────────────────────────────

void TstPaneTreeModel::findLeaf_byViewId()
{
    QPaneTreeModel model;
    buildThreeLeafTree(model);
    QVERIFY(model.findLeaf("L") != nullptr);
    QVERIFY(model.findLeaf("R") != nullptr);
    QVERIFY(model.findLeaf("BR") != nullptr);
    QVERIFY(model.findLeaf("???") == nullptr);
}

void TstPaneTreeModel::findNode_byNodeId()
{
    QPaneTreeModel model;
    const QString id = model.root()->nodeId();
    QCOMPARE(model.findNode(id), model.root());
    QVERIFY(model.findNode("no-such") == nullptr);
}

void TstPaneTreeModel::allViewIds_DFSOrder()
{
    QPaneTreeModel model;
    buildThreeLeafTree(model);
    const QStringList vids = model.allViewIds();
    QCOMPARE(vids, (QStringList{"L", "R", "BR"}));
}

void TstPaneTreeModel::allLeaves_DFSOrder()
{
    QPaneTreeModel model;
    buildThreeLeafTree(model);
    const auto leaves = model.allLeaves();
    QCOMPARE(leaves.size(), 3);
    QCOMPARE(leaves.at(0)->viewId(), QStringLiteral("L"));
    QCOMPARE(leaves.at(1)->viewId(), QStringLiteral("R"));
    QCOMPARE(leaves.at(2)->viewId(), QStringLiteral("BR"));
}

void TstPaneTreeModel::parentOf_siblingOf()
{
    QPaneTreeModel model;
    buildThreeLeafTree(model);
    auto* L = model.findLeaf("L");
    auto* R = model.findLeaf("R");
    auto* BR = model.findLeaf("BR");
    QCOMPARE(model.parentOf(L), model.root());
    QCOMPARE(model.siblingOf(L), model.parentOf(R));
    QCOMPARE(model.siblingOf(R), BR);
    QVERIFY(model.parentOf(model.root()) == nullptr);
}

// ─── undo / redo ─────────────────────────────────────────────────────────

void TstPaneTreeModel::undo_revertsSplit()
{
    QPaneTreeModel model;
    QCOMPARE(model.leafCount(), 1);
    model.split("default", Qt::Horizontal);
    QCOMPARE(model.leafCount(), 2);
    model.undo();
    QCOMPARE(model.leafCount(), 1);
}

void TstPaneTreeModel::redo_replaysSplit()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal);
    model.undo();
    QCOMPARE(model.leafCount(), 1);
    model.redo();
    QCOMPARE(model.leafCount(), 2);
}

void TstPaneTreeModel::canUndo_canRedo_flags()
{
    QPaneTreeModel model;
    QVERIFY(!model.canUndo());
    QVERIFY(!model.canRedo());
    model.split("default", Qt::Horizontal);
    QVERIFY(model.canUndo());
    QVERIFY(!model.canRedo());
    model.undo();
    QVERIFY(model.canRedo());
}

// ─── 持久化 ─────────────────────────────────────────────────────────────

void TstPaneTreeModel::saveState_restoreState_roundTrip()
{
    QPaneTreeModel a;
    buildThreeLeafTree(a);
    const QVariantMap state = a.saveState();

    QPaneTreeModel b;
    QVERIFY(b.restoreState(state));
    QCOMPARE(b.allViewIds(), (QStringList{"L", "R", "BR"}));
    QCOMPARE(b.leafCount(), 3);
}

void TstPaneTreeModel::restoreState_preservesData()
{
    QPaneTreeModel a;
    a.root()->setData(QStringLiteral("payload"));
    const QVariantMap state = a.saveState();

    QPaneTreeModel b;
    QVERIFY(b.restoreState(state));
    QCOMPARE(b.root()->data().toString(), QStringLiteral("payload"));
}

// ─── activeLeaf ──────────────────────────────────────────────────────────

void TstPaneTreeModel::setActiveLeaf_setsAndEmits()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal, "B");
    QPaneNode* b = model.findLeaf("B");
    QSignalSpy spy(&model, &QPaneTreeModel::activeLeafChanged);
    model.setActiveLeaf(b);
    QCOMPARE(model.activeLeaf(), b);
    QCOMPARE(spy.count(), 1);
}

void TstPaneTreeModel::setActiveLeaf_rejectsNonLeaf()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal);
    QPaneNode* before = model.activeLeaf();
    model.setActiveLeaf(model.root()); // root 现在是 Split，不允许
    QCOMPARE(model.activeLeaf(), before);
}

void TstPaneTreeModel::setActiveLeaf_rejectsForeignNode()
{
    QPaneTreeModel model;
    auto* foreign = QPaneNode::makeLeaf("foreign");
    QPaneNode* before = model.activeLeaf();
    model.setActiveLeaf(foreign);
    QCOMPARE(model.activeLeaf(), before);
    delete foreign;
}

void TstPaneTreeModel::activeLeaf_fallsBackWhenClosed()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal, "B");
    QPaneNode* b = model.findLeaf("B");
    model.setActiveLeaf(b);
    model.close("B");
    // active 应自动回退到剩下的 default leaf
    QVERIFY(model.activeLeaf() != nullptr);
    QCOMPARE(model.activeLeaf()->viewId(), QStringLiteral("default"));
}

// ─── maximizedLeaf ──────────────────────────────────────────────────────

void TstPaneTreeModel::setMaximizedLeaf_setsAndEmits()
{
    QPaneTreeModel model;
    QPaneNode* leaf = model.root();
    QSignalSpy spy(&model, &QPaneTreeModel::maximizedLeafChanged);
    model.setMaximizedLeaf(leaf);
    QCOMPARE(model.maximizedLeaf(), leaf);
    QCOMPARE(spy.count(), 1);
}

void TstPaneTreeModel::toggleMaximize_togglesOnOff()
{
    QPaneTreeModel model;
    QPaneNode* leaf = model.root();
    model.toggleMaximize(leaf);
    QCOMPARE(model.maximizedLeaf(), leaf);
    model.toggleMaximize(leaf);
    QVERIFY(model.maximizedLeaf() == nullptr);
}

void TstPaneTreeModel::maximizedLeaf_clearsWhenLeafClosed()
{
    QPaneTreeModel model;
    model.split("default", Qt::Horizontal, "B");
    QPaneNode* b = model.findLeaf("B");
    model.setMaximizedLeaf(b);
    QCOMPARE(model.maximizedLeaf(), b);
    model.close("B");
    QVERIFY(model.maximizedLeaf() == nullptr);
}

// ─── focus 导航 ─────────────────────────────────────────────────────────

void TstPaneTreeModel::focusNext_cyclesForward()
{
    QPaneTreeModel model;
    buildThreeLeafTree(model);
    model.setActiveLeaf(model.findLeaf("L"));
    model.focusNext();
    QCOMPARE(model.activeLeaf()->viewId(), QStringLiteral("R"));
    model.focusNext();
    QCOMPARE(model.activeLeaf()->viewId(), QStringLiteral("BR"));
    model.focusNext();
    QCOMPARE(model.activeLeaf()->viewId(), QStringLiteral("L"));
}

void TstPaneTreeModel::focusPrevious_cyclesBackward()
{
    QPaneTreeModel model;
    buildThreeLeafTree(model);
    model.setActiveLeaf(model.findLeaf("L"));
    model.focusPrevious();
    QCOMPARE(model.activeLeaf()->viewId(), QStringLiteral("BR"));
    model.focusPrevious();
    QCOMPARE(model.activeLeaf()->viewId(), QStringLiteral("R"));
}

void TstPaneTreeModel::focusNext_singleLeaf_staysSame()
{
    QPaneTreeModel model;
    QPaneNode* only = model.root();
    model.focusNext();
    QCOMPARE(model.activeLeaf(), only);
}

QTEST_GUILESS_MAIN(TstPaneTreeModel)
#include "tst_panetreemodel.moc"
