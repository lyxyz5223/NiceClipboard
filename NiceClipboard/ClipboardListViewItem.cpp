#include "ClipboardListViewItem.h"
#include <QPainter>

void ClipboardListItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QStyledItemDelegate::paint(painter, option, index);
    QRect rect = option.rect;
    rect.adjust(ITEM_PADDING, ITEM_PADDING, -ITEM_PADDING, -ITEM_PADDING);
    QVariant variant = index.data(Qt::DisplayRole);
    ClipboardItemData itemData = variant.value<ClipboardItemData>();
    if (itemData.hasImage())
    {
        QRect imageRect(rect.topLeft(), itemData.image.size());
        double imageAspectRatio = static_cast<double>(itemData.image.width()) / itemData.image.height();
        double rectAspectRatio = static_cast<double>(rect.width()) / rect.height();
        if (imageAspectRatio >= rectAspectRatio)
        {
            imageRect.setWidth(rect.width());
            imageRect.setHeight(static_cast<int>(rect.width() / imageAspectRatio));
        }
        else
        {
            imageRect.setHeight(rect.height());
            imageRect.setWidth(static_cast<int>(rect.height() * imageAspectRatio));
        }
        painter->drawImage(imageRect, itemData.image);
    }
    else if (itemData.hasHtml())
    {
        painter->save();
        painter->translate(rect.topLeft());
        QTextDocument* textDoc = itemData.getCachedHtmlTextDocument();
        if (textDoc && !textDoc->isEmpty())
            textDoc->drawContents(painter, QRect(0, 0, rect.width(), rect.height()));
        painter->restore();
    }
    else if (itemData.hasText())
    {
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, itemData.text);
    }
}

QSize ClipboardListItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    //constexpr int textLineCount = 3;
    //auto height = option.fontMetrics.height();
    //auto leading = option.fontMetrics.leading() < 3 ? option.fontMetrics.height() / 2 : option.fontMetrics.leading();
    //return QSize(size.width(), height * textLineCount + (textLineCount - 1) * leading);
    size = size.expandedTo(QSize(0, 80));
    auto widgetSize = option.widget->size();
    size = size.expandedTo(QSize(0, widgetSize.width() / 4));
    return size; // 确保每个项至少有100像素的高度
}


