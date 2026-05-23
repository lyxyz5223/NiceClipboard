#include "ColoredSvgIcon.h"
#include <QIODevice>
#include <QFile>
#include <QSvgRenderer>
#include <QPainter>

QIcon ColoredSvgIcon::makeColoredIcon(const QString& svgPath, const QColor& color)
{
    QFile file(svgPath);
    if (!file.open(QIODevice::ReadOnly)) return QIcon();
    QByteArray svgData = file.readAll();

    // 把SVG里的"currentColor"替换成目标颜色值
    svgData.replace("currentColor", color.name(QColor::HexRgb).toUtf8());

    QSvgRenderer renderer(svgData);
    QPixmap pixmap(24, 24); // 指定图标尺寸
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    return QIcon(pixmap);
}


