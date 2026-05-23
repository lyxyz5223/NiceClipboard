#pragma once
#include <QDialog>
#include "ui_SettingsWidget.h"
#include "SettingsUIFactory.h"
#include <QLabel>
#include <QKeySequenceEdit>


class SettingsWidget : public QDialog
{
    Q_OBJECT
public:
    SettingsWidget(QWidget* parent = nullptr);
    ~SettingsWidget() = default;

signals:
    void configChanged(const QString& key, const QVariant& value, const QVariant& oldValue);
    void hotKeyChanged(const QString& key, const QKeySequence& keySeq, const QKeySequence& oldKeySeq);
    void configSaved();

private slots:
    void onDialogButtonBoxClicked(QAbstractButton* button);

private:
    Ui::SettingsWidgetClass ui;

    QList<ISettingsUIWidget*> settingsUIWidgets;

    void saveConfig();
};

