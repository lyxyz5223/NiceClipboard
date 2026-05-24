#include "SettingsWidget.h"
#include "GlobalConfig.h"

SettingsWidget::SettingsWidget(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    for (size_t index = 0; index < g_settingPages.size(); ++index)
    {
        if (index >= ui.tabWidget->count())
            ui.tabWidget->addTab(new QWidget(ui.tabWidget), g_settingPages[index].pageName);
        ui.tabWidget->setTabText(index, g_settingPages[index].pageName);
        ui.tabWidget->widget(index)->setObjectName(g_settingPages[index].id);
        ui.tabWidget->widget(index)->setWindowTitle(g_settingPages[index].pageName);
        ui.tabWidget->widget(index)->setWindowIcon(g_settingPages[index].icon);
        ui.tabWidget->widget(index)->setLayout(new QVBoxLayout());
        ui.tabWidget->widget(index)->layout()->setContentsMargins(0, 0, 0, 0);
        ui.tabWidget->widget(index)->layout()->setSpacing(0);
        auto* page = SettingsUIFactory::createWidget(g_settingPages[index].settings, ui.tabWidget->widget(index));
        settingsUIWidgets.append(page);
        ui.tabWidget->widget(index)->layout()->addWidget(page);
    }

}


void SettingsWidget::onDialogButtonBoxClicked(QAbstractButton* button)
{
    auto role = ui.buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::AcceptRole) // Ok
    {
        // 保存并应用设置，并退出设置窗口
        saveConfig();
        close();
    }
    else if (role == QDialogButtonBox::ApplyRole) // Apply
    {
        // 保存并应用设置
        saveConfig();
    }
    else/* if (role == QDialogButtonBox::RejectRole)*/ // Cancel
    {
        // 取消设置
        close();
    }
}

void SettingsWidget::saveConfig()
{
    //for (size_t idx = 0; idx < ui.tabWidget->count(); ++idx)
    for (size_t pageIndex = 0; pageIndex < g_settingPages.size(); ++pageIndex)
    {
        auto widget = settingsUIWidgets.at(pageIndex);
        for (size_t settingIndex = 0; settingIndex < g_settingPages[pageIndex].settings.size(); ++settingIndex)
        {
            auto& item = widget->settings()[settingIndex];
            if (item.key.isEmpty()) continue;
            QVariant oldVal = g_settingPages[pageIndex].settings[settingIndex].currentValue;
            if (item.currentValue != oldVal)
            {
                emit configChanged(item.key, item.currentValue, oldVal);
                g_settingPages[pageIndex].settings[settingIndex].currentValue = item.currentValue;
            }
            if (item.currentValue.canConvert<QKeySequence>()
                && item.currentValue.userType() == QMetaType::QKeySequence)
            {
                auto curKeySeq = item.currentValue.value<QKeySequence>();
                auto oldKeySeq = oldVal.value<QKeySequence>();
                if (curKeySeq != oldKeySeq)
                    emit hotKeyChanged(item.key, curKeySeq, oldKeySeq);
            }
        }
    }
    emit configSaved();
}