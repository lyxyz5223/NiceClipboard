#include "GlobalConfig.h"
#include <QLayout>
#include <QLabel>
#include <QVariant>
#include "GlobalConfigManager.h"
#include <QString>
#include <QMessageBox>


#define SETTING_PAGE(id, name, items) \
    { id, QObject::tr(name), QIcon(), items }

#define SETTING_PAGE_EX(id, name, icon, items) \
    { id, QObject::tr(name), icon, items }

#define SETTING_ITEMS(...) __VA_ARGS__

#define SETTING_ITEM(key, type, name, description) \
    { GlobalConfigManager::configKeyToString(key), QObject::tr(name), type, QObject::tr(description), g_defaultSettings[GlobalConfigManager::configKeyToString(key)], g_defaultSettings[GlobalConfigManager::configKeyToString(key)] }

#define SETTING_ITEM_CB(key, type, name, description, callback, callbackData) \
    { GlobalConfigManager::configKeyToString(key), QObject::tr(name), type, QObject::tr(description), g_defaultSettings[GlobalConfigManager::configKeyToString(key)], g_defaultSettings[GlobalConfigManager::configKeyToString(key)], callback, callbackData }

#define SETTING_ITEM_EX(key, type, name, description, extraData) \
    { GlobalConfigManager::configKeyToString(key), QObject::tr(name), type, QObject::tr(description), g_defaultSettings[GlobalConfigManager::configKeyToString(key)], g_defaultSettings[GlobalConfigManager::configKeyToString(key)], SettingItem::ModifiedCallback(), QVariant(), extraData }

#define SETTING_ITEM_FULL(key, type, name, description, callback, callbackData, extraData) \
    { GlobalConfigManager::configKeyToString(key), QObject::tr(name), type, QObject::tr(description), g_defaultSettings[GlobalConfigManager::configKeyToString(key)], g_defaultSettings[GlobalConfigManager::configKeyToString(key)], callback, callbackData, extraData }

#define DEFAULT_SETTING_ITEM(key, value) \
    { GlobalConfigManager::configKeyToString(key), value }


const QMap<QString, QVariant> g_defaultSettings{
    DEFAULT_SETTING_ITEM(GlobalConfigManager::StartWithWindows, false),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::ShowInSystemTray, true),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::SavedDataFilePath, "./Data/Clipboard.ini"),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::SavedDataImagePath, "./Data/Images/"),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::ClearHistoryOnLoad, false),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::ClearHistoryOnQuit, false),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::ClipboardHistoryRetentionDays, 7),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::HistorySize, 100),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::SavedDataTimerInterval, 3000),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::MouseDoubleLeftClickInterval, 150),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::MouseDoubleMiddleClickInterval, 200),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::MouseDoubleRightClickInterval, 200),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::ShowWindowShortcut, QKeySequence{ QKeyCombination(Qt::AltModifier | Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_S) }),
    DEFAULT_SETTING_ITEM(GlobalConfigManager::QSSMainWindowStyle, QString("")),
};


QList<SettingPage> g_settingPages{ {
    SETTING_PAGE("regular", "常规", SETTING_ITEMS({
        SETTING_ITEM(GlobalConfigManager::StartWithWindows, SettingItem::CheckBox, "开机自启动", "是否开机自启动"),
        SETTING_ITEM(GlobalConfigManager::ShowInSystemTray, SettingItem::CheckBox, "显示在系统托盘", "是否在系统托盘显示图标"),
    })),
    SETTING_PAGE("advanced", "高级", SETTING_ITEMS({
        SETTING_ITEM_CB(GlobalConfigManager::SavedDataFilePath, SettingItem::LineEdit, "剪贴板数据保存路径", "剪贴板历史记录数据的保存路径",
        [](const SettingItem& item) {}, QVariant()),
        SETTING_ITEM(GlobalConfigManager::SavedDataImagePath, SettingItem::LineEdit, "剪贴板图片保存路径", "剪贴板图片的保存路径"),
        SETTING_ITEM(GlobalConfigManager::SavedDataTimerInterval, SettingItem::UInt32, "剪贴板数据保存间隔", "自动保存剪贴板历史记录数据的时间间隔（毫秒），每隔该时间间隔检查一次历史记录数据，若有变化则保存一次"),
        SETTING_ITEM(GlobalConfigManager::ClearHistoryOnLoad, SettingItem::CheckBox, "启动时自动清除历史记录", "启动软件时自动清除上次软件保存的剪贴板历史记录"),
        SETTING_ITEM(GlobalConfigManager::ClearHistoryOnQuit, SettingItem::CheckBox, "退出时自动清除历史记录", "退出软件时自动清除软件保存的剪贴板历史记录（软件崩溃除外）"),
        SETTING_ITEM(GlobalConfigManager::ClipboardHistoryRetentionDays, SettingItem::UInt32, "剪贴板历史记录保留天数", "剪贴板历史记录的最大保留天数，超过后会删除最旧的条目"),
        SETTING_ITEM(GlobalConfigManager::HistorySize, SettingItem::UInt32, "历史记录大小", "剪贴板历史记录的最大条目数，超过后会删除最旧的条目"),
        SETTING_ITEM(GlobalConfigManager::MouseDoubleLeftClickInterval, SettingItem::UInt32, "鼠标左键双击间隔", "鼠标左键双击的时间间隔（毫秒）"),
        SETTING_ITEM(GlobalConfigManager::MouseDoubleMiddleClickInterval, SettingItem::UInt32, "鼠标中键双击间隔", "鼠标中键双击的时间间隔（毫秒）"),
        SETTING_ITEM(GlobalConfigManager::MouseDoubleRightClickInterval, SettingItem::UInt32, "鼠标右键双击间隔", "鼠标右键双击的时间间隔（毫秒）"),
    })),
    SETTING_PAGE("shortcuts", "快捷键", SETTING_ITEMS({
        SETTING_ITEM(GlobalConfigManager::ShowWindowShortcut, SettingItem::KeySequence, "显示主窗口快捷键", "用于显示主窗口的快捷键"),
    })),
    SETTING_PAGE("uiCustomization", "界面定制", SETTING_ITEMS({
        // 这里是界面定制的设置项，例如主题选择、字体选择等
        SETTING_ITEM(GlobalConfigManager::QSSMainWindowStyle, SettingItem::PlainTextEdit, "主窗口样式表", "用于定制主窗口界面样式的QSS样式表文本"),
    })),
    SETTING_PAGE("about", "关于", SETTING_ITEMS({
        { ""/*这不是设置项，所以可以为空*/, QObject::tr("版本"), SettingItem::Custom, "软件版本", QVariant(), QVariant(), SettingItem::ModifiedCallback(), QVariant(),
            SettingItem::CustomExtraData{{}, nullptr, [](QWidget* settingPage) -> QWidget* {
                QWidget* page = new QWidget();
                page->setLayout(new QVBoxLayout());
                page->layout()->setContentsMargins(0, 0, 0, 0);
                page->layout()->setSpacing(0);
                QLabel* label = new QLabel("NiceClipboard v1.0.0", page);
                page->layout()->addWidget(label);
                return page;
            }}
        },
    })),
} };



void updateCurrentSettings()
{
    for (auto& page : g_settingPages)
    {
        for (auto& setting : page.settings)
        {
            if (setting.key.isEmpty()) continue;
            auto currentValue = GlobalConfigManager::instance()->query(setting.key, setting.defaultValue);
            setting.currentValue = currentValue;
        }
    }
}

