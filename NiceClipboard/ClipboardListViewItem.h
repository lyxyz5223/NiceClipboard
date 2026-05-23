#pragma once
#include "ui_ClipboardListViewItemWidget.h"
#include "ClipboardItemData.h"
#include <QStyledItemDelegate>
#include <QAbstractListModel>
#include <QListWidget>

class ClipboardListViewItemWidgetContentsLabel : public QLabel {
    QImage image;
    void adjustPixmap() {
        if (!image.isNull())
            setPixmap(QPixmap::fromImage(image).scaled(size(), Qt::KeepAspectRatio));
    }
public:
    explicit ClipboardListViewItemWidgetContentsLabel(QWidget* parent = nullptr) : QLabel(parent) {}
    void setImage(const QImage& img) {
        image = img;
        adjustPixmap();
    }
protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        adjustPixmap();
    }
};

class ClipboardListItemWidget : public QWidget
{
    Q_OBJECT
private:
    Ui::ClipboardListViewItemWidget ui;

public:
    ClipboardListItemWidget(QWidget* parent = nullptr)
        : QWidget(parent) {
        ui.setupUi(this);
    }
    ~ClipboardListItemWidget() {}

    void setShowMenuFunction(std::function<void(QPoint pos)> showMenuFunc) {
        connect(ui.btnOptions, &QPushButton::clicked, this, [this, showMenuFunc] {
            QPoint menuPos = ui.btnOptions->mapToGlobal(QPoint(ui.btnOptions->width() / 2, ui.btnOptions->height() / 2));
            showMenuFunc(menuPos);
            });
    }

    void setPinToTopFunction(std::function<void()> pinToTopFunc) {
        connect(ui.btnPin, &QPushButton::clicked, this, [this, pinToTopFunc] {
            pinToTopFunc();
            });
    }
    void updateUiClipboardItemData(const ClipboardItemData& itemData) {
        if (itemData.hasImage())
        {
            ui.labelContents->setScaledContents(false);
            ui.labelContents->setPixmap(QPixmap::fromImage(itemData.getImage()).scaled(ui.labelContents->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            //ui.labelContents->setImage(itemData.getImage());
        }
        else if (itemData.hasHtml())
        {
            ui.labelContents->setText(itemData.getHtml());
        }
        else if (itemData.hasText())
        {
            ui.labelContents->setText(itemData.getText());
        }
    }
};


class ClipboardListItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ClipboardListItemModel() {

    }
    void setClipboardItemData(const QModelIndex& index, const ClipboardItemData& item) {
        setData(index, item, Qt::DisplayRole);
    }

    ClipboardItemData getClipboardItemData(const QModelIndex& index) const {
        return data(index, Qt::DisplayRole).value<ClipboardItemData>();
    }

    virtual bool setData(const QModelIndex& index, const ClipboardItemData& item, Qt::ItemDataRole role) {
        if (index.row() < 0 || index.row() >= m_items.size())
            return false;
        m_items.replace(index.row(), item);
        return true;
    }

    virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override {
        if (value.canConvert<ClipboardItemData>())
            return setData(index, value.value<ClipboardItemData>(), static_cast<Qt::ItemDataRole>(role));
        return false;
    }

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (index.row() < 0 || index.row() >= m_items.size())
            return QVariant();
        auto& item = m_items.at(index.row());
        return QVariant::fromValue(item);
    }

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return m_items.size();
    }

private:
    QList<ClipboardItemData> m_items;
};


class ClipboardListItem : public QListWidgetItem
{
    ClipboardListItemWidget* m_itemWidget = nullptr;
public:
    explicit ClipboardListItem(QListWidget* listview = nullptr)
        : QListWidgetItem(listview) {
        m_itemWidget = new ClipboardListItemWidget(listview);
    }
    ClipboardListItem(const ClipboardListItem& other) : QListWidgetItem(other), m_itemWidget(other.m_itemWidget) {}

    void setClipboardItemData(const ClipboardItemData& item) {
        setData(Qt::DisplayRole, QVariant::fromValue(item));
        m_itemWidget->updateUiClipboardItemData(item);
    }
    ClipboardItemData&& getClipboardItemData() {
        return data(Qt::DisplayRole).value<ClipboardItemData>();
    }
    void setShowMenuFunction(std::function<void(QPoint pos)> showMenuFunc) {
        m_itemWidget->setShowMenuFunction(showMenuFunc);
    }

    void setPinToTopFunction(std::function<void()> pinToTopFunc) {
        m_itemWidget->setPinToTopFunction(pinToTopFunc);
    }
    QWidget* itemWidget() { return m_itemWidget; }

};

class ClipboardListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    // painting
    void paint(QPainter* painter,
        const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;

    constexpr static int ITEM_PADDING = 7;
};

