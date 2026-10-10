#pragma once

#include <QDialog>
#include <functional>

#include "core/Layout.h"
#include "core/Models.h"

class QLineEdit;
class QPushButton;
class QDoubleSpinBox;
class QSpinBox;
class SizeComboBox;

// Creates or edits a drawer group, including its number of rows and columns.
class GroupDialog : public QDialog
{
    Q_OBJECT

public:
    // Returns true if the candidate placement is free of collisions.
    using PlacementCheck = std::function<bool(const DrawerGroup &)>;
    // Proposes a position for a block of the given size.
    using PositionSuggester = std::function<Layout::Position(int rows, int cols)>;

    // `suggest` may be empty; when set, the position follows the size until edited by hand.
    GroupDialog(const DrawerGroup &group, PlacementCheck isFree, PositionSuggester suggest,
                QWidget *parent = nullptr);
    DrawerGroup group() const;

    void accept() override;

private:
    void suggestPosition();
    void chooseColor();
    void showColor();

    DrawerGroup m_group;
    PlacementCheck m_isFree;
    QLineEdit *m_name;
    QSpinBox *m_rows;
    QSpinBox *m_cols;
    QDoubleSpinBox *m_posX;
    QDoubleSpinBox *m_posY;
    PositionSuggester m_suggest;
    bool m_autoPosition = false;
    bool m_updatingPosition = false;
    SizeComboBox *m_size;
    QPushButton *m_colorButton;
    QString m_color; // Empty means colored by drawer size.
};
