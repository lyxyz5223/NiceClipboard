#pragma once
#include <QCheckBox>

class CheckBoxWithUserData : public QCheckBox
{
    Q_OBJECT
public:
    using QCheckBox::QCheckBox;

    void setUserData(const QVariant& data) { userData = data; }
    void setUserData(QVariant&& data) { userData = std::move(data); }
    const QVariant& getUserData() const { return userData; }

private:
    QVariant userData;
};

