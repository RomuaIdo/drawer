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
    QSize sizeHint() const override;

    // Free mode drops the grid snapping while dragging blocks.
    void setFreeMove(bool free) { m_freeMove = free; }
    bool freeMove() const { return m_freeMove; }

signals:
    void drawerSelected(const Drawer &drawer);
    void drawerActivated(const Drawer &drawer); // Double click.
    void groupContextRequested(const DrawerGroup &group, const QPoint &globalPos);
    void groupMoved(const DrawerGroup &group, double x, double y);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

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
    void cancelDrag();
    QList<DrawerGroup> otherGroups(int excludedId) const;

    QList<GroupLayout> m_layouts;
    int m_selectedDrawerId = 0;
    QSet<int> m_highlighted;

    // Block drag state: a press on a drawer becomes a drag after a small movement.
    bool m_freeMove = false;
    bool m_dragging = false;
    bool m_dragValid = false;
    DrawerGroup m_dragGroup; // Candidate placement while dragging; id 0 when idle.
    Layout::Position m_dragOrigin;
    QPoint m_pressPos;
};
