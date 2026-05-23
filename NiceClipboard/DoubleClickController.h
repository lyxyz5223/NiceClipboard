#pragma once
#include <QWidget>
#include <QTimer>

/// <summary>
/// Usage:
/// detectDoubleClick();
/// connect();
/// start();
/// 没必要stop();
/// </summary>
class DoubleClickController
{
    using ClickCallback = std::function<void()>;
    ClickCallback clickedCallback;
    //ClickCallback dbClickedCallback;
    QObject* parent{ nullptr };
    QTimer* timer{ nullptr };
public:
    DoubleClickController(QObject* parent = nullptr) : parent(parent) {
        timer = new QTimer(parent);
        timer->setSingleShot(true);
        timer->setInterval(200);
    }
    ~DoubleClickController() = default;

    void setParent(QObject* newParent) {
        parent = newParent;
        timer->setParent(newParent);
    }

    void setInterval(int msec) {
        timer->setInterval(msec);
    }

    // 通过调用此函数来检测并调用双击回调
    // 如果调用了回调，则返回true，尽管回调为nullptr，仍然返回true
    bool detectDoubleClick(ClickCallback callback) {
        if (timer->isActive())
        {
            timer->stop();
            if (callback)
                callback();
            return true;
        }
        return false;
    }

    // 通过QTimer的回调超时未进行双击事件自动调用单击事件
    void connect(ClickCallback callback) {
        clickedCallback = callback;
        timer->disconnect(timer, SIGNAL(timeout()), nullptr, nullptr);
        timer->connect(timer, &QTimer::timeout, parent, [this]() {
            if (clickedCallback)
                clickedCallback();
        });
    }

    //void setClickedCallback(ClickCallback callback) {
    //    clickedCallback = callback;
    //}

    //void setDoubleClickedCallback(ClickCallback callback) {
    //    dbClickedCallback = callback;
    //}

    void start() {
        timer->start();
    }

    void stop() {
        timer->stop();
    }
};

