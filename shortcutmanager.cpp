#include "shortcutmanager.h"

#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#endif

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
{
}

bool ShortcutManager::createShortcuts()
{
#ifdef Q_OS_WIN
    return createWindowsShortcut();
#elif defined(Q_OS_LINUX)
    return createLinuxDesktopEntry();
#else
    qWarning() << "Shortcuts not supported on this platform";
    return false;
#endif
}

bool ShortcutManager::isFirstLaunch() const
{
    QSettings settings;
    return !settings.value("first_launch_done", false).toBool();
}

void ShortcutManager::markFirstLaunchDone()
{
    QSettings settings;
    settings.setValue("first_launch_done", true);
    settings.sync();
}

bool ShortcutManager::createWindowsShortcut() const
{
#ifdef Q_OS_WIN
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString shortcutPath = desktopPath + "\\Pixelator.lnk";
    QString targetPath = QCoreApplication::applicationFilePath();

    HRESULT hres;
    IShellLinkW* psl = nullptr;

    CoInitialize(nullptr);

    hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hres))
    {
        psl->SetPath(targetPath.toStdWString().c_str());
        psl->SetIconLocation(targetPath.toStdWString().c_str(), 0);
        psl->SetWorkingDirectory(QFileInfo(targetPath).absolutePath().toStdWString().c_str());

        IPersistFile* ppf = nullptr;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres))
        {
            hres = ppf->Save(shortcutPath.toStdWString().c_str(), TRUE);
            ppf->Release();

            if (SUCCEEDED(hres))
            {
                qInfo() << "Desktop shortcut created successfully at:" << shortcutPath;
                psl->Release();
                CoUninitialize();
                return true;
            }
        }

        psl->Release();
    }

    CoUninitialize();
    qWarning() << "Failed to create desktop shortcut";
    return false;

#else
    return false;
#endif
}

bool ShortcutManager::createLinuxDesktopEntry() const
{
#ifdef Q_OS_LINUX
    QString appPath = getAppImagePath();
    if (appPath.isEmpty())
    {
        appPath = QCoreApplication::applicationFilePath();
    }

    QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (desktopDir.isEmpty())
    {
        desktopDir = QDir::homePath() + "/.local/share/applications";
    }

    QDir dir(desktopDir);
    if (!dir.exists() && !dir.mkpath("."))
    {
        qWarning() << "Failed to create directory:" << desktopDir;
        return false;
    }

    QString desktopFilePath = desktopDir + "/Pixelator.desktop";

    QString iconPath = QString("%1/.local/share/icons/hicolor/scalable/apps/pixelator.svg").arg(QDir::homePath());
    QString desktopContent = QString(
        "[Desktop Entry]\n"
        "Version=1.0\n"
        "Type=Application\n"
        "Name=Pixelator\n"
        "Comment=Sprite Cleaner - Batch Edition\n"
        "Exec=%1\n"
        "Icon=%2\n"
        "Terminal=false\n"
        "Categories=Graphics;Utility;\n"
    ).arg(appPath, iconPath);

    QFile file(desktopFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open file for writing:" << desktopFilePath;
        return false;
    }

    file.write(desktopContent.toUtf8());
    file.close();

    // Make the file executable
    QFile::setPermissions(desktopFilePath, QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
                                           QFile::ReadGroup | QFile::ExeGroup |
                                           QFile::ReadOther | QFile::ExeOther);

    qInfo() << "Desktop entry created successfully at:" << desktopFilePath;
    return true;

#else
    return false;
#endif
}

QString ShortcutManager::getAppImagePath()
{
    // Check if running from AppImage
    if (!qEnvironmentVariableIsEmpty("APPIMAGE"))
    {
        return qgetenv("APPIMAGE");
    }

    if (!qEnvironmentVariableIsEmpty("ARGV0"))
    {
        QString argv0 = qgetenv("ARGV0");
        if (argv0.endsWith(".AppImage"))
        {
            return argv0;
        }
    }

    return QString();
}
