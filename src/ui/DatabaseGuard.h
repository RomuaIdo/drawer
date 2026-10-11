#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

#include "data/DatabaseError.h"

// Runs a database action. A failure is reported to the user and the result is false, so
// callers can simply stop: `if (!runGuarded(this, [&] { ... })) return;`.
template <typename Fn>
bool runGuarded(QWidget *parent, Fn &&action)
{
    try {
        action();
        return true;
    } catch (const DatabaseError &e) {
        QMessageBox::critical(parent, QObject::tr("Database error"), QString::fromUtf8(e.what()));
        return false;
    }
}
