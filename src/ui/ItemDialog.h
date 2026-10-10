#pragma once

#include <QDialog>

#include "core/Models.h"

class QComboBox;
class QLineEdit;
class QSpinBox;

class ItemDialog : public QDialog
{
    Q_OBJECT

public:
    struct DrawerChoice {
        int id;
        QString text;
    };

    // `drawers` lists the valid destinations, so editing an item can also move it.
    ItemDialog(const Item &item, const QList<DrawerChoice> &drawers, QWidget *parent = nullptr);
    Item item() const;

private:
    Item m_item;
    QComboBox *m_drawer;
    QLineEdit *m_name;
    QLineEdit *m_category;
    QLineEdit *m_value;
    QLineEdit *m_package;
    QSpinBox *m_quantity;
    QLineEdit *m_notes;
};
