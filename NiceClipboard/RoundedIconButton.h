#pragma once
#include <QPushButton>
#include <QPen>
#include <QBrush>
#include <QPainterPath>

class RoundedIconButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor DESIGNABLE true)

public:
    RoundedIconButton(QWidget* parent = nullptr);
    ~RoundedIconButton();

    QColor iconColor() const {
        return m_iconColor;
    }
    void setIconColor(const QColor& color);


    void setText(const QString& text) {

    }
    QString text() const {

    }
    bool hitTest(const QPoint& pos) const;

protected:
    virtual void paintEvent(QPaintEvent*) override;
    virtual void resizeEvent(QResizeEvent* e) override;
    virtual bool hitButton(const QPoint& pos) const override;
    virtual void mouseMoveEvent(QMouseEvent* e) override;
    virtual void mousePressEvent(QMouseEvent* e) override;
    virtual void mouseReleaseEvent(QMouseEvent* e) override;
    virtual void enterEvent(QEnterEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;
    
signals:

public slots:

private:
    QColor m_iconColor{ Qt::black };
    bool mousePressed{ false };
    bool mouseHovered{ false };

    QPixmap iconPixmap{ icon().pixmap(iconSize(), QIcon::Mode::Normal, QIcon::State::Off)};
    qreal xRadius = qMin(width(), height()) / 2.0;
    qreal yRadius = qMin(width(), height()) / 2.0;
    Qt::SizeMode roundMode = Qt::AbsoluteSize;
    QPainterPath roundedBtnPath;
    QPixmap iconToColoredPixmap(const QIcon& icon, QSize size, QColor color);
};

