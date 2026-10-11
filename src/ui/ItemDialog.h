#pragma once

#include "CabinetView.h"
#include "ItemFormDialog.h"

class QLabel;
class QSpinBox;

// Adds or edits one entry of an item: the drawer is picked by clicking it on `cabinet`, the
// cabinet view that is already open in the main window, so editing can also move the item.
class ItemDialog : public ItemFormDialog
{
    Q_OBJECT

public:
    ItemDialog(const Item &item, const DrawerLayouts &drawers, CabinetView *cabinet,
               QWidget *parent = nullptr);

    // For items that are about to be restocked: the quantity is how many to add.
    void setRestockMode();
    // Offers the items already in stock while the name is typed; choosing one fills the
    // category, value, package, minimum stock and notes.
    void setSuggestions(const QList<Item> &items);
    Item item() const override;

protected:
    bool isComplete() const override { return m_drawerId != 0; }

private:
    void chooseDrawer(int drawerId);

    DrawerLayouts m_layouts;
    QLabel *m_drawerText;
    QSpinBox *m_quantity;
    int m_drawerId = 0;
};
