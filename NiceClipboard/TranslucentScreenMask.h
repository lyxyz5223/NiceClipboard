#pragma once
#include <QWidget>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <Windows.h>

class TranslucentScreenMask : public QWidget
{
    Q_OBJECT
public:
    TranslucentScreenMask(QWidget* parent = nullptr) : QWidget(parent) {
        this->setAttribute(Qt::WA_TranslucentBackground);
        this->setAttribute(Qt::WA_ShowWithoutActivating);
        this->setAttribute(Qt::WA_AlwaysStackOnTop);
        this->setAttribute(Qt::WA_QuitOnClose);
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool | //Qt::CustomizeWindowHint |
            Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint
            | Qt::WindowOverridesSystemGestures
        );
        this->setWindowModality(Qt::NonModal); // 非模态窗口
        this->setFocusPolicy(Qt::NoFocus);
        // 取消激活窗口
        auto hwndThisWindow = reinterpret_cast<HWND>(this->winId());
        SetWindowLong(hwndThisWindow, GWL_EXSTYLE, GetWindowLong(hwndThisWindow, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
        SetWindowLong(hwndThisWindow, GWL_EXSTYLE, GetWindowLong(hwndThisWindow, GWL_EXSTYLE) & ~(WS_EX_WINDOWEDGE));

        setGeometry(QGuiApplication::primaryScreen()->geometry());
    }

    void setBackgroundColor(QColor color) {
        m_bkgColor = color;
    }

    QColor getBackgroundColor() const {
        return m_bkgColor;
    }
protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setPen(Qt::NoPen);
        painter.fillRect(rect(), m_bkgColor); // 半透明黑色遮罩
    }
    void mousePressEvent(QMouseEvent* event) override {
        
    }
    void mouseReleaseEvent(QMouseEvent* event) override {
        emit clicked();
    }
signals:
    void clicked();
private:
    QColor m_bkgColor{ 0, 0, 0, 1 };
};

