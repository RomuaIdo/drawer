#pragma once

#include <QHash>
#include <QSet>
#include <QWidget>

#include "core/Models.h"

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

signals:
    void drawerSelected(const Drawer &drawer);
    void drawerActivated(const Drawer &drawer); // Double click.
    void groupContextRequested(const DrawerGroup &group, const QPoint &globalPos);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    struct Hit {
        const GroupLayout *layout = nullptr;
        const Drawer *drawer = nullptr;
    };

    QRect cellRect(const DrawerGroup &group, int row, int col) const;
    QRect groupRect(const DrawerGroup &group) const;
    Hit hitTest(const QPoint &pos) const;

    QList<GroupLayout> m_layouts;
    int m_selectedDrawerId = 0;
    QSet<int> m_highlighted;
};
