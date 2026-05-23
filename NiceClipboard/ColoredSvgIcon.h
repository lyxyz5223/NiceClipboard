#pragma once
#include <QIcon>

struct ColoredSvgIcon
{
    static QIcon makeColoredIcon(const QString& svgPath, const QColor& color);
};