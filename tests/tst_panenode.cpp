#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>

#include "QPaneTree/QPaneNode.h"

using namespace QPaneTree;

class TstPaneNode : public QObject
{
    Q_OBJECT

private slots:
    // ── 工厂 ─────────────────────────────────────────────────────────────
    void makeLeaf_setsType();
    void makeLeaf_uniqueNodeIds();
    void makeSplit_parentsChildren();
    void makeSplit_clampsRatio();

    // ── 直接 setter ──────────────────────────────────────────────────────
    void setRatio_clampedAndNotifies();
    void setRatio_noOpDoesNotEmit();
    void setData_emitsOnChange();
    void setViewId_emitsOnChange();

    // ── promote / demote ────────────────────────────────────────────────
    void promoteToSplit_changesType();
    void promoteToSplit_clearsData();
    void demoteToLeaf_inheritsViewIdAndData();

    // ── 序列化 ──────────────────────────────────────────────────────────
    void toVariantMap_leaf();
    void toVariantMap_split_recursive();
    void fromVariantMap_leafRoundTrip();
    void fromVariantMap_splitRoundTrip();
    void fromVariantMap_dataRoundTrip();
};

// ─── 工厂 ─────────────────────────────────────────────────────────────────

void TstPaneNode::makeLeaf_setsType()
{
    auto* leaf = QPaneNode::makeLeaf("view-1");
    QVERIFY(leaf->isLeaf());
    QVERIFY(!leaf->isSplit());
    QCOMPARE(leaf->viewId(), QStringLiteral("view-1"));
    QVERIFY(!leaf->nodeId().isEmpty());
    delete leaf;
}

void TstPaneNode::makeLeaf_uniqueNodeIds()
{
    auto* a = QPaneNode::makeLeaf("a");
    auto* b = QPaneNode::makeLeaf("b");
    QVERIFY(a->nodeId() != b->nodeId());
    delete a;
    delete b;
}

void TstPaneNode::makeSplit_parentsChildren()
{
    auto* l = QPaneNode::makeLeaf("l");
    auto* r = QPaneNode::makeLeaf("r");
    auto* split = QPaneNode::makeSplit(Qt::Horizontal, 0.4, l, r);
    QVERIFY(split->isSplit());
    QCOMPARE(split->orientation(), Qt::Horizontal);
    QCOMPARE(split->first(), l);
    QCOMPARE(split->second(), r);
    QCOMPARE(l->parent(), split);
    QCOMPARE(r->parent(), split);
    delete split;
}

void TstPaneNode::makeSplit_clampsRatio()
{
    auto* l = QPaneNode::makeLeaf("l");
    auto* r = QPaneNode::makeLeaf("r");
    auto* tooLow = QPaneNode::makeSplit(Qt::Horizontal, 0.01, l, r);
    QCOMPARE(tooLow->ratio(), 0.1);
    delete tooLow;

    auto* l2 = QPaneNode::makeLeaf("l");
    auto* r2 = QPaneNode::makeLeaf("r");
    auto* tooHigh = QPaneNode::makeSplit(Qt::Horizontal, 0.99, l2, r2);
    QCOMPARE(tooHigh->ratio(), 0.9);
    delete tooHigh;
}

// ─── setter ───────────────────────────────────────────────────────────────

void TstPaneNode::setRatio_clampedAndNotifies()
{
    auto* split = QPaneNode::makeSplit(Qt::Horizontal,
                                       0.5,
                                       QPaneNode::makeLeaf("a"),
                                       QPaneNode::makeLeaf("b"));
    QSignalSpy spy(split, &QPaneNode::ratioChanged);
    split->setRatio(0.7);
    QCOMPARE(split->ratio(), 0.7);
    QCOMPARE(spy.count(), 1);

    split->setRatio(2.0);
    QCOMPARE(split->ratio(), 0.9);
    QCOMPARE(spy.count(), 2);
    delete split;
}

void TstPaneNode::setRatio_noOpDoesNotEmit()
{
    auto* split = QPaneNode::makeSplit(Qt::Horizontal,
                                       0.5,
                                       QPaneNode::makeLeaf("a"),
                                       QPaneNode::makeLeaf("b"));
    QSignalSpy spy(split, &QPaneNode::ratioChanged);
    split->setRatio(0.5);
    QCOMPARE(spy.count(), 0);
    delete split;
}

void TstPaneNode::setData_emitsOnChange()
{
    auto* leaf = QPaneNode::makeLeaf("v");
    QSignalSpy spy(leaf, &QPaneNode::dataChanged);
    leaf->setData(QStringLiteral("hello"));
    QCOMPARE(leaf->data().toString(), QStringLiteral("hello"));
    QCOMPARE(spy.count(), 1);
    leaf->setData(QStringLiteral("hello"));
    QCOMPARE(spy.count(), 1);
    delete leaf;
}

void TstPaneNode::setViewId_emitsOnChange()
{
    auto* leaf = QPaneNode::makeLeaf("old");
    QSignalSpy spy(leaf, &QPaneNode::viewIdChanged);
    leaf->setViewId("new");
    QCOMPARE(leaf->viewId(), QStringLiteral("new"));
    QCOMPARE(spy.count(), 1);
    delete leaf;
}

// ─── promote / demote ────────────────────────────────────────────────────

void TstPaneNode::promoteToSplit_changesType()
{
    auto* leaf = QPaneNode::makeLeaf("orig");
    const QString origNodeId = leaf->nodeId();
    QSignalSpy typeSpy(leaf, &QPaneNode::nodeTypeChanged);
    QSignalSpy firstSpy(leaf, &QPaneNode::firstChanged);

    auto* a = QPaneNode::makeLeaf("a");
    auto* b = QPaneNode::makeLeaf("b");
    leaf->promoteToSplit(Qt::Vertical, 0.3, a, b);

    QVERIFY(leaf->isSplit());
    QCOMPARE(leaf->orientation(), Qt::Vertical);
    QCOMPARE(leaf->first(), a);
    QCOMPARE(leaf->second(), b);
    // nodeId 跨升级保持稳定
    QCOMPARE(leaf->nodeId(), origNodeId);
    QCOMPARE(typeSpy.count(), 1);
    QCOMPARE(firstSpy.count(), 1);
    delete leaf;
}

void TstPaneNode::promoteToSplit_clearsData()
{
    auto* leaf = QPaneNode::makeLeaf("v");
    leaf->setData(QStringLiteral("payload"));
    QSignalSpy dataSpy(leaf, &QPaneNode::dataChanged);

    leaf->promoteToSplit(Qt::Horizontal,
                         0.5,
                         QPaneNode::makeLeaf("a"),
                         QPaneNode::makeLeaf("b"));
    QVERIFY(!leaf->data().isValid());
    QCOMPARE(dataSpy.count(), 1);
    delete leaf;
}

void TstPaneNode::demoteToLeaf_inheritsViewIdAndData()
{
    auto* parent = QPaneNode::makeSplit(Qt::Horizontal,
                                        0.5,
                                        QPaneNode::makeLeaf("a"),
                                        QPaneNode::makeLeaf("b"));
    const QString origNodeId = parent->nodeId();
    parent->demoteToLeaf("survivor", QVariant(QStringLiteral("survivor-data")));

    QVERIFY(parent->isLeaf());
    QCOMPARE(parent->viewId(), QStringLiteral("survivor"));
    QCOMPARE(parent->data().toString(), QStringLiteral("survivor-data"));
    QVERIFY(parent->first() == nullptr);
    QVERIFY(parent->second() == nullptr);
    QCOMPARE(parent->nodeId(), origNodeId);
    delete parent;
}

// ─── 序列化 ──────────────────────────────────────────────────────────────

void TstPaneNode::toVariantMap_leaf()
{
    auto* leaf = QPaneNode::makeLeaf("v");
    leaf->setData(QStringLiteral("foo"));
    const QVariantMap map = leaf->toVariantMap();
    QCOMPARE(map.value("type").toString(), QStringLiteral("leaf"));
    QCOMPARE(map.value("viewId").toString(), QStringLiteral("v"));
    QCOMPARE(map.value("data").toString(), QStringLiteral("foo"));
    delete leaf;
}

void TstPaneNode::toVariantMap_split_recursive()
{
    auto* split = QPaneNode::makeSplit(Qt::Vertical,
                                       0.3,
                                       QPaneNode::makeLeaf("L"),
                                       QPaneNode::makeLeaf("R"));
    const QVariantMap map = split->toVariantMap();
    QCOMPARE(map.value("type").toString(), QStringLiteral("split"));
    QCOMPARE(map.value("orientation").toInt(), static_cast<int>(Qt::Vertical));
    QCOMPARE(map.value("ratio").toReal(), 0.3);
    QCOMPARE(map.value("first").toMap().value("viewId").toString(), QStringLiteral("L"));
    QCOMPARE(map.value("second").toMap().value("viewId").toString(), QStringLiteral("R"));
    delete split;
}

void TstPaneNode::fromVariantMap_leafRoundTrip()
{
    auto* leaf = QPaneNode::makeLeaf("v");
    const QVariantMap map = leaf->toVariantMap();
    delete leaf;

    auto* restored = QPaneNode::fromVariantMap(map);
    QVERIFY(restored->isLeaf());
    QCOMPARE(restored->viewId(), QStringLiteral("v"));
    delete restored;
}

void TstPaneNode::fromVariantMap_splitRoundTrip()
{
    auto* split = QPaneNode::makeSplit(Qt::Vertical,
                                       0.42,
                                       QPaneNode::makeLeaf("L"),
                                       QPaneNode::makeLeaf("R"));
    const QVariantMap map = split->toVariantMap();
    delete split;

    auto* restored = QPaneNode::fromVariantMap(map);
    QVERIFY(restored->isSplit());
    QCOMPARE(restored->orientation(), Qt::Vertical);
    QCOMPARE(restored->ratio(), 0.42);
    QCOMPARE(restored->first()->viewId(), QStringLiteral("L"));
    QCOMPARE(restored->second()->viewId(), QStringLiteral("R"));
    delete restored;
}

void TstPaneNode::fromVariantMap_dataRoundTrip()
{
    auto* leaf = QPaneNode::makeLeaf("v");
    QVariantMap payload;
    payload["note"] = "hello";
    payload["scroll"] = 42;
    leaf->setData(payload);
    const QVariantMap map = leaf->toVariantMap();
    delete leaf;

    auto* restored = QPaneNode::fromVariantMap(map);
    QCOMPARE(restored->data().toMap().value("note").toString(), QStringLiteral("hello"));
    QCOMPARE(restored->data().toMap().value("scroll").toInt(), 42);
    delete restored;
}

QTEST_GUILESS_MAIN(TstPaneNode)
#include "tst_panenode.moc"
