#include "pixelworker.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    // Prefer Wayland when available, while retaining X11 compatibility.
    qputenv("QT_QPA_PLATFORM", qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")
                                     ? "wayland;xcb"
                                     : qgetenv("QT_QPA_PLATFORM"));
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("Pixelator");
    app.setOrganizationName("Pixelator Studio");

    PixelWorker pixelWorker;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("pixelWorker", &pixelWorker);
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings)
            qWarning().noquote() << warning.toString();
    });
    // The QML module is compiled into the executable, keeping the AppImage self-contained.
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Pixelator/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
