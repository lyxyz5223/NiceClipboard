#pragma once
#include <QListWidget>
#include <QListView>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QResizeEvent>
#include <QMenu>
#include "AnimatedMenu.h"
#include <Logger.h>
#include "ClipboardItemData.h"


class ClipboardListView : public QListWidget
{
    Q_OBJECT
public:
    explicit ClipboardListView(QWidget* parent = nullptr);
    ~ClipboardListView() {}

signals:
    void leftClicked(QListWidgetItem* item, const QPoint& pos);
    void rightClicked(QListWidgetItem* item, const QPoint& pos);
    //void leftDoubleClicked(ClipboardListItem* item);
    //void rightDoubleClicked(ClipboardListItem* item);

protected:
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private slots:
    void sliderValueChanged(int value);
private:
    QPropertyAnimation* scrollAnim{ nullptr };
    //int scrollAnimStartValue{ 0 };
    int accumulatedScrollAngleDelta{ 0 };
    //Logger logger{ "ClipboardListView" };

    int calcModelIndexFromScrollBarValue(int scrollBarValue);
    int calcScrollBarValueFromModelIndex(int index);

};

