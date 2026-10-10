#pragma once

#include <QDialog>
#include <functional>

#include "core/Models.h"

class QLineEdit;
class QSpinBox;
class SizeComboBox;

// Creates or edits a drawer group. Dimensions are fixed once the group exists.
class GroupDialog : public QDialog
{
    Q_OBJECT

public:
    // Returns true if the candidate placement is free of collisions.
    using PlacementCheck = std::function<bool(const DrawerGroup &)>;

    GroupDialog(const DrawerGroup &group, PlacementCheck isFree, QWidget *parent = nullptr);
    DrawerGroup group() const;

    void accept() override;

private:
    DrawerGroup m_group;
    PlacementCheck m_isFree;
    QLineEdit *m_name;
    QSpinBox *m_rows;
    QSpinBox *m_cols;
    QSpinBox *m_posX;
    QSpinBox *m_posY;
    SizeComboBox *m_size;
};
