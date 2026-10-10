#pragma once

#include <QMainWindow>

#include "core/Models.h"
#include "ItemDialog.h"
#include "data/Repositories.h"

class CabinetView;
class ItemPanel;
class QComboBox;
class QLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(ICabinetRepository &cabinets, IDrawerRepository &drawers, IItemRepository &items,
               QWidget *parent = nullptr);

private:
    void createActions();
    void reloadCabinets(int selectId = 0);
    void reloadView();
    int currentCabinetId() const;

    void addCabinet();
    void editCabinet();
    void deleteCabinet();
    void addGroup();
    void editGroup(const DrawerGroup &group);
    void deleteGroup(const DrawerGroup &group);
    void editDrawer(const Drawer &drawer);
    void selectDrawer(const Drawer &drawer);
    void applySearch();
    void showGroupMenu(const DrawerGroup &group, const QPoint &globalPos);

    // Runs `action`, reporting persistence errors to the user.
    template <typename Fn> void guarded(Fn &&action);

    ICabinetRepository &m_cabinets;
    IDrawerRepository &m_drawers;
    IItemRepository &m_items;
    QComboBox *m_cabinetCombo;
    CabinetView *m_view;
    ItemPanel *m_panel;
    QLineEdit *m_search;
    QList<ItemDialog::DrawerChoice> m_drawerChoices;
    int m_selectedDrawerId = 0;
};
