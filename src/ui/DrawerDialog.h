#pragma once

#include <QDialog>

#include "core/Models.h"

class QLineEdit;
class SizeComboBox;

class DrawerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DrawerDialog(const Drawer &drawer, QWidget *parent = nullptr);
    Drawer drawer() const;

private:
    Drawer m_drawer;
    QLineEdit *m_label;
    SizeComboBox *m_size;
};
