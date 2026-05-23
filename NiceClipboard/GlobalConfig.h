#pragma once
#include <QList>
#include "SettingsUIFactory.h"

struct SettingPage {
    QString id; // 设置项所属页面的唯一标识，必须唯一
    QString pageName; // 设置项所属页面的名称，例如“常规”、“高级”、“快捷键”、“关于”等
    QIcon icon; // 设置项所属页面的图标，可以为空
    QList<SettingItem> settings; // 该页面包含的设置项列表
    //std::function<void(const QList<SettingItem>&)> onPageModified; // 页面内任一设置项被修改后的回调函数，例如用于启用“应用”按钮等
};


extern const QMap<QString, QVariant> g_defaultSettings;
extern QList<SettingPage> g_settingPages;



void updateCurrentSettings();
