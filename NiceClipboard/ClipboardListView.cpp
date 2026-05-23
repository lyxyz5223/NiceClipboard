#include "ClipboardListView.h"
#include <QPainter>
#include <QTextDocument>
#include <QScrollBar>
#include <QMessageBox>
#include "ClipboardListViewItem.h"

ClipboardListView::ClipboardListView(QWidget* parent)
    : QListWidget(parent)
{
    setItemDelegate(new ClipboardListItemDelegate(this));
    auto vScrollBar = verticalScrollBar();
    vScrollBar->connect(vScrollBar, &QScrollBar::valueChanged, this, &ClipboardListView::sliderValueChanged);
    scrollAnim = new QPropertyAnimation(vScrollBar, "value");
    scrollAnim->setEasingCurve(QEasingCurve::OutCubic);

}

void ClipboardListView::wheelEvent(QWheelEvent* e)
{
    auto scrollMode = verticalScrollMode();
    if (scrollMode == ScrollPerItem)
    {
        QListView::wheelEvent(e);
        return;
    }
    auto singleStep = verticalScrollBar()->singleStep();
    auto scrollAngle = e->angleDelta().y();
    if ((accumulatedScrollAngleDelta > 0 && scrollAngle < 0) || (accumulatedScrollAngleDelta < 0 && scrollAngle > 0))
        accumulatedScrollAngleDelta = 0;
    if (scrollAnim->state() == QAbstractAnimation::Stopped && std::abs(accumulatedScrollAngleDelta) >= 120)
        accumulatedScrollAngleDelta = 0;
    accumulatedScrollAngleDelta += scrollAngle;
    if (std::abs(accumulatedScrollAngleDelta) >= 120)
    {
        if (scrollAnim->state() == QAbstractAnimation::Running)
        {
            scrollAnim->stop();
            accumulatedScrollAngleDelta -= scrollAnim->currentValue().toInt() - scrollAnim->startValue().toInt();
        }
        scrollAnim->setDuration(300 + std::abs(accumulatedScrollAngleDelta) * 25 / 120);
        auto curValue = verticalScrollBar()->value();
        scrollAnim->setStartValue(curValue);
        //if (scrollMode == ScrollPerPixel)
        scrollAnim->setEndValue(curValue - static_cast<long long>(accumulatedScrollAngleDelta) * singleStep / 120);
        //else
        //    scrollAnim->setEndValue(calcScrollBarValueFromModelIndex(calcModelIndexFromScrollBarValue(curValue) - singleStep * static_cast<long long>(accumulatedScrollAngleDelta) / 120));
        if (scrollAnim->state() == QAbstractAnimation::Stopped)
            scrollAnim->start();
    }
    //Logger("ClipboardListView").info("singleStep: {}, startValue: {}, endValue: {}, totalScrollAngleDelta: {}, scrollAngle: {}, pixel: {}", singleStep, scrollAnim->startValue().toInt(), scrollAnim->endValue().toInt(), accumulatedScrollAngleDelta, scrollAngle, scrollAngle / 15 * verticalScrollBar()->singleStep());
}

void ClipboardListView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MouseButton::LeftButton)
    {
        //Logger("ClipboardListView").info("Mouse left button pressed.");
    }
    if (e->button() == Qt::MouseButton::RightButton)
    {
        //Logger("ClipboardListView").info("Mouse right button pressed.");
        //return; // 过滤掉右键消息，防止鼠标右键双击触发itemDbClicked信号
    }
    QListWidget::mousePressEvent(e);
}

void ClipboardListView::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MouseButton::LeftButton)
    {
        auto item = itemAt(e->pos());
        if (item)
            emit leftClicked(item, e->pos());
        //else
        //    QMessageBox::warning(this, tr("警告"), tr("该位置没有找到列表项"));
    }
    if (e->button() == Qt::MouseButton::RightButton)
    {
        auto item = itemAt(e->pos());
        if (item)
            emit rightClicked(item, e->pos());
        //else
        //    QMessageBox::warning(this, tr("警告"), tr("该位置没有找到列表项"));
    }
    QListWidget::mouseReleaseEvent(e);
}

int ClipboardListView::calcModelIndexFromScrollBarValue(int scrollBarValue)
{
    int targetIndex = -1;
    size_t totalHeight = 0;
    for (int i = 0; i < count(); ++i)
    {
        int itemHeight = sizeHintForRow(i);
        totalHeight += itemHeight;
        if (totalHeight > scrollBarValue)
        {
            targetIndex = i;
            break;
        }
    }
    return targetIndex;
}

int ClipboardListView::calcScrollBarValueFromModelIndex(int index)
{
    int targetValue = 0;
    for (int i = 0; i < index; ++i)
        targetValue += sizeHintForRow(i);
    return targetValue;
}


void ClipboardListView::sliderValueChanged(int value)
{
}

