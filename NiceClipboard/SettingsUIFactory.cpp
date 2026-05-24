#include "SettingsUIFactory.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QKeySequenceEdit>
#include <QRadioButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>

#define setupValue(widget, setValueFunc, targetType) \
    if (item.currentValue.canConvert<targetType>()) \
        widget->setValueFunc(item.currentValue.value<targetType>()); \
    else if (item.defaultValue.canConvert<targetType>()) \
        widget->setValueFunc(item.defaultValue.value<targetType>());



void ISettingsUIWidget::setupUI(const SettingItem& item)
{
    switch (item.type)
    {
    case SettingItem::UInt32:
    case SettingItem::Int32:
    {
        auto* spinBox = new QSpinBox(this);
        spinBox->setMaximum(std::numeric_limits<int>::max());
        if (item.type == SettingItem::UInt32)
            spinBox->setMinimum(0);
        else
            spinBox->setMinimum(std::numeric_limits<int>::min());
        setupProperties(spinBox, item);
        if (item.type == SettingItem::UInt32)
        {
            if (spinBox->minimum() < 0)
                spinBox->setMinimum(0);
            if (spinBox->maximum() < 0)
                spinBox->setMaximum(std::numeric_limits<int>::max());
        }
        setupValue(spinBox, setValue, int);
        connect(spinBox, &QSpinBox::valueChanged, this, [this, key=item.key](int value) { setSetting(key, value); });
        addWidgetWithLabel(item, spinBox);
        break;
    }
    case SettingItem::Double:
    {
        auto* spinBox = new QDoubleSpinBox(this);
        setupProperties(spinBox, item);
        setupValue(spinBox, setValue, double);
        connect(spinBox, &QDoubleSpinBox::valueChanged, this, [this, key = item.key](double value) { setSetting(key, value); });
        addWidgetWithLabel(item, spinBox);
        break;
    }
    case SettingItem::CheckBox:
    {
        auto* checkBox = new QCheckBox(item.name, this);
        checkBox->setTristate(false);
        setupProperties(checkBox, item);
        setupValue(checkBox, setChecked, bool);
        connect(checkBox, &QCheckBox::stateChanged, this, [this, key = item.key](int value) { setSetting(key, value); });
        addWidgetWithoutLabel(item, checkBox);
        break;
    }
    case SettingItem::RadioButton:
    {
        auto* radioButton = new QRadioButton(item.name, this);
        setupProperties(radioButton, item);
        setupValue(radioButton, setChecked, int);
        connect(radioButton, &QRadioButton::toggled, this, [this, key = item.key](bool checked) { setSetting(key, checked); });
        addWidgetWithoutLabel(item, radioButton);
        break;
    }
    case SettingItem::LineEdit:
    {
        auto* edit = new QLineEdit(this);
        setupProperties(edit, item);
        setupValue(edit, setText, QString);
        //connect(edit, &QLineEdit::editingFinished, this, [this, key = item.key, edit]() { setSetting(key, edit->text()); });
        connect(edit, &QLineEdit::textChanged, this, [this, key = item.key](const QString& text) { setSetting(key, text); });
        addWidgetWithLabel(item, edit);
        break;
    }
    case SettingItem::TextEdit:
    {
        auto* edit = new QTextEdit(this);
        setupProperties(edit, item);
        setupValue(edit, setText, QString);
        connect(edit, &QTextEdit::textChanged, this, [this, key = item.key, edit]() { setSetting(key, edit->toHtml()); });
        addWidgetWithLabel(item, edit);
        break;
    }
    case SettingItem::PlainTextEdit:
    {
        auto* edit = new QPlainTextEdit(this);
        setupProperties(edit, item);
        setupValue(edit, setPlainText, QString);
        connect(edit, &QPlainTextEdit::textChanged, this, [this, key = item.key, edit]() { setSetting(key, edit->toPlainText()); });
        addWidgetWithLabel(item, edit);
        break;
    }
    case SettingItem::ComboBox:
    {
        QComboBox* comboBox = new QComboBox(this);
        if (std::holds_alternative<SettingItem::ComboBoxExtraData>(item.extraData))
        {
            auto& comboBoxData = std::get<SettingItem::ComboBoxExtraData>(item.extraData);
            for (const auto& option : comboBoxData.options)
            {
                comboBox->addItem(option);
            }
        }
        setupProperties<SettingItem::ComboBoxExtraData>(comboBox, item);
        if (item.currentValue.userType() == QMetaType::QString || item.defaultValue.userType() == QMetaType::QString)
        {
            setupValue(comboBox, setCurrentText, QString);
        }
        else
        {
            setupValue(comboBox, setCurrentIndex, int);
        }
        //connect(comboBox, &QComboBox::currentTextChanged, this, [this, key = item.key](const QString& text) { setSetting(key, text); });
        connect(comboBox, &QComboBox::currentIndexChanged, this, [this, key = item.key](int index) { setSetting(key, index); });
        addWidgetWithLabel(item, comboBox);
        break;
    }
    case SettingItem::Slider:
    {
        QSlider* slider = new QSlider(this);
        setupProperties(slider, item);
        setupValue(slider, setValue, int);
        connect(slider, &QSlider::valueChanged, this, [this, key = item.key](int value) { setSetting(key, value); });
        addWidgetWithLabel(item, slider);
        break;
    }
    case SettingItem::KeySequence:
    {
        auto* keySeq = new QKeySequenceEdit(this);
        setupProperties(keySeq, item);
        auto v = item.currentValue.value<QKeySequence>().toString();
        setupValue(keySeq, setKeySequence, QKeySequence);
        connect(keySeq, &QKeySequenceEdit::keySequenceChanged, this, [this, key = item.key](const QKeySequence& seq) { setSetting(key, seq); });
        addWidgetWithLabel(item, keySeq);
    }
        break;
    case SettingItem::Custom:
        if (std::holds_alternative<SettingItem::CustomExtraData>(item.extraData))
        {
            auto& customData = std::get<SettingItem::CustomExtraData>(item.extraData);
            if (customData.widget)
            {
                setupProperties<SettingItem::CustomExtraData>(customData.widget, item);
                addWidgetWithoutLabel(item, customData.widget);
            }
            else if (customData.creator)
            {
                auto* l = customData.creator(this);
                setupProperties<SettingItem::CustomExtraData>(l, item);
                addWidgetWithoutLabel(item, l);
            }
        }
        break;
    default:
        break;
    }
}

void ISettingsUIWidget::addWidgetWithLabel(const SettingItem& item, QWidget* widget)
{
    auto* label = new QLabel(item.name, this);
    label->setEnabled(item.enabled);
    widget->setEnabled(item.enabled);
    QHBoxLayout* hl = new QHBoxLayout();
    hl->addWidget(label);
    hl->addWidget(widget);
    layout()->addItem(hl);
}

void ISettingsUIWidget::addWidgetWithoutLabel(const SettingItem& item, QWidget* widget)
{
    widget->setEnabled(item.enabled);
    layout()->addWidget(widget);
}

template <typename ExtraDataType>
void ISettingsUIWidget::setupProperties(QObject* object, const SettingItem& item)
{
    if (std::holds_alternative<ExtraDataType>(item.extraData))
    {
        auto& data = std::get<ExtraDataType>(item.extraData);
        for (auto it = data.properties.begin(); it != data.properties.end(); ++it)
            object->setProperty(it.key().toUtf8(), it.value());
    }
}

void ISettingsUIWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    auto& s = settings();
    for (auto& item : s)
        setupUI(item);
    mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}