// qpanetree 渲染层测试的入口 —— QtQuickTest 跑 tests/qml 下所有 tst_*.qml。
//
// **为什么需要它**:此前本库只有模型层的 C++ 单测,而 leaf delegate 的生命周期
// 缺陷(树一变就把幸存那一格的 delegate 销毁重建)整个发生在渲染层 —— 模型测试
// 一条都照不到,缺陷在库里活了很久,是宿主先撞上的。
//
// 自己写入口而不用 QUICK_TEST_MAIN:得先把构建出来的 PaneTree 模块目录加进
// import path,否则测试里 `import PaneTree` 解析不了。

#include <QQmlEngine>
#include <QQuickStyle>
#include <QtQuickTest>

class Setup : public QObject
{
    Q_OBJECT

public:
    Setup()
    {
        // 跟示例程序一致。不设的话 macOS 上走 NativeStyle,而 SplitView 在不同
        // 样式下是两份不同实现。
        QQuickStyle::setStyle(QStringLiteral("Basic"));
    }

public slots:
    void qmlEngineAvailable(QQmlEngine* engine)
    {
        engine->addImportPath(QStringLiteral(QPANETREE_QML_IMPORT_PATH));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(qpanetree_view, Setup)

#include "tst_qml_view.moc"
