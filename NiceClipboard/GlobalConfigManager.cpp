#include "GlobalConfigManager.h"
#include "GlobalConfig.h"
#include <QDir>

GlobalConfigManager* GlobalConfigManager::s_instance{ nullptr };
QString GlobalConfigManager::configPath{
    QDir("./Configs/").absoluteFilePath(GlobalConfigManager::configFileName)
};


QVariant GlobalConfigManager::query(ConfigKey key) const
{
    QString configKeyStr = configKeyToString(key);
    if (configKeyStr.isEmpty())
        return QVariant();
    return query(configKeyStr, g_defaultSettings.value(configKeyStr));
}

