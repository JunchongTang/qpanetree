#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication::setOrganizationName("qpanetree");
    QGuiApplication::setApplicationName("editor-demo");

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("PaneTreeEditorDemo", "Main");
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
