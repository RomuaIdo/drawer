#include "CabinetView.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <cmath>

#include "ItemMime.h"

namespace {

constexpr int kCellWidth = 72;
constexpr int kCellHeight = 52;
constexpr int kMargin = 12;
constexpr int kCellPadding = 2;
constexpr int kTagDarkening = 190; // Percent; 100 keeps the drawer color.
constexpr int kSelectionDarkening = 200;
constexpr int kEmphasisDarkening = 140;

QColor sizeColor(DrawerSize size)
{
    switch (size) {
    case DrawerSize::Small: return QColor(0xc8, 0xe6, 0xc9);
    case DrawerSize::Medium: return QColor(0xff, 0xe0, 0x82);
    case DrawerSize::Large: return QColor(0xef, 0x9a, 0x9a);
    }
    return Qt::white;
}

// Group color when set, otherwise the color of the drawer's size.
QColor drawerColor(const DrawerGroup &group, DrawerSize size)
{
    const QColor custom(group.color);
    return custom.isValid() ? custom : sizeColor(size);
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
    setMouseTracking(true);
    setAcceptDrops(true);
}

void CabinetView::setLayouts(const QList<GroupLayout> &layouts)
{
    cancelDrag();
    m_banding = false;
    m_layouts = layouts;
    QSet<int> surviving;
    for (const GroupLayout &l : m_layouts) {
        if (m_selectedGroups.contains(l.group.id))
            surviving.insert(l.group.id);
    }
    m_selectedGroups = surviving;
    setMinimumSize(sizeHint());
    updateGeometry();
    update();
}

void CabinetView::setSelectedDrawer(int drawerId)
{
    m_selectedDrawerId = drawerId;
    update();
}

void CabinetView::setEmphasizedDrawers(const QSet<int> &drawerIds)
{
    if (drawerIds == m_emphasized)
        return;
    m_emphasized = drawerIds;
    update();
}

QRect CabinetView::drawerRect(int drawerId) const
{
    for (const GroupLayout &l : m_layouts) {
        for (const Drawer &d : l.drawers) {
            if (d.id == drawerId)
                return cellRect(l.group, d.row, d.col);
        }
    }
    return {};
}

void CabinetView::setPickerMode(bool picker)
{
    m_picker = picker;
    setAcceptDrops(!picker);
    setFocusPolicy(picker ? Qt::NoFocus : Qt::StrongFocus);
}

void CabinetView::setLowStockDrawers(const QSet<int> &drawerIds)
{
    m_lowStock = drawerIds;
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

bool CabinetView::event(QEvent *event)
{
    if (event->type() != QEvent::ToolTip)
        return QWidget::event(event);

    const auto *help = static_cast<QHelpEvent *>(event);
    const Hit hit = hitTest(help->pos());
    if (hit.drawer) {
        QString text = tr("%1 / %2 (%3)").arg(hit.layout->group.name, hit.drawer->label,
                                              drawerSizeToString(hit.drawer->size));
        if (m_lowStock.contains(hit.drawer->id))
            text += tr("\nLow stock");
        QToolTip::showText(help->globalPos(), text, this);
    } else {
        QToolTip::hideText();
    }
    return true;
}

// Name tag in the top-left corner of a block, so the group is visible without any click.
void CabinetView::drawGroupTag(QPainter &p, const DrawerGroup &group,
                               const QList<Drawer> &drawers) const
{
    p.save();
    QFont font = p.font();
    font.setPointSizeF(font.pointSizeF() * 0.8);
    font.setBold(true);
    p.setFont(font);

    const QRect block = groupRect(group);
    const QFontMetrics metrics(font);
    const QString text = metrics.elidedText(group.name, Qt::ElideRight, block.width() - 12);
    const QRect tag(block.left() + 1, block.top() + 1, metrics.horizontalAdvance(text) + 8,
                    metrics.height() + 2);

    // Same hue as the drawer under the tag, only darker.
    const DrawerSize size = drawers.isEmpty() ? group.defaultSize : drawers.first().size;
    p.setPen(Qt::NoPen);
    p.setBrush(drawerColor(group, size).darker(kTagDarkening));
    p.drawRoundedRect(tag, 3, 3);
    p.setPen(Qt::white);
    p.drawText(tag, Qt::AlignCenter, text);
    p.restore();
}

void CabinetView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().window());

    // Selection outlines are drawn after everything else so no neighbor can cover them.
    QRect selectedCell;
    QColor selectedCellColor;
    struct BlockOutline {
        QRect rect;
        QColor color;
    };
    QList<BlockOutline> selectedBlocks;
    for (const GroupLayout &l : m_layouts) {
        const bool beingDragged = m_dragging && m_dragOrigins.contains(l.group.id);
        const DrawerGroup group = displayed(l.group);
        p.setOpacity(beingDragged ? 0.7 : 1.0);

        for (const Drawer &d : l.drawers) {
            const QRect cell = cellRect(group, d.row, d.col)
                                   .adjusted(kCellPadding, kCellPadding, -kCellPadding, -kCellPadding);
            QColor fill = drawerColor(group, d.size);
            if (m_emphasized.contains(d.id) || m_highlighted.contains(d.id))
                fill = fill.darker(kEmphasisDarkening); // Picked in a list, or matching the search.
            if (d.id == m_selectedDrawerId) {
                selectedCell = cell;
                selectedCellColor = fill.darker(kSelectionDarkening);
            }
            const bool darkFill = fill.lightness() < 140;
            p.setPen(QPen(Qt::darkGray, 1));
            p.setBrush(fill);
            p.drawRoundedRect(cell, 4, 4);
            if (m_lowStock.contains(d.id)) {
                const QRect badge(cell.right() - 15, cell.top() + 3, 13, 13);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0xd3, 0x20, 0x20));
                p.drawEllipse(badge);
                p.setPen(Qt::white);
                p.drawText(badge, Qt::AlignCenter, "!");
            }

            // Keep text readable on custom, possibly dark, colors.
            p.setPen(darkFill ? Qt::white : Qt::black);
            p.drawText(cell.adjusted(4, 2, -4, -2), Qt::AlignCenter | Qt::TextWordWrap, d.label);
            p.setPen(darkFill ? QColor(0xd0, 0xd0, 0xd0) : QColor(Qt::darkGray));
            p.drawText(cell.adjusted(4, 2, -4, -2), Qt::AlignRight | Qt::AlignBottom,
                       sizeLetter(d.size));
        }
        drawGroupTag(p, group, l.drawers);

        // Block outline makes grouped drawers easy to tell apart. Selected blocks use the
        // highlight color; a drag that would overlap another block turns red.
        if (m_selectedGroups.contains(l.group.id)) {
            selectedBlocks.append({groupRect(group), beingDragged && !m_dragValid
                                                         ? QColor(0xd3, 0x20, 0x20)
                                                         : palette().color(QPalette::Highlight)});
        } else {
            p.setPen(QPen(Qt::black, 2));
            p.setBrush(Qt::NoBrush);
            p.drawRect(groupRect(group));
        }
    }
    p.setOpacity(1.0);

    p.setBrush(Qt::NoBrush);
    for (const BlockOutline &outline : selectedBlocks) {
        p.setPen(QPen(outline.color, 3));
        p.drawRect(outline.rect);
    }

    if (m_dropDrawerId) {
        for (const GroupLayout &l : m_layouts) {
            for (const Drawer &d : l.drawers) {
                if (d.id != m_dropDrawerId)
                    continue;
                p.setPen(QPen(palette().color(QPalette::Highlight), 3, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(cellRect(displayed(l.group), d.row, d.col)
                                      .adjusted(kCellPadding, kCellPadding, -kCellPadding, -kCellPadding),
                                  4, 4);
            }
        }
    }

    if (!selectedCell.isNull()) {
        p.setPen(QPen(selectedCellColor, 3));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(selectedCell, 4, 4);
    }

    if (m_banding) {
        QColor fill = palette().color(QPalette::Highlight);
        fill.setAlpha(50);
        p.setPen(palette().color(QPalette::Highlight));
        p.setBrush(fill);
        p.drawRect(m_bandRect);
    }
}

void CabinetView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    const Hit hit = hitTest(event->pos());
    if (m_picker) {
        if (hit.drawer) {
            setSelectedDrawer(hit.drawer->id);
            emit drawerSelected(*hit.drawer);
        }
        return;
    }
    setFocus();
    const bool additive = event->modifiers() & Qt::ControlModifier;
    m_pressPos = event->pos();
    m_pressedGroupId = 0;
    m_collapseTo = 0;

    // Empty space starts a rubber band.
    if (!hit.layout) {
        m_banding = true;
        m_bandBase = additive ? m_selectedGroups : QSet<int>();
        m_bandRect = QRect(event->pos(), event->pos());
        setSelectedGroups(m_bandBase);
        return;
    }

    const int id = hit.layout->group.id;
    if (additive) {
        QSet<int> selection = m_selectedGroups;
        if (!selection.remove(id))
            selection.insert(id);
        setSelectedGroups(selection);
        if (selection.contains(id))
            m_pressedGroupId = id;
        return;
    }

    if (hit.drawer)
        emit drawerSelected(*hit.drawer);
    if (!m_selectedGroups.contains(id))
        setSelectedGroups({id});
    else if (m_selectedGroups.size() > 1)
        m_collapseTo = id; // Becomes the only selection unless this turns into a drag.
    m_pressedGroupId = id;
}

void CabinetView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_banding && (event->buttons() & Qt::LeftButton)) {
        updateBand(event->pos());
        return;
    }
    if (!(event->buttons() & Qt::LeftButton) || m_pressedGroupId == 0)
        return;
    if (!m_dragging) {
        if ((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
            return;
        m_dragging = true;
        m_collapseTo = 0;
        m_dragOrigins.clear();
        for (const GroupLayout &l : m_layouts) {
            if (m_selectedGroups.contains(l.group.id))
                m_dragOrigins.insert(l.group.id, {l.group.posX, l.group.posY});
        }
        setCursor(Qt::ClosedHandCursor);
    }
    updateDrag(event->pos());
}

void CabinetView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    if (m_banding) {
        m_banding = false;
        update();
        return;
    }

    const bool commit = m_dragging && m_dragValid && (m_dragShift.x != 0 || m_dragShift.y != 0);
    QList<DrawerGroup> moved;
    if (commit) {
        for (const GroupLayout &l : m_layouts) {
            if (m_dragOrigins.contains(l.group.id))
                moved.append(displayed(l.group));
        }
    }
    const int collapse = m_collapseTo;
    cancelDrag();
    m_pressedGroupId = 0;
    m_collapseTo = 0;

    if (commit)
        emit groupsMoved(moved);
    else if (collapse)
        setSelectedGroups({collapse});
}

void CabinetView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_dragging || m_banding) {
            cancelDrag();
            m_banding = false;
        } else {
            setSelectedGroups({});
        }
    } else if (event->matches(QKeySequence::SelectAll)) {
        selectAllGroups();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void CabinetView::updateDrag(const QPoint &pos)
{
    const double dx = double(pos.x() - m_pressPos.x()) / kCellWidth;
    const double dy = double(pos.y() - m_pressPos.y()) / kCellHeight;
    const QList<Layout::Position> origins = m_dragOrigins.values();
    m_dragShift = Layout::dragDelta(origins, m_dragOrigins.value(m_pressedGroupId), dx, dy,
                                    !m_freeMove);

    // Moved blocks keep their offsets, so only the ones left behind can be hit.
    const QList<DrawerGroup> fixed = unselectedGroups();
    m_dragValid = true;
    for (const GroupLayout &l : m_layouts) {
        if (m_dragOrigins.contains(l.group.id) && Layout::collides(displayed(l.group), fixed))
            m_dragValid = false;
    }
    update();
}

void CabinetView::updateBand(const QPoint &pos)
{
    m_bandRect = QRect(m_pressPos, pos).normalized();
    QSet<int> selection = m_bandBase;
    for (const GroupLayout &l : m_layouts) {
        if (groupRect(l.group).intersects(m_bandRect))
            selection.insert(l.group.id);
    }
    setSelectedGroups(selection);
    update(); // The band itself moved even when the selection did not change.
}

void CabinetView::cancelDrag()
{
    m_dragging = false;
    m_dragValid = false;
    m_dragOrigins.clear();
    m_dragShift = {};
    unsetCursor();
    update();
}

void CabinetView::selectAllGroups()
{
    QSet<int> all;
    for (const GroupLayout &l : m_layouts)
        all.insert(l.group.id);
    setSelectedGroups(all);
}

void CabinetView::setSelectedGroups(const QSet<int> &ids)
{
    if (ids == m_selectedGroups)
        return;
    m_selectedGroups = ids;
    update();
}

DrawerGroup CabinetView::displayed(const DrawerGroup &group) const
{
    DrawerGroup result = group;
    const auto origin = m_dragOrigins.constFind(group.id);
    if (m_dragging && origin != m_dragOrigins.constEnd()) {
        result.posX = origin->x + m_dragShift.x;
        result.posY = origin->y + m_dragShift.y;
    }
    return result;
}

QList<DrawerGroup> CabinetView::unselectedGroups() const
{
    QList<DrawerGroup> groups;
    for (const GroupLayout &l : m_layouts) {
        if (!m_selectedGroups.contains(l.group.id))
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
    if (m_picker)
        return;
    const Hit hit = hitTest(event->pos());
    if (hit.layout && !m_selectedGroups.contains(hit.layout->group.id))
        setSelectedGroups({hit.layout->group.id});
    if (hit.layout)
        emit groupContextRequested(hit.layout->group, hit.drawer ? hit.drawer->id : 0,
                                   event->globalPos());
}

void CabinetView::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_picker)
        return;
    if (ItemMime::accepts(event->mimeData()))
        event->acceptProposedAction();
}

void CabinetView::dragMoveEvent(QDragMoveEvent *event)
{
    const Hit hit = hitTest(event->position().toPoint());
    const int target = ItemMime::accepts(event->mimeData()) && hit.drawer ? hit.drawer->id : 0;
    if (target != m_dropDrawerId) {
        m_dropDrawerId = target;
        update();
    }
    if (target)
        event->acceptProposedAction();
    else
        event->ignore();
}

void CabinetView::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dropDrawerId = 0;
    update();
    event->accept();
}

void CabinetView::dropEvent(QDropEvent *event)
{
    const Hit hit = hitTest(event->position().toPoint());
    const bool copy = event->dropAction() == Qt::CopyAction;
    const QList<int> ids = ItemMime::decode(event->mimeData());
    m_dropDrawerId = 0;
    update();
    if (!hit.drawer || ids.isEmpty()) {
        event->ignore();
        return;
    }
    // Reported as a copy so the source view never tries to remove its own rows.
    event->setDropAction(Qt::CopyAction);
    event->accept();
    emit itemsDropped(ids, hit.drawer->id, copy);
}
