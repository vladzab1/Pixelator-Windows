#pragma once

#include <QObject>
#include <QString>

class ShortcutManager final : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutManager(QObject *parent = nullptr);

    Q_INVOKABLE bool createShortcuts();
    Q_INVOKABLE bool isFirstLaunch() const;
    Q_INVOKABLE void markFirstLaunchDone();

private:
    [[nodiscard]] bool createWindowsShortcut() const;
    [[nodiscard]] bool createLinuxDesktopEntry() const;
    [[nodiscard]] static QString getAppImagePath();
};
