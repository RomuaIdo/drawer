#pragma once

#include <QDialog>

#include "core/Models.h"

class QLineEdit;
class QPlainTextEdit;

class CabinetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CabinetDialog(const Cabinet &cabinet, QWidget *parent = nullptr);
    Cabinet cabinet() const;

private:
    Cabinet m_cabinet;
    QLineEdit *m_name;
    QPlainTextEdit *m_description;
};
