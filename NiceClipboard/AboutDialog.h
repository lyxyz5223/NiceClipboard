#pragma once
#include <QDialog>
#include "ui_AboutWidget.h"

class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    AboutDialog(QWidget* parent = nullptr);

private:
    Ui::AboutWidget ui;
};

