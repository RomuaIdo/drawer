#pragma once

#include <QHash>
#include <QMainWindow>

#include "core/Models.h"
#include "ItemDialog.h"
#include "data/Repositories.h"
#include "export/Exporter.h"
#include "tools/ToolRegistry.h"

class CabinetView;
class AllItemsPanel;
class ItemPanel;
class QComboBox;
class QAction;
class QLineEdit;
class QScrollArea;
class QTabWidget;
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
    void moveGroups(const QList<DrawerGroup> &groups);
    void editDrawer(const Drawer &drawer);
    void exportOrganization();
    void importOrganization();
    void showTools();
    void exportMapping();
    void selectDrawer(const Drawer &drawer);
    void applySearch();
    void updateLowStock();
    void refreshCategories();
    void showItemDrawers(const QList<int> &drawerIds); // From the "All items" tab.
    void showGroupMenu(const DrawerGroup &group, int drawerId, const QPoint &globalPos);
    void copyItems(const QList<int> &itemIds);
    void cutItems(const QList<int> &itemIds);
    void pasteItems(int drawerId);
    void transferItems(const QList<int> &itemIds, int drawerId, bool copy);
    void refreshItems();

    // Edit menu: act on the focused part of the window.
    void editCopy();
    void editCut();
    void editPaste();
    void editSelectAll();
    void editDelete();
    void deleteDrawer(int drawerId);
    void undo();
    void redo();
    void afterHistoryChange(const QString &message);
    void updateUndoActions();
    // Closes the changes made so far as one undo step.
    void recordChange(const QString &label);

    // Records a change step when the surrounding user action ends, however it ends.
    class ChangeScope
    {
    public:
        ChangeScope(MainWindow &window, QString label)
            : m_window(window), m_label(std::move(label)) {}
        ~ChangeScope() { m_window.recordChange(m_label); }
        ChangeScope(const ChangeScope &) = delete;
        ChangeScope &operator=(const ChangeScope &) = delete;

    private:
        MainWindow &m_window;
        QString m_label;
    };

    // Runs `action`, reporting persistence errors to the user.
    template <typename Fn> void guarded(Fn &&action);

    ICabinetRepository &m_cabinets;
    IDrawerRepository &m_drawers;
    IItemRepository &m_items;
    IMappingRepository &m_mapping;
    IOrganizationRepository &m_organization;
    IUndoHistory &m_undo;
    const IExporter &m_exporter;
    ToolRegistry m_tools;
    ToolsDialog *m_toolsDialog = nullptr;
    QComboBox *m_cabinetCombo;
    CabinetView *m_view;
    ItemPanel *m_panel;
    AllItemsPanel *m_allItems;
    QTabWidget *m_itemTabs;
    QScrollArea *m_scroll;
    QLineEdit *m_search;
    QComboBox *m_category;
    QList<ItemDialog::DrawerChoice> m_drawerChoices;
    QHash<int, Drawer> m_drawersById; // Drawers of the current cabinet.
    QHash<int, QString> m_groupNames; // Group id -> name, for the current cabinet.
    int m_selectedDrawerId = 0;
    QList<int> m_itemClipboard; // Items copied or cut, waiting to be pasted.
    bool m_clipboardIsCut = false; // Pasting then moves the items instead of copying them.
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
};
