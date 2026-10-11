#include "CabinetEditor.h"

#include <QMessageBox>

#include "CabinetDialog.h"
#include "DatabaseGuard.h"
#include "DrawerDialog.h"
#include "GroupDialog.h"
#include "core/Layout.h"

CabinetEditor::CabinetEditor(ICabinetRepository &cabinets, IDrawerRepository &drawers,
                             QWidget *parent)
    : QObject(parent)
    , m_cabinets(cabinets)
    , m_drawers(drawers)
{
}

QWidget *CabinetEditor::window() const
{
    return static_cast<QWidget *>(parent());
}

void CabinetEditor::addCabinet()
{
    CabinetDialog dialog({}, window());
    if (dialog.exec() == QDialog::Accepted)
        runGuarded(window(), [&] { emit cabinetsChanged(m_cabinets.add(dialog.cabinet()).id); });
}

void CabinetEditor::editCabinet(int cabinetId)
{
    if (!cabinetId)
        return;
    runGuarded(window(), [&] {
        for (const Cabinet &c : m_cabinets.all()) {
            if (c.id != cabinetId)
                continue;
            CabinetDialog dialog(c, window());
            if (dialog.exec() == QDialog::Accepted) {
                m_cabinets.update(dialog.cabinet());
                emit cabinetsChanged(cabinetId);
            }
        }
    });
}

void CabinetEditor::deleteCabinet(int cabinetId, const QString &name)
{
    if (!cabinetId)
        return;
    const auto answer = QMessageBox::question(
        window(), tr("Delete cabinet"),
        tr("Delete \"%1\" with all its drawers and items?").arg(name));
    if (answer == QMessageBox::Yes) {
        runGuarded(window(), [&] {
            m_cabinets.remove(cabinetId);
            emit cabinetsChanged(0);
        });
    }
}

void CabinetEditor::addGroup(int cabinetId)
{
    if (!cabinetId) {
        QMessageBox::information(window(), tr("Drawer group"), tr("Create a cabinet first."));
        return;
    }
    runGuarded(window(), [&] {
        const QList<DrawerGroup> existing = m_drawers.groupsOf(cabinetId);
        DrawerGroup proposal;
        proposal.cabinetId = cabinetId;
        proposal.rows = proposal.cols = 4;

        GroupDialog dialog(
            proposal, [&](const DrawerGroup &g) { return !Layout::collides(g, existing); },
            [&](int rows, int cols) { return Layout::suggestPosition(existing, rows, cols); },
            window());
        if (dialog.exec() == QDialog::Accepted) {
            m_drawers.addGroup(dialog.group());
            emit layoutChanged();
        }
    });
}

void CabinetEditor::editGroup(const DrawerGroup &group)
{
    runGuarded(window(), [&] {
        const QList<DrawerGroup> existing = m_drawers.groupsOf(group.cabinetId);
        const auto sameSize = [&](const DrawerGroup &g) {
            return g.rows == group.rows && g.cols == group.cols;
        };
        // A resize pushes its neighbors away, so only plain moves can collide.
        GroupDialog dialog(
            group,
            [&](const DrawerGroup &g) { return !sameSize(g) || !Layout::collides(g, existing); },
            {}, window());
        if (dialog.exec() != QDialog::Accepted)
            return;

        const DrawerGroup edited = dialog.group();
        if (sameSize(edited)) {
            m_drawers.updateGroup(edited);
        } else {
            const int lost = m_drawers.itemsLostByResize(edited.id, edited.rows, edited.cols);
            if (lost > 0
                && QMessageBox::warning(
                       window(), tr("Resize drawer group"),
                       tr("Shrinking removes drawers that hold %n item(s). Continue?", "", lost),
                       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel)
                    != QMessageBox::Yes)
                return;
            m_drawers.resizeGroup(
                edited, Layout::displacedByResize(existing, edited, group.rows, group.cols));
        }
        emit layoutChanged();
    });
}

void CabinetEditor::deleteGroup(const DrawerGroup &group)
{
    const auto answer = QMessageBox::question(
        window(), tr("Delete drawer group"),
        tr("Delete \"%1\" with all its drawers and items?").arg(group.name));
    if (answer == QMessageBox::Yes) {
        runGuarded(window(), [&] {
            m_drawers.removeGroup(group.id);
            emit layoutChanged();
        });
    }
}

void CabinetEditor::moveGroups(const QList<DrawerGroup> &groups)
{
    runGuarded(window(), [&] {
        m_drawers.moveGroups(groups);
        emit layoutChanged();
    });
}

void CabinetEditor::editDrawer(const Drawer &drawer)
{
    DrawerDialog dialog(drawer, window());
    if (dialog.exec() == QDialog::Accepted) {
        runGuarded(window(), [&] {
            m_drawers.updateDrawer(dialog.drawer());
            emit layoutChanged();
        });
    }
}
