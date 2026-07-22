#include "pixelworker.h"
#include "shortcutmanager.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        const bool inAppImage = !qEnvironmentVariableIsEmpty("APPIMAGE")
                             || !qEnvironmentVariableIsEmpty("ARGV0");
        qputenv("QT_QPA_PLATFORM", inAppImage ? "xcb" : "wayland;xcb");
    }
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("Pixelator");
    app.setOrganizationName("Pixelator Studio");

    PixelWorker pixelWorker;
    ShortcutManager shortcutManager;
    
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("pixelWorker", &pixelWorker);
    engine.rootContext()->setContextProperty("shortcutManager", &shortcutManager);
    
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
