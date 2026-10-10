#include "CabinetView.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <cmath>

namespace {

constexpr int kCellWidth = 72;
constexpr int kCellHeight = 52;
constexpr int kMargin = 12;
constexpr int kCellPadding = 2;

QColor sizeColor(DrawerSize size)
{
    switch (size) {
    case DrawerSize::Small: return QColor(0xc8, 0xe6, 0xc9);
    case DrawerSize::Medium: return QColor(0xff, 0xe0, 0x82);
    case DrawerSize::Large: return QColor(0xef, 0x9a, 0x9a);
    }
    return Qt::white;
}

QString sizeLetter(DrawerSize size)
{
    return drawerSizeToString(size).left(1).toUpper();
}

} // namespace

CabinetView::CabinetView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(sizeHint());
    setFocusPolicy(Qt::StrongFocus);
}

void CabinetView::setLayouts(const QList<GroupLayout> &layouts)
{
    cancelDrag();
    m_layouts = layouts;
    setMinimumSize(sizeHint());
    updateGeometry();
    update();
}

void CabinetView::setSelectedDrawer(int drawerId)
{
    m_selectedDrawerId = drawerId;
    update();
}

void CabinetView::setHighlightedDrawers(const QSet<int> &drawerIds)
{
    m_highlighted = drawerIds;
    update();
}

QSize CabinetView::sizeHint() const
{
    int maxX = 6;
    int maxY = 4;
    for (const GroupLayout &l : m_layouts) {
        maxX = std::max(maxX, int(std::ceil(l.group.posX + l.group.cols)));
        maxY = std::max(maxY, int(std::ceil(l.group.posY + l.group.rows)));
    }
    return {2 * kMargin + maxX * kCellWidth, 2 * kMargin + maxY * kCellHeight};
}

QRect CabinetView::cellRect(const DrawerGroup &group, int row, int col) const
{
    return QRect(kMargin + qRound((group.posX + col) * kCellWidth),
                 kMargin + qRound((group.posY + row) * kCellHeight), kCellWidth, kCellHeight);
}

QRect CabinetView::groupRect(const DrawerGroup &group) const
{
    return cellRect(group, 0, 0).united(cellRect(group, group.rows - 1, group.cols - 1));
}

CabinetView::Hit CabinetView::hitTest(const QPoint &pos) const
{
    for (const GroupLayout &l : m_layouts) {
        if (!groupRect(l.group).contains(pos))
            continue;
        for (const Drawer &d : l.drawers) {
            if (cellRect(l.group, d.row, d.col).contains(pos))
                return {&l, &d};
        }
        return {&l, nullptr};
    }
    return {};
}

void CabinetView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().window());

    for (const GroupLayout &l : m_layouts) {
        const bool beingDragged = m_dragging && l.group.id == m_dragGroup.id;
        const DrawerGroup &group = beingDragged ? m_dragGroup : l.group;
        p.setOpacity(beingDragged ? 0.7 : 1.0);

        for (const Drawer &d : l.drawers) {
            const QRect cell = cellRect(group, d.row, d.col)
                                   .adjusted(kCellPadding, kCellPadding, -kCellPadding, -kCellPadding);
            const bool selected = d.id == m_selectedDrawerId;
            const bool highlighted = m_highlighted.contains(d.id);

            p.setPen(QPen(selected ? Qt::blue : Qt::darkGray, selected ? 3 : 1));
            p.setBrush(sizeColor(d.size));
            p.drawRoundedRect(cell, 4, 4);
            if (highlighted) {
                p.setPen(QPen(QColor(0xd8, 0x1b, 0x60), 3));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(cell.adjusted(2, 2, -2, -2), 3, 3);
            }

            p.setPen(Qt::black);
            p.drawText(cell.adjusted(4, 2, -4, -2), Qt::AlignCenter | Qt::TextWordWrap, d.label);
            p.setPen(Qt::darkGray);
            p.drawText(cell.adjusted(4, 2, -4, -2), Qt::AlignRight | Qt::AlignBottom,
                       sizeLetter(d.size));
        }
        // Block outline makes grouped drawers easy to tell apart.
        // Dragged blocks are outlined red when the drop would overlap another block.
        const QColor outline = beingDragged && !m_dragValid ? QColor(0xd3, 0x20, 0x20) : Qt::black;
        p.setPen(QPen(outline, beingDragged ? 3 : 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(groupRect(group));
    }
    p.setOpacity(1.0);
}

void CabinetView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    const Hit hit = hitTest(event->pos());
    if (hit.drawer)
        emit drawerSelected(*hit.drawer);
    if (hit.layout) {
        m_dragGroup = hit.layout->group;
        m_dragOrigin = {m_dragGroup.posX, m_dragGroup.posY};
        m_pressPos = event->pos();
    }
}

void CabinetView::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_dragGroup.id == 0)
        return;
    if (!m_dragging
        && (event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
        return;
    m_dragging = true;
    setCursor(Qt::ClosedHandCursor);
    updateDrag(event->pos());
}

void CabinetView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    const bool commit = m_dragging && m_dragValid
        && (m_dragGroup.posX != m_dragOrigin.x || m_dragGroup.posY != m_dragOrigin.y);
    const DrawerGroup moved = m_dragGroup;
    cancelDrag();
    if (commit)
        emit groupMoved(moved, moved.posX, moved.posY);
}

void CabinetView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_dragging)
        cancelDrag();
    else
        QWidget::keyPressEvent(event);
}

void CabinetView::updateDrag(const QPoint &pos)
{
    const double dx = double(pos.x() - m_pressPos.x()) / kCellWidth;
    const double dy = double(pos.y() - m_pressPos.y()) / kCellHeight;
    const Layout::Position target = Layout::dragTarget(m_dragOrigin, dx, dy, !m_freeMove);
    m_dragGroup.posX = target.x;
    m_dragGroup.posY = target.y;
    m_dragValid = !Layout::collides(m_dragGroup, otherGroups(m_dragGroup.id));
    update();
}

void CabinetView::cancelDrag()
{
    m_dragging = false;
    m_dragValid = false;
    m_dragGroup = {};
    unsetCursor();
    update();
}

QList<DrawerGroup> CabinetView::otherGroups(int excludedId) const
{
    QList<DrawerGroup> groups;
    for (const GroupLayout &l : m_layouts) {
        if (l.group.id != excludedId)
            groups.append(l.group);
    }
    return groups;
}

void CabinetView::mouseDoubleClickEvent(QMouseEvent *event)
{
    const Hit hit = hitTest(event->pos());
    if (event->button() == Qt::LeftButton && hit.drawer)
        emit drawerActivated(*hit.drawer);
}

void CabinetView::contextMenuEvent(QContextMenuEvent *event)
{
    const Hit hit = hitTest(event->pos());
    if (hit.layout)
        emit groupContextRequested(hit.layout->group, event->globalPos());
}
