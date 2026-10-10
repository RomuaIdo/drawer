#pragma once

#include <QHash>
#include <QSet>
#include <QWidget>

#include "core/Layout.h"
#include "core/Models.h"

class QPainter;

// Draws a cabinet as a grid of drawer groups and reports clicks on drawers.
class CabinetView : public QWidget
{
    Q_OBJECT

public:
    struct GroupLayout {
        DrawerGroup group;
        QList<Drawer> drawers;
    };

    explicit CabinetView(QWidget *parent = nullptr);

    void setLayouts(const QList<GroupLayout> &layouts);
    void setSelectedDrawer(int drawerId);
    void setHighlightedDrawers(const QSet<int> &drawerIds);
    // Drawers holding items at or below their minimum stock get a warning badge.
    void setLowStockDrawers(const QSet<int> &drawerIds);
    // Drawers shown with a darker fill, e.g. where the item picked in a list is stored.
    void setEmphasizedDrawers(const QSet<int> &drawerIds);
    // Bounds of a drawer in widget coordinates; null when it is not shown.
    QRect drawerRect(int drawerId) const;
    QSize sizeHint() const override;

    // Free mode drops the grid snapping while dragging blocks.
    void setFreeMove(bool free) { m_freeMove = free; }
    bool freeMove() const { return m_freeMove; }

    // Blocks currently selected for moving (Ctrl+click, rubber band or Ctrl+A).
    QSet<int> selectedGroups() const { return m_selectedGroups; }
    void selectAllGroups();

signals:
    void drawerSelected(const Drawer &drawer);
    void drawerActivated(const Drawer &drawer); // Double click.
    // `drawerId` is the drawer under the cursor (0 when none).
    void groupContextRequested(const DrawerGroup &group, int drawerId, const QPoint &globalPos);
    // Items dropped on a drawer; `copy` when Ctrl was held.
    void itemsDropped(const QList<int> &itemIds, int drawerId, bool copy);
    // Blocks dropped at a new place, carrying their new positions.
    void groupsMoved(const QList<DrawerGroup> &groups);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    struct Hit {
        const GroupLayout *layout = nullptr;
        const Drawer *drawer = nullptr;
    };

    QRect cellRect(const DrawerGroup &group, int row, int col) const;
    QRect groupRect(const DrawerGroup &group) const;
    Hit hitTest(const QPoint &pos) const;
    void drawGroupTag(QPainter &p, const DrawerGroup &group, const QList<Drawer> &drawers) const;
    void updateDrag(const QPoint &pos);
    void updateBand(const QPoint &pos);
    void cancelDrag();
    void setSelectedGroups(const QSet<int> &ids);
    // Where a block is drawn: its stored position, or the drag preview.
    DrawerGroup displayed(const DrawerGroup &group) const;
    QList<DrawerGroup> unselectedGroups() const;

    QList<GroupLayout> m_layouts;
    int m_selectedDrawerId = 0;
    QSet<int> m_highlighted;
    QSet<int> m_lowStock;
    QSet<int> m_emphasized;
    int m_dropDrawerId = 0; // Drawer under an item being dragged over the view.
    QSet<int> m_selectedGroups;
    bool m_freeMove = false;
    QPoint m_pressPos;
    int m_pressedGroupId = 0; // Block under the mouse button; the anchor of a drag.
    int m_collapseTo = 0;     // Block to select alone if the press turns out to be a plain click.

    // Dragging the selected blocks; starts after a small movement.
    bool m_dragging = false;
    bool m_dragValid = false;
    QHash<int, Layout::Position> m_dragOrigins; // Selected block id -> position at drag start.
    Layout::Position m_dragShift;

    // Rubber band selection started on empty space.
    bool m_banding = false;
    QRect m_bandRect;
    QSet<int> m_bandBase; // Selection kept when Ctrl was held at the start.
};
