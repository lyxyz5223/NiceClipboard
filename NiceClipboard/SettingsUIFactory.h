#pragma once
#include <QWidget>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <functional>
#include <Logger.h>

struct SettingItem
{
    enum SettingType
    {
        UInt32,
        Int32, // int
        //UInt64,
        //Int64, // long long
        //Float,
        Double,
        CheckBox,
        RadioButton,
        //RadioButtonGroup,
        LineEdit,
        TextEdit,
        PlainTextEdit,
        ComboBox,
        Slider,
        KeySequence, // QKeySequence，快捷键编辑器
        Shortcut = KeySequence,
        Custom, // 需要自定义UI，无法通过简单的类型区分
    };

    struct BasicExtraData {
        QMap<QString, QVariant> properties; // 用于存储Qt控件的属性，例如QCheckBox的tristate属性，QLineEdit的placeholderText属性等
    };


    struct ComboBoxExtraData : public BasicExtraData {
        QStringList options;
    };

    struct CustomExtraData : public BasicExtraData {
        QWidget* widget = nullptr; // 需要用户自己创建布局和控件，并将它们添加到布局中
        std::function<QWidget*(QWidget* settingPage)> creator;
        //CustomExtraData() = default;
        //using BasicExtraData::BasicExtraData;
        //CustomExtraData(QLayout* layout) : layout(layout) {}
        //CustomExtraData(const std::function<QLayout*()>& creator) : creator(creator) {}
    };

    //union ExtraData {
    //    ComboBoxExtraData comboBox;
    //    SliderExtraData slider;
    //    CustomExtraData custom;
    //    ExtraData() {}
    //    ~ExtraData() {}
    //    ExtraData(const ComboBoxExtraData& data) : comboBox(data) {}
    //    ExtraData(const SliderExtraData& data) : slider(data) {}
    //    ExtraData(const CustomExtraData& data) : custom(data) {}
    //    void copy(SettingType type, const ExtraData& other) {
    //        if (type == ComboBox)
    //            comboBox.options = other.comboBox.options;
    //        else if (type == Slider)
    //        {
    //            slider.min = other.slider.min;
    //            slider.max = other.slider.max;
    //            slider.singleStep = other.slider.singleStep;
    //            slider.pageStep = other.slider.pageStep;
    //        }
    //        else if (type == Custom)
    //            custom.layout = other.custom.layout;
    //    }
    //};
    
    using ModifiedCallback = std::function<void(const SettingItem& item)>;
    using ExtraData = std::variant<BasicExtraData, ComboBoxExtraData, CustomExtraData>;

    QString key; // 用于唯一标识设置项，必须唯一
    QString name;
    SettingType type;
    QString description;
    QVariant defaultValue;
    QVariant currentValue;
    ModifiedCallback modifiedCallback;
    QVariant modifiedCallbackData; // 修改后数据项处理函数的额外数据，例如需要传递给处理函数的参数等
    ExtraData extraData;
    ~SettingItem() noexcept {

    }
    SettingItem() = default;
    SettingItem(QString key, QString name, SettingType type, QString description,
        QVariant defaultValue, QVariant currentValue,
        ModifiedCallback modifiedCallback = ModifiedCallback(), QVariant modifiedCallbackData = QVariant(), ExtraData extraData = ExtraData{})
        : key(std::move(key)), name(std::move(name)), type(type), description(std::move(description))
        , defaultValue(defaultValue), currentValue(currentValue)
        , modifiedCallback(modifiedCallback), modifiedCallbackData(modifiedCallbackData), extraData(extraData) {
        //this->extraData.copy(type, extraData);
    }
    SettingItem(const SettingItem& other) : key(other.key), name(other.name), type(other.type), description(other.description)
        , defaultValue(other.defaultValue), currentValue(other.currentValue)
        , modifiedCallback(other.modifiedCallback), modifiedCallbackData(other.modifiedCallbackData), extraData(other.extraData) {
        //extraData.copy(type, other.extraData);
    }
    SettingItem& operator=(const SettingItem& other) {
        key = other.key;
        name = other.name;
        type = other.type;
        description = other.description;
        defaultValue = other.defaultValue;
        currentValue = other.currentValue;
        modifiedCallback = other.modifiedCallback;
        modifiedCallbackData = other.modifiedCallbackData;
        //extraData.copy(type, other.extraData);
        extraData = other.extraData;
        return *this;
    }
    SettingItem(SettingItem&& other) = default;
};

class ISettingsUIWidget : public QWidget {
    Q_OBJECT
private:
    void setupUI(const SettingItem& item);
    template<typename ExtraDataType = SettingItem::BasicExtraData>
    void setupProperties(QObject* object, const SettingItem& item);
    void addWidgetWithLabel(const QString& labelText, QWidget* widget);

protected:
    //void closeEvent(QCloseEvent* event) override { }

    //virtual QList<SettingItem>& modifiedSettings() = 0;

public:

    ISettingsUIWidget(QWidget* parent = nullptr) : QWidget(parent) {}

    virtual ~ISettingsUIWidget() = default;

    virtual void setupUI();



    /// <summary>
    /// 获取设置项列表，类似于列表的data函数
    /// </summary>
    /// <returns>设置项列表</returns>
    virtual const QList<SettingItem>& settings() const = 0;
    virtual void setSetting(const QString& key, const QVariant& value) = 0;
    /// <summary>
    /// 有些设置项需要一些特殊操作，比如需要重启软件，因此需要一个回调函数来通知外部进行这些操作，例如重启软件，或者提示用户重启软件等
    /// </summary>
    virtual void modifiedCallback(const SettingItem& item) = 0;

};

class FactorySettingsUIWidget : public ISettingsUIWidget
{
    Q_OBJECT

public:
    FactorySettingsUIWidget(QList<SettingItem>& settings, QWidget* parent = nullptr) : m_settings(settings), ISettingsUIWidget(parent)  {
        setupUI();
    }
    ~FactorySettingsUIWidget() override {

    }
    virtual const QList<SettingItem>& settings() const override {
        return m_settings;
    }
    virtual void setSetting(const QString& key, const QVariant& value) override {
        for (auto& item : m_settings)
        {
            if (item.key == key)
            {
                QString v = value.value<QKeySequence>().toString();
                if (item.currentValue != v)
                    modifiedCallback(item);
                item.currentValue = value;
                break;
            }
        }
    }

    virtual void modifiedCallback(const SettingItem& item) override {
        if (item.modifiedCallback)
            item.modifiedCallback(item);
    }

private:
    QList<SettingItem> m_settings;
};

struct SettingsUIFactory
{
    static ISettingsUIWidget* createWidget(QList<SettingItem>& settings, QWidget* parent = nullptr) {
        return new FactorySettingsUIWidget(settings, parent);
    }
};

