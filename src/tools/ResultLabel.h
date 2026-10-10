#pragma once

#include <QLabel>

// Label for tool results: default font, selectable text.
inline QLabel *makeResultLabel()
{
    auto *label = new QLabel;
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);
    return label;
}
