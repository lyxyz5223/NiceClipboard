#pragma once
#include <QKeySequenceEdit>

class HotKeyEdit : public QKeySequenceEdit
{
    Q_OBJECT
public:
    HotKeyEdit(QWidget* parent = nullptr) : QKeySequenceEdit(parent) {}
protected:
    void keyPressEvent(QKeyEvent* event) override;
};

