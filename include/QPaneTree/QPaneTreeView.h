#pragma once

#include <QQuickItem>
#include <QQmlComponent>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include "QPaneTreeViewAttached.h"

namespace QPaneTree {

class QPaneTreeModel;

// QPaneTreeView —— 递归分割面板视图。C++ 壳子负责注册类型 + 持有属性，递归
// 渲染由内嵌 QML 实现（PaneTreeViewImpl.qml）完成。
//
// 用法（QML）：
//   PaneTreeView {
//       model: paneTreeModel
//       leafDelegate: Component { MyEditor { viewId: PaneTreeView.viewId } }
//       handleDelegate: Component { Rectangle { ... } }  // 可选
//       splitDelegate: Component { SplitView { ... } }   // 可选
//   }
class QPaneTreeView : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PaneTreeView)
    QML_ATTACHED(QPaneTreeViewAttached)

    Q_PROPERTY(QPaneTree::QPaneTreeModel* model
               READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* leafDelegate
               READ leafDelegate WRITE setLeafDelegate NOTIFY leafDelegateChanged)
    Q_PROPERTY(QQmlComponent* splitDelegate
               READ splitDelegate WRITE setSplitDelegate NOTIFY splitDelegateChanged)
    Q_PROPERTY(QQmlComponent* handleDelegate
               READ handleDelegate WRITE setHandleDelegate NOTIFY handleDelegateChanged)

public:
    explicit QPaneTreeView(QQuickItem* parent = nullptr);
    ~QPaneTreeView() override;

    QPaneTreeModel* model() const { return m_model; }
    QQmlComponent* leafDelegate() const { return m_leafDelegate; }
    QQmlComponent* splitDelegate() const { return m_splitDelegate; }
    QQmlComponent* handleDelegate() const { return m_handleDelegate; }

    void setModel(QPaneTreeModel* m);
    void setLeafDelegate(QQmlComponent* c);
    void setSplitDelegate(QQmlComponent* c);
    void setHandleDelegate(QQmlComponent* c);

    // QML_ATTACHED 钩子——给某个 item 返回它的 attached 实例
    static QPaneTreeViewAttached* qmlAttachedProperties(QObject* attachee);

signals:
    void modelChanged();
    void leafDelegateChanged();
    void splitDelegateChanged();
    void handleDelegateChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF& newGeo, const QRectF& oldGeo) override;

private:
    void rebuildImpl();
    void resizeImpl();

    QPointer<QPaneTreeModel> m_model;
    QQmlComponent* m_leafDelegate = nullptr;
    QQmlComponent* m_splitDelegate = nullptr;
    QQmlComponent* m_handleDelegate = nullptr;

    // 内嵌 QML 实现的根 item（PaneTreeViewImpl.qml 实例化结果）
    QQuickItem* m_impl = nullptr;
};

} // namespace QPaneTree
