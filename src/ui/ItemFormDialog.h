#pragma once

#include <QDialog>
#include <QString>

#include "core/Models.h"

class QCheckBox;
class QFormLayout;
class QLineEdit;
class QPushButton;
class QSpinBox;

// The fields every item dialog shares (name, category, value, package, minimum stock, notes)
// and the OK/Cancel handling. Subclasses add what is specific to them.
class ItemFormDialog : public QDialog
{
    Q_OBJECT

public:
    // The edited item: the original with the shared fields replaced.
    virtual Item item() const;
    // Shows the dialog without blocking the main window (so its cabinet stays clickable) and
    // waits for it to close. Returns true if accepted; false if cancelled or already open.
    bool run();

protected:
    struct Row {
        QString label;
        QWidget *widget = nullptr;
    };

    ItemFormDialog(const Item &item, QWidget *parent);

    // Builds the form: `lead` goes before the name, `extra` after the package.
    void buildForm(const Row &lead, const Row &extra);
    // OK needs a name, plus whatever isComplete() adds.
    void updateOk();
    virtual bool isComplete() const { return true; }
    // Fills every shared field except the name from `source`.
    void fillDetails(const Item &source);
    QFormLayout *form() const { return m_form; }

    Item m_item;
    QLineEdit *m_name;
    QLineEdit *m_category;
    QLineEdit *m_value;
    QLineEdit *m_package;

private:
    QCheckBox *m_alert;
    QSpinBox *m_minStock;
    QLineEdit *m_notes;
    QPushButton *m_ok = nullptr;
    QFormLayout *m_form = nullptr;
};
