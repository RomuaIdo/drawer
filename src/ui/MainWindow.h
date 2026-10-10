#pragma once

#include <QHash>
#include <QMainWindow>

#include "core/Models.h"
#include "ItemDialog.h"
#include "data/Repositories.h"
#include "export/Exporter.h"
#include "tools/ToolRegistry.h"

class CabinetView;
class ItemPanel;
class QComboBox;
class QLineEdit;
class ToolsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const Repositories &repos, const IExporter &exporter, QWidget *parent = nullptr);

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
    void moveGroup(const DrawerGroup &group, double x, double y);
    void editDrawer(const Drawer &drawer);
    void exportOrganization();
    void importOrganization();
    void showTools();
    void exportMapping();
    void selectDrawer(const Drawer &drawer);
    void applySearch();
    void showGroupMenu(const DrawerGroup &group, const QPoint &globalPos);

    // Runs `action`, reporting persistence errors to the user.
    template <typename Fn> void guarded(Fn &&action);

    ICabinetRepository &m_cabinets;
    IDrawerRepository &m_drawers;
    IItemRepository &m_items;
    IMappingRepository &m_mapping;
    IOrganizationRepository &m_organization;
    const IExporter &m_exporter;
    ToolRegistry m_tools;
    ToolsDialog *m_toolsDialog = nullptr;
    QComboBox *m_cabinetCombo;
    CabinetView *m_view;
    ItemPanel *m_panel;
    QLineEdit *m_search;
    QList<ItemDialog::DrawerChoice> m_drawerChoices;
    QHash<int, QString> m_groupNames; // Group id -> name, for the current cabinet.
    int m_selectedDrawerId = 0;
};
