#pragma once
#include <QSettings>
#include <QKeySequence>
#include <QVariant>

class GlobalConfigManager
{
    static GlobalConfigManager* s_instance;

    static QString configPath;
    std::unique_ptr<QSettings> settings{ nullptr };

    GlobalConfigManager() = default;
public:
    constexpr static const char* configFileName = "config.ini";

    enum ConfigKey {
        StartWithWindows,
        ShowInSystemTray,
        SavedDataFilePath,
        SavedDataImagePath,
        SavedDataTimerInterval,
        ClearHistoryOnLoad,
        ClearHistoryOnQuit,
        ClipboardHistoryRetentionDays,
        HistorySize,
        MouseDoubleLeftClickInterval,
        MouseDoubleMiddleClickInterval,
        MouseDoubleRightClickInterval,
        ShowWindowShortcut,
        QSSMainWindowStyle,
    };

    constexpr static const char* configKeyToString(ConfigKey key) {
        switch (key)
        {
        case GlobalConfigManager::StartWithWindows:
            return "StartWithWindows";
        case GlobalConfigManager::ShowInSystemTray:
            return "ShowInSystemTray";
        case GlobalConfigManager::SavedDataFilePath:
            return "SavedDataFilePath";
        case GlobalConfigManager::SavedDataImagePath:
            return "SavedDataImagePath";
        case GlobalConfigManager::SavedDataTimerInterval:
            return "SavedDataTimerInterval";
        case GlobalConfigManager::ClearHistoryOnLoad:
            return "ClearHistoryOnLoad";
        case GlobalConfigManager::ClearHistoryOnQuit:
            return "ClearHistoryOnQuit";
        case GlobalConfigManager::ClipboardHistoryRetentionDays:
            return "ClipboardHistoryRetentionDays";
        case GlobalConfigManager::HistorySize:
            return "HistorySize";
        case GlobalConfigManager::MouseDoubleLeftClickInterval:
            return "MouseDoubleLeftClickInterval";
        case GlobalConfigManager::MouseDoubleMiddleClickInterval:
            return "MouseDoubleMiddleClickInterval";
        case GlobalConfigManager::MouseDoubleRightClickInterval:
            return "MouseDoubleRightClickInterval";
        case GlobalConfigManager::ShowWindowShortcut:
            return "ShowWindowShortcut";
        case GlobalConfigManager::QSSMainWindowStyle:
            return "QSSMainWindowStyle";
        default:
            return "";
        }
    }


    static GlobalConfigManager* instance() {
        if (!s_instance)
            s_instance = new GlobalConfigManager();
        return s_instance;
    }
    static void destroyInstance() {
        delete s_instance;
        s_instance = nullptr;
    }

    void destroy() {
        destroyInstance();
    }

    static void setConfigFilePath(const QString& path) { configPath = path; }

    void read() {
        settings = std::make_unique<QSettings>(configPath, QSettings::IniFormat);
    }

    void sync() {
        settings->sync();
    }

    QVariant query(const QString& key, const QVariant& defaultValue = QVariant()) const {
        return settings->value(key, defaultValue);
    }

    QVariant query(ConfigKey key) const;

    void write(const QString& key, const QVariant& value) {
        settings->setValue(key, value);
    }

    void write(ConfigKey key, const QVariant& value) {
        write(configKeyToString(key), value);
    }
};

