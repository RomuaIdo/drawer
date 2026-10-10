#include "DrawerSize.h"

QString drawerSizeToString(DrawerSize size)
{
    switch (size) {
    case DrawerSize::Small: return "small";
    case DrawerSize::Medium: return "medium";
    case DrawerSize::Large: return "large";
    }
    return "medium";
}

DrawerSize drawerSizeFromString(const QString &text, DrawerSize fallback)
{
    for (DrawerSize size : {DrawerSize::Small, DrawerSize::Medium, DrawerSize::Large}) {
        if (text.compare(drawerSizeToString(size), Qt::CaseInsensitive) == 0)
            return size;
    }
    return fallback;
}
