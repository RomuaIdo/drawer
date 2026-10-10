#pragma once

#include <QString>

enum class DrawerSize { Small, Medium, Large };

QString drawerSizeToString(DrawerSize size);
DrawerSize drawerSizeFromString(const QString &text, DrawerSize fallback = DrawerSize::Medium);
