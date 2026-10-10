#pragma once

#include <QComboBox>

#include "core/DrawerSize.h"

// Combo box listing the drawer sizes.
class SizeComboBox : public QComboBox
{
public:
    explicit SizeComboBox(DrawerSize current, QWidget *parent = nullptr)
        : QComboBox(parent)
    {
        addItem(tr("Small"), QVariant::fromValue(static_cast<int>(DrawerSize::Small)));
        addItem(tr("Medium"), QVariant::fromValue(static_cast<int>(DrawerSize::Medium)));
        addItem(tr("Large"), QVariant::fromValue(static_cast<int>(DrawerSize::Large)));
        setCurrentIndex(findData(static_cast<int>(current)));
    }

    DrawerSize size() const { return static_cast<DrawerSize>(currentData().toInt()); }
};
