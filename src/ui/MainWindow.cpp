#include "MainWindow.h"

#include <QAction>
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
#include "ItemPanel.h"
#include "core/Layout.h"
#include "tools/ToolsDialog.h"
#include "data/DatabaseError.h"

MainWindow::MainWindow(const Repositories &repos, const IExporter &exporter, QWidget *parent)
    : QMainWindow(parent)
    , m_cabinets(repos.cabinets)
    , m_drawers(repos.drawers)
    , m_items(repos.items)
    , m_mapping(repos.mapping)
    , m_exporter(exporter)
    , m_tools(ToolRegistry::withDefaultTools())
    , m_cabinetCombo(new QComboBox)
    , m_view(new CabinetView)
    , m_panel(new ItemPanel(repos.items))
    , m_search(new QLineEdit)
{
    setWindowTitle(tr("Drawer"));
    resize(1024, 700);

    auto *scroll = new QScrollArea;
    scroll->setWidget(m_view);
    scroll->setWidgetResizable(true);
    setCentralWidget(scroll);

    auto *dock = new QDockWidget(tr("Items"), this);
    dock->setWidget(m_panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->setMinimumWidth(420);

    createActions();

    connect(m_cabinetCombo, &QComboBox::currentIndexChanged, this, &MainWindow::reloadView);
    connect(m_view, &CabinetView::drawerSelected, this, &MainWindow::selectDrawer);
    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::applySearch);
    connect(m_panel, &ItemPanel::itemsChanged, this, &MainWindow::applySearch);
    connect(m_view, &CabinetView::drawerActivated, this, &MainWindow::editDrawer);
    connect(m_view, &CabinetView::groupMoved, this, &MainWindow::moveGroup);
    connect(m_view, &CabinetView::groupContextRequested, this, &MainWindow::showGroupMenu);

    guarded([this] { reloadCabinets(); });
}

void MainWindow::createActions()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Export mapping as %1...").arg(m_exporter.name()), QKeySequence(Qt::CTRL | Qt::Key_E), this,
                        &MainWindow::exportMapping);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

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
    m_search->setPlaceholderText(tr("Search items..."));
    m_search->setClearButtonEnabled(true);
    m_search->setMaximumWidth(260);
    bar->addWidget(m_search);
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
    for (const auto &l : layouts) {
        for (const Drawer &d : l.drawers)
            m_drawerChoices.append({d.id, QString("%1 / %2").arg(l.group.name, d.label)});
    }
    m_selectedDrawerId = 0;
    m_panel->clear();
    applySearch();

    // Keep the selection across reloads that did not remove the drawer.
    for (const auto &l : layouts) {
        for (const Drawer &d : l.drawers) {
            if (d.id == previouslySelected)
                selectDrawer(d);
        }
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
    m_view->setSelectedDrawer(drawer.id);
    m_panel->setDrawer(drawer, m_drawerChoices);
    statusBar()->showMessage(
        tr("Drawer %1 (%2)").arg(drawer.label, drawerSizeToString(drawer.size)));
}

// Highlights every drawer holding an item that matches the search text.
void MainWindow::applySearch()
{
    QSet<int> drawerIds;
    const QString text = m_search->text().trimmed();
    if (!text.isEmpty()) {
        guarded([&] {
            for (const Item &item : m_items.search(text))
                drawerIds.insert(item.drawerId);
        });
        statusBar()->showMessage(tr("%n drawer(s) match", "", drawerIds.size()));
    }
    m_view->setHighlightedDrawers(drawerIds);
}

void MainWindow::addCabinet()
{
    CabinetDialog dialog({}, this);
    if (dialog.exec() == QDialog::Accepted) {
        guarded([&] { reloadCabinets(m_cabinets.add(dialog.cabinet()).id); });
    }
}

void MainWindow::editCabinet()
{
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
    guarded([&] {
        const QList<DrawerGroup> existing = m_drawers.groupsOf(group.cabinetId);
        GroupDialog dialog(
            group, [&](const DrawerGroup &g) { return !Layout::collides(g, existing); }, {}, this);
        if (dialog.exec() == QDialog::Accepted) {
            m_drawers.updateGroup(dialog.group());
            reloadView();
        }
    });
}

void MainWindow::deleteGroup(const DrawerGroup &group)
{
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

void MainWindow::moveGroup(const DrawerGroup &group, double x, double y)
{
    DrawerGroup moved = group;
    moved.posX = x;
    moved.posY = y;
    guarded([&] {
        m_drawers.updateGroup(moved);
        reloadView();
    });
}

void MainWindow::editDrawer(const Drawer &drawer)
{
    DrawerDialog dialog(drawer, this);
    if (dialog.exec() == QDialog::Accepted) {
        guarded([&] {
            m_drawers.updateDrawer(dialog.drawer());
            reloadView();
        });
    }
}

void MainWindow::showGroupMenu(const DrawerGroup &group, const QPoint &globalPos)
{
    QMenu menu;
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
