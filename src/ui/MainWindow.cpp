#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QToolBar>

#include "CabinetDialog.h"
#include "CabinetView.h"
#include "DrawerDialog.h"
#include "GroupDialog.h"
#include "AllItemsPanel.h"
#include "ItemPanel.h"
#include "ListPanel.h"
#include "core/Layout.h"
#include "data/ItemSearch.h"
#include "tools/ToolsDialog.h"
#include "data/DatabaseError.h"
#include "export/OrganizationJson.h"

MainWindow::MainWindow(const Repositories &repos, const IExporter &exporter, QWidget *parent)
    : QMainWindow(parent)
    , m_cabinets(repos.cabinets)
    , m_drawers(repos.drawers)
    , m_items(repos.items)
    , m_mapping(repos.mapping)
    , m_organization(repos.organization)
    , m_lists(repos.lists)
    , m_undo(repos.undo)
    , m_exporter(exporter)
    , m_tools(ToolRegistry::withDefaultTools())
    , m_cabinetCombo(new QComboBox)
    , m_view(new CabinetView)
    , m_panel(new ItemPanel(repos.items))
    , m_allItems(new AllItemsPanel(repos.items))
    , m_pickList(new ListPanel(ListKind::Pick, repos.lists))
    , m_restockList(new ListPanel(ListKind::Restock, repos.lists))
    , m_itemTabs(new QTabWidget)
    , m_search(new QLineEdit)
    , m_category(new QComboBox)
{
    setWindowTitle(tr("Drawer"));
    resize(1024, 700);

    m_scroll = new QScrollArea;
    m_scroll->setWidget(m_view);
    m_scroll->setWidgetResizable(true);
    setCentralWidget(m_scroll);

    m_itemTabs->addTab(m_panel, tr("Selected drawer"));
    m_itemTabs->addTab(m_allItems, tr("All items"));
    m_itemTabs->addTab(m_pickList, tr("Pick list"));
    m_itemTabs->addTab(m_restockList, tr("Restock list"));
    auto *dock = new QDockWidget(tr("Items"), this);
    dock->setWidget(m_itemTabs);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->setMinimumWidth(420);

    createActions();

    connect(m_cabinetCombo, &QComboBox::currentIndexChanged, this, &MainWindow::reloadView);
    connect(m_view, &CabinetView::drawerSelected, this, [this](const Drawer &drawer) {
        // Picking a drawer on the cabinet replaces whatever the "All items" list pointed at.
        m_allItems->clearSelection();
        m_view->setEmphasizedDrawers({});
        selectDrawer(drawer);
    });
    connect(m_allItems, &AllItemsPanel::drawersChosen, this, &MainWindow::showItemDrawers);
    // Items edited from the "All items" tab: everything else follows, the tab already did.
    connect(m_allItems, &AllItemsPanel::itemsChanged, this, [this](const QString &what) {
        recordChange(what);
        refreshCategories();
        refreshLists();
        m_panel->reload();
        applySearch();
        updateLowStock();
    });
    connect(m_allItems, &AllItemsPanel::drawersActivated, this,
            [this] { m_itemTabs->setCurrentWidget(m_panel); });
    // Each tab points at the drawers of its own selection; leaving a tab drops its emphasis.
    connect(m_itemTabs, &QTabWidget::currentChanged, this,
            [this] { showItemDrawers(drawersOfCurrentTab()); });
    for (ListPanel *list : {m_pickList, m_restockList}) {
        connect(list, &ListPanel::drawersChosen, this, &MainWindow::showItemDrawers);
        connect(list, &ListPanel::listChanged, this, [this](const QString &what) {
            recordChange(what);
            refreshLists();
        });
        // Applying changes the stock: everything that shows quantities follows.
        connect(list, &ListPanel::applied, this, [this](const QString &what) {
            recordChange(what);
            refreshItems();
        });
    }
    connect(m_panel, &ItemPanel::addToListRequested, this, &MainWindow::addToList);
    connect(m_allItems, &AllItemsPanel::addToListRequested, this, &MainWindow::addToList);
    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::applySearch);
    connect(m_category, &QComboBox::currentIndexChanged, this, [this] {
        m_allItems->setCategoryFilter(m_category->currentData().toString());
        applySearch();
    });
    connect(m_panel, &ItemPanel::copyRequested, this, &MainWindow::copyItems);
    connect(m_panel, &ItemPanel::pasteRequested, this,
            [this] { pasteItems(m_selectedDrawerId); });
    connect(m_view, &CabinetView::itemsDropped, this, &MainWindow::transferItems);
    connect(m_panel, &ItemPanel::cutRequested, this, &MainWindow::cutItems);
    connect(m_panel, &ItemPanel::itemsChanged, this, [this](const QString &what) {
        recordChange(what);
        refreshCategories();
        m_allItems->reload();
        refreshLists();
        applySearch();
        updateLowStock();
    });
    connect(m_view, &CabinetView::drawerActivated, this, &MainWindow::editDrawer);
    connect(m_view, &CabinetView::groupsMoved, this, &MainWindow::moveGroups);
    connect(m_view, &CabinetView::groupContextRequested, this, &MainWindow::showGroupMenu);

    guarded([this] { reloadCabinets(); });
}

void MainWindow::createActions()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Import database..."), this, &MainWindow::importOrganization);
    fileMenu->addAction(tr("Export &database..."), this, &MainWindow::exportOrganization);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Export mapping as %1...").arg(m_exporter.name()), QKeySequence(Qt::CTRL | Qt::Key_E), this,
                        &MainWindow::exportMapping);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    m_undoAction = editMenu->addAction(tr("&Undo"), QKeySequence::Undo, this, &MainWindow::undo);
    m_redoAction = editMenu->addAction(tr("&Redo"), this, &MainWindow::redo);
    m_redoAction->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Y),
                                QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
    editMenu->addSeparator();
    editMenu->addAction(tr("Cu&t"), QKeySequence::Cut, this, &MainWindow::editCut);
    editMenu->addAction(tr("&Copy"), QKeySequence::Copy, this, &MainWindow::editCopy);
    editMenu->addAction(tr("&Paste"), QKeySequence::Paste, this, &MainWindow::editPaste);
    editMenu->addSeparator();
    editMenu->addAction(tr("&Delete"), QKeySequence::Delete, this, &MainWindow::editDelete);
    editMenu->addAction(tr("Select &all"), QKeySequence::SelectAll, this,
                        &MainWindow::editSelectAll);
    updateUndoActions();

    menuBar()->addMenu(tr("&Tools"))->addAction(tr("&Electronics tools..."), this,
                                                &MainWindow::showTools);

    auto *bar = addToolBar(tr("Cabinet"));
    bar->setMovable(false);
    bar->addWidget(new QLabel(tr("Cabinet: ")));
    bar->addWidget(m_cabinetCombo);
    m_cabinetCombo->setMinimumWidth(180);
    bar->addAction(tr("New"), this, &MainWindow::addCabinet);
    bar->addAction(tr("Edit"), this, &MainWindow::editCabinet);
    bar->addAction(tr("Delete"), this, &MainWindow::deleteCabinet);
    bar->addSeparator();
    bar->addAction(tr("Add drawer group"), this, &MainWindow::addGroup);
    bar->addSeparator();
    m_search->setPlaceholderText(tr("Search items or a value..."));
    m_search->setToolTip(tr("Text matches name, category, value, package and notes. A value such "
                            "as 1uF also finds items written 1u or 1000n."));
    m_search->setClearButtonEnabled(true);
    m_search->setMaximumWidth(260);
    bar->addWidget(m_search);
    m_category->setToolTip(tr("Show only items of this category"));
    bar->addWidget(m_category);
}

template <typename Fn> void MainWindow::guarded(Fn &&action)
{
    try {
        action();
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), QString::fromUtf8(e.what()));
    }
}

int MainWindow::currentCabinetId() const
{
    return m_cabinetCombo->currentData().toInt();
}

void MainWindow::reloadCabinets(int selectId)
{
    const int wanted = selectId ? selectId : currentCabinetId();
    {
        QSignalBlocker block(m_cabinetCombo);
        m_cabinetCombo->clear();
        for (const Cabinet &c : m_cabinets.all())
            m_cabinetCombo->addItem(c.name, c.id);
        const int index = m_cabinetCombo->findData(wanted);
        m_cabinetCombo->setCurrentIndex(index >= 0 ? index : (m_cabinetCombo->count() ? 0 : -1));
    }
    reloadView();
}

void MainWindow::reloadView()
{
    const int previouslySelected = m_selectedDrawerId;
    QList<CabinetView::GroupLayout> layouts;
    if (currentCabinetId()) {
        guarded([&] {
            for (const DrawerGroup &g : m_drawers.groupsOf(currentCabinetId()))
                layouts.append({g, m_drawers.drawersOf(g.id)});
        });
    }
    m_view->setLayouts(layouts);

    m_drawerChoices.clear();
    m_groupNames.clear();
    m_drawersById.clear();
    for (const auto &l : layouts) {
        m_groupNames.insert(l.group.id, l.group.name);
        for (const Drawer &d : l.drawers)
            m_drawersById.insert(d.id, d);
        for (const Drawer &d : l.drawers)
            m_drawerChoices.append({d.id, QString("%1 / %2").arg(l.group.name, d.label)});
    }
    m_selectedDrawerId = 0;
    m_panel->clear();
    applySearch();
    updateLowStock();
    refreshCategories();
    m_allItems->setDestinations(m_drawerChoices);
    refreshLists();
    m_allItems->setCabinet(currentCabinetId());
    m_view->setEmphasizedDrawers({});

    // Keep the selection across reloads that did not remove the drawer.
    for (const auto &l : layouts) {
        for (const Drawer &d : l.drawers) {
            if (d.id == previouslySelected)
                selectDrawer(d);
        }
    }
}

void MainWindow::exportOrganization()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Export database"),
                                                "drawer-database.json",
                                                tr("Drawer database (*.json)"));
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".json";

    guarded([&] {
        const Organization organization = m_organization.load();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || file.write(OrganizationJson::write(organization)) < 0) {
            QMessageBox::critical(this, tr("Export failed"), file.errorString());
            return;
        }
        statusBar()->showMessage(
            tr("Exported %n cabinet(s) to %1", "", organization.cabinets.size()).arg(path));
    });
}

void MainWindow::importOrganization()
{
    ChangeScope scope(*this, tr("Import database"));
    const QString path = QFileDialog::getOpenFileName(this, tr("Import database"), QString(),
                                                      tr("Drawer database (*.json)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Import failed"), file.errorString());
        return;
    }
    try {
        const Organization organization = OrganizationJson::read(file.readAll());
        const auto answer = QMessageBox::question(
            this, tr("Import database"),
            tr("Add %n cabinet(s) from this file? Existing cabinets are kept.", "",
               organization.cabinets.size()));
        if (answer != QMessageBox::Yes)
            return;
        m_organization.import(organization);
        reloadCabinets();
        statusBar()->showMessage(tr("Imported %n cabinet(s)", "", organization.cabinets.size()));
    } catch (const OrganizationFormatError &e) {
        QMessageBox::critical(this, tr("Import failed"), QString::fromUtf8(e.what()));
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), QString::fromUtf8(e.what()));
    }
}

void MainWindow::showTools()
{
    if (!m_toolsDialog)
        m_toolsDialog = new ToolsDialog(m_tools, this);
    m_toolsDialog->show();
    m_toolsDialog->raise();
}

void MainWindow::exportMapping()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Export mapping"), "drawer-mapping",
                                                m_exporter.fileFilter());
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += "." + m_exporter.defaultSuffix();

    guarded([&] {
        const QList<MappingRow> rows = m_mapping.rows();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(this, tr("Export failed"), file.errorString());
            return;
        }
        m_exporter.write(rows, file);
        statusBar()->showMessage(tr("Exported %n row(s) to %1", "", rows.size()).arg(path));
    });
}

void MainWindow::selectDrawer(const Drawer &drawer)
{
    m_selectedDrawerId = drawer.id;
    m_allItems->setDefaultDrawer(drawer.id);
    m_view->setSelectedDrawer(drawer.id);
    const QString groupName = m_groupNames.value(drawer.groupId);
    m_panel->setDrawer(drawer, groupName, m_drawerChoices);
    statusBar()->showMessage(tr("Group %1, drawer %2 (%3)")
                                 .arg(groupName, drawer.label, drawerSizeToString(drawer.size)));
}

// Selecting a row of "All items" selects its drawer and darkens every drawer holding the item.
void MainWindow::showItemDrawers(const QList<int> &drawerIds)
{
    QSet<int> shown;
    for (int id : drawerIds) {
        if (m_drawersById.contains(id))
            shown.insert(id);
    }
    m_view->setEmphasizedDrawers(shown);
    if (shown.isEmpty())
        return;

    const Drawer first = m_drawersById.value(drawerIds.first());
    selectDrawer(first);
    const QRect where = m_view->drawerRect(first.id);
    if (!where.isNull())
        m_scroll->ensureVisible(where.center().x(), where.center().y(), where.width(), where.height());
}

void MainWindow::updateLowStock()
{
    QSet<int> drawerIds;
    guarded([&] {
        for (const Item &item : m_items.lowStock())
            drawerIds.insert(item.drawerId);
    });
    m_view->setLowStockDrawers(drawerIds);
}

// Highlights every drawer holding an item that matches the search text.
void MainWindow::applySearch()
{
    QSet<int> drawerIds;
    const SearchQuery query{m_search->text(), m_category->currentData().toString()};
    if (!query.isEmpty()) {
        guarded([&] {
            for (const Item &item : ItemSearch::run(m_items, query))
                drawerIds.insert(item.drawerId);
        });
        statusBar()->showMessage(tr("%n drawer(s) match", "", drawerIds.size()));
    }
    m_view->setHighlightedDrawers(drawerIds);
}

// Keeps the category list in step with the items; a category that vanished stops filtering.
QList<int> MainWindow::drawersOfCurrentTab() const
{
    const QWidget *tab = m_itemTabs->currentWidget();
    if (tab == m_allItems)
        return m_allItems->selectedDrawerIds();
    if (tab == m_pickList)
        return m_pickList->selectedDrawerIds();
    if (tab == m_restockList)
        return m_restockList->selectedDrawerIds();
    return {};
}

// Reloads both lists and shows how many lines each one holds in its tab title.
void MainWindow::refreshLists()
{
    m_pickList->reload();
    m_restockList->reload();
    const auto title = [](const QString &name, int lines) {
        return lines > 0 ? QString("%1 (%2)").arg(name).arg(lines) : name;
    };
    m_itemTabs->setTabText(m_itemTabs->indexOf(m_pickList), title(tr("Pick list"), m_pickList->lineCount()));
    m_itemTabs->setTabText(m_itemTabs->indexOf(m_restockList),
                           title(tr("Restock list"), m_restockList->lineCount()));
}

void MainWindow::addToList(ListKind kind, const QList<int> &itemIds)
{
    if (itemIds.isEmpty())
        return;
    const bool pick = kind == ListKind::Pick;
    ChangeScope scope(*this, pick ? tr("Add to pick list") : tr("Add to restock list"));
    guarded([&] {
        m_lists.add(kind, itemIds);
        refreshLists();
        statusBar()->showMessage(pick ? tr("Added %n item(s) to the pick list", "", itemIds.size())
                                      : tr("Added %n item(s) to the restock list", "", itemIds.size()));
    });
}

void MainWindow::refreshCategories()
{
    const QString previous = m_category->currentData().toString();
    QStringList categories;
    guarded([&] { categories = m_items.categories(); });

    QSignalBlocker block(m_category);
    m_category->clear();
    m_category->addItem(tr("All categories"), QString());
    for (const QString &category : categories)
        m_category->addItem(category, category);
    const int index = m_category->findData(previous);
    m_category->setCurrentIndex(index >= 0 ? index : 0);
    m_allItems->setCategoryFilter(m_category->currentData().toString());
}

void MainWindow::addCabinet()
{
    ChangeScope scope(*this, tr("Add cabinet"));
    CabinetDialog dialog({}, this);
    if (dialog.exec() == QDialog::Accepted) {
        guarded([&] { reloadCabinets(m_cabinets.add(dialog.cabinet()).id); });
    }
}

void MainWindow::editCabinet()
{
    ChangeScope scope(*this, tr("Edit cabinet"));
    const int id = currentCabinetId();
    if (!id)
        return;
    guarded([&] {
        for (const Cabinet &c : m_cabinets.all()) {
            if (c.id != id)
                continue;
            CabinetDialog dialog(c, this);
            if (dialog.exec() == QDialog::Accepted) {
                m_cabinets.update(dialog.cabinet());
                reloadCabinets(id);
            }
        }
    });
}

void MainWindow::deleteCabinet()
{
    ChangeScope scope(*this, tr("Delete cabinet"));
    const int id = currentCabinetId();
    if (!id)
        return;
    const auto answer = QMessageBox::question(
        this, tr("Delete cabinet"),
        tr("Delete \"%1\" with all its drawers and items?").arg(m_cabinetCombo->currentText()));
    if (answer == QMessageBox::Yes) {
        guarded([&] {
            m_cabinets.remove(id);
            reloadCabinets();
        });
    }
}

void MainWindow::addGroup()
{
    ChangeScope scope(*this, tr("Add drawer group"));
    const int cabinetId = currentCabinetId();
    if (!cabinetId) {
        QMessageBox::information(this, tr("Drawer group"), tr("Create a cabinet first."));
        return;
    }
    guarded([&] {
        const QList<DrawerGroup> existing = m_drawers.groupsOf(cabinetId);
        DrawerGroup proposal;
        proposal.cabinetId = cabinetId;
        proposal.rows = proposal.cols = 4;

        GroupDialog dialog(
            proposal, [&](const DrawerGroup &g) { return !Layout::collides(g, existing); },
            [&](int rows, int cols) { return Layout::suggestPosition(existing, rows, cols); },
            this);
        if (dialog.exec() == QDialog::Accepted) {
            m_drawers.addGroup(dialog.group());
            reloadView();
        }
    });
}

void MainWindow::editGroup(const DrawerGroup &group)
{
    ChangeScope scope(*this, tr("Edit drawer group"));
    guarded([&] {
        const QList<DrawerGroup> existing = m_drawers.groupsOf(group.cabinetId);
        const auto sameSize = [&](const DrawerGroup &g) {
            return g.rows == group.rows && g.cols == group.cols;
        };
        // A resize pushes its neighbors away, so only plain moves can collide.
        GroupDialog dialog(
            group,
            [&](const DrawerGroup &g) { return !sameSize(g) || !Layout::collides(g, existing); },
            {}, this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        const DrawerGroup edited = dialog.group();
        if (sameSize(edited)) {
            m_drawers.updateGroup(edited);
        } else {
            const int lost = m_drawers.itemsLostByResize(edited.id, edited.rows, edited.cols);
            if (lost > 0
                && QMessageBox::warning(
                       this, tr("Resize drawer group"),
                       tr("Shrinking removes drawers that hold %n item(s). Continue?", "", lost),
                       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel)
                    != QMessageBox::Yes)
                return;
            m_drawers.resizeGroup(
                edited, Layout::displacedByResize(existing, edited, group.rows, group.cols));
        }
        reloadView();
    });
}

void MainWindow::deleteGroup(const DrawerGroup &group)
{
    ChangeScope scope(*this, tr("Delete drawer group"));
    const auto answer = QMessageBox::question(
        this, tr("Delete drawer group"),
        tr("Delete \"%1\" with all its drawers and items?").arg(group.name));
    if (answer == QMessageBox::Yes) {
        guarded([&] {
            m_drawers.removeGroup(group.id);
            reloadView();
        });
    }
}

void MainWindow::moveGroups(const QList<DrawerGroup> &groups)
{
    ChangeScope scope(*this, tr("Move drawer groups"));
    guarded([&] {
        m_drawers.moveGroups(groups);
        reloadView();
    });
}

void MainWindow::editDrawer(const Drawer &drawer)
{
    ChangeScope scope(*this, tr("Edit drawer"));
    DrawerDialog dialog(drawer, this);
    if (dialog.exec() == QDialog::Accepted) {
        guarded([&] {
            m_drawers.updateDrawer(dialog.drawer());
            reloadView();
        });
    }
}

void MainWindow::showGroupMenu(const DrawerGroup &group, int drawerId, const QPoint &globalPos)
{
    QMenu menu;
    if (drawerId) {
        QString label;
        for (const auto &choice : m_drawerChoices) {
            if (choice.id == drawerId)
                label = choice.text;
        }
        menu.addAction(tr("Paste %n item(s) into %1", "", m_itemClipboard.size()).arg(label),
                       [&] { pasteItems(drawerId); })
            ->setEnabled(!m_itemClipboard.isEmpty());
        menu.addAction(tr("Delete drawer"), [&] { deleteDrawer(drawerId); });
        menu.addSeparator();
    }
    menu.addAction(tr("Edit group \"%1\"").arg(group.name), [&] { editGroup(group); });
    menu.addAction(tr("Delete group"), [&] { deleteGroup(group); });
    menu.addSeparator();
    QAction *free = menu.addAction(tr("Move freely (ignore grid)"));
    free->setCheckable(true);
    free->setChecked(m_view->freeMove());
    free->setToolTip(tr("Applies to every block you drag until turned off"));
    connect(free, &QAction::toggled, m_view, &CabinetView::setFreeMove);
    menu.exec(globalPos);
}

void MainWindow::copyItems(const QList<int> &itemIds)
{
    m_itemClipboard = itemIds;
    m_clipboardIsCut = false;
    m_panel->setCanPaste(!m_itemClipboard.isEmpty());
    statusBar()->showMessage(tr("Copied %n item(s)", "", itemIds.size()));
}

void MainWindow::cutItems(const QList<int> &itemIds)
{
    m_itemClipboard = itemIds;
    m_clipboardIsCut = true;
    m_panel->setCanPaste(!m_itemClipboard.isEmpty());
    statusBar()->showMessage(tr("Cut %n item(s); paste them into a drawer to move them", "",
                                itemIds.size()));
}

void MainWindow::pasteItems(int drawerId)
{
    if (m_itemClipboard.isEmpty() || !drawerId)
        return;
    transferItems(m_itemClipboard, drawerId, !m_clipboardIsCut);
    if (m_clipboardIsCut) { // Cut items can only be pasted once.
        m_itemClipboard.clear();
        m_clipboardIsCut = false;
        m_panel->setCanPaste(false);
    }
}

void MainWindow::editCopy()
{
    const QList<int> ids = m_panel->selectedItemIds();
    if (ids.isEmpty())
        statusBar()->showMessage(tr("Select items to copy"));
    else
        copyItems(ids);
}

void MainWindow::editCut()
{
    const QList<int> ids = m_panel->selectedItemIds();
    if (ids.isEmpty())
        statusBar()->showMessage(tr("Select items to cut"));
    else
        cutItems(ids);
}

void MainWindow::editPaste()
{
    if (m_selectedDrawerId == 0)
        statusBar()->showMessage(tr("Select a drawer to paste into"));
    else
        pasteItems(m_selectedDrawerId);
}

// Selects the items when the list has the focus, otherwise the drawer groups.
void MainWindow::editSelectAll()
{
    const QWidget *focus = QApplication::focusWidget();
    if (focus && m_panel->isAncestorOf(focus))
        m_panel->selectAll();
    else
        m_view->selectAllGroups();
}

// Deletes the selected items when the list has the focus, otherwise the selected drawer.
void MainWindow::editDelete()
{
    const QWidget *focus = QApplication::focusWidget();
    if (focus && m_panel->isAncestorOf(focus))
        m_panel->removeSelected();
    else if (m_selectedDrawerId == 0)
        statusBar()->showMessage(tr("Select a drawer to delete"));
    else
        deleteDrawer(m_selectedDrawerId);
}

void MainWindow::deleteDrawer(int drawerId)
{
    ChangeScope scope(*this, tr("Delete drawer"));
    guarded([&] {
        const int itemCount = m_items.inDrawer(drawerId).size();
        if (itemCount > 0
            && QMessageBox::question(
                   this, tr("Delete drawer"),
                   tr("This drawer holds %n item(s), which will be deleted too. Continue?", "",
                      itemCount))
                != QMessageBox::Yes)
            return;
        m_drawers.removeDrawer(drawerId);
        reloadView();
        statusBar()->showMessage(tr("Drawer deleted"));
    });
}

void MainWindow::recordChange(const QString &label)
{
    try {
        m_undo.commit(label);
    } catch (const DatabaseError &e) {
        QMessageBox::critical(this, tr("Database error"), QString::fromUtf8(e.what()));
    }
    updateUndoActions();
}

void MainWindow::updateUndoActions()
{
    m_undoAction->setEnabled(m_undo.canUndo());
    m_undoAction->setText(m_undo.canUndo() ? tr("&Undo %1").arg(m_undo.undoLabel()) : tr("&Undo"));
    m_redoAction->setEnabled(m_undo.canRedo());
    m_redoAction->setText(m_undo.canRedo() ? tr("&Redo %1").arg(m_undo.redoLabel()) : tr("&Redo"));
}

void MainWindow::undo()
{
    if (!m_undo.canUndo())
        return;
    const QString label = m_undo.undoLabel();
    guarded([&] { m_undo.undo(); });
    afterHistoryChange(tr("Undid: %1").arg(label));
}

void MainWindow::redo()
{
    if (!m_undo.canRedo())
        return;
    const QString label = m_undo.redoLabel();
    guarded([&] { m_undo.redo(); });
    afterHistoryChange(tr("Redid: %1").arg(label));
}

// Everything on screen may have changed; rebuild it from the database.
void MainWindow::afterHistoryChange(const QString &message)
{
    updateUndoActions();
    guarded([this] { reloadCabinets(); });
    statusBar()->showMessage(message);
}

void MainWindow::transferItems(const QList<int> &itemIds, int drawerId, bool copy)
{
    ChangeScope scope(*this, copy ? tr("Copy items") : tr("Move items"));
    guarded([&] {
        if (copy)
            m_items.copyItems(itemIds, drawerId);
        else
            m_items.moveItems(itemIds, drawerId);
        refreshItems();
        statusBar()->showMessage(copy ? tr("Copied %n item(s)", "", itemIds.size())
                                      : tr("Moved %n item(s)", "", itemIds.size()));
    });
}

void MainWindow::refreshItems()
{
    m_panel->reload();
    refreshCategories();
    m_allItems->reload();
    refreshLists();
    applySearch();
    updateLowStock();
}
