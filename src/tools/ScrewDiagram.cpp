#include "ScrewDiagram.h"

#include <QPainter>
#include <QPainterPath>

namespace {

const QColor kDimension(190, 60, 120);
// The slab the head sits in; the margin to its left holds the depth label.
constexpr double kBlockLeft = 66, kBlockRight = 238;

struct Palette {
    QColor body, section, outline, text;
};

Palette paletteFor(const QWidget *w)
{
    const QPalette &p = w->palette();
    return {p.color(QPalette::Mid), p.color(QPalette::Midlight), p.color(QPalette::WindowText),
            p.color(QPalette::WindowText)};
}

void arrowHead(QPainter &p, QPointF tip, QPointF from)
{
    const QPointF d = (tip - from) / std::hypot(tip.x() - from.x(), tip.y() - from.y());
    const QPointF n(-d.y(), d.x());
    p.setBrush(kDimension);
    p.drawPolygon(QPolygonF{tip, tip - d * 6 + n * 2.5, tip - d * 6 - n * 2.5});
}

// A dimension line with arrows at both ends.
void dimension(QPainter &p, QPointF a, QPointF b)
{
    p.setPen(QPen(kDimension, 1));
    p.drawLine(a, b);
    arrowHead(p, a, b);
    arrowHead(p, b, a);
}

void extension(QPainter &p, QPointF a, QPointF b)
{
    p.setPen(QPen(kDimension, 1, Qt::DotLine));
    p.drawLine(a, b);
}

void label(QPainter &p, const QRectF &rect, int flags, const QString &text)
{
    // A background patch keeps the dotted lines from running through the words.
    const QRectF used = p.boundingRect(rect, flags | Qt::TextWordWrap, text).adjusted(-2, 0, 2, 0);
    p.fillRect(used, p.background());
    p.setPen(kDimension);
    p.drawText(rect, flags | Qt::TextWordWrap, text);
}

// Horizontal measure `text` between x1 and x2 at height y, with extension lines from `from`.
void widthOf(QPainter &p, double x1, double x2, double y, double from, const QString &text,
             bool textBelow = false)
{
    extension(p, {x1, from}, {x1, y});
    extension(p, {x2, from}, {x2, y});
    dimension(p, {x1, y}, {x2, y});
    const QRectF box((x1 + x2) / 2 - 60, textBelow ? y + 2 : y - 16, 120, 14);
    label(p, box, Qt::AlignCenter, text);
}

// Vertical measure between y1 and y2 at x, with extension lines from `from`; the text goes
// to the right (or left) of the line.
void heightOf(QPainter &p, double y1, double y2, double x, double from, const QString &text,
              bool textRight, double textWidth = 50)
{
    extension(p, {from, y1}, {x, y1});
    extension(p, {from, y2}, {x, y2});
    dimension(p, {x, y1}, {x, y2});
    const double mid = (y1 + y2) / 2;
    const QRectF box(textRight ? x + 4 : x - textWidth - 4, mid - 14, textWidth, 28);
    label(p, box, (textRight ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter, text);
}

} // namespace

ScrewDiagram::ScrewDiagram(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(sizeHint());
}

void ScrewDiagram::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBackground(palette().color(QPalette::Window));
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() - 1);
    p.setFont(small);
    const Palette c = paletteFor(this);

    auto title = [&](const QString &text, double width) {
        p.setPen(c.text);
        p.drawText(QRectF(0, 0, width, 18), Qt::AlignCenter, text);
    };

    // --- Thread, seen from the side ---------------------------------------------------------
    {
        const double cy = 125, major = 48, minor = 36, pitch = 26, left = 135;
        const int teeth = 6;
        // One period: root flat, flank up, crest flat, flank down (60 degrees between flanks).
        const double rootFlat = 5.2, flank = 7.5;
        QPainterPath body;
        body.moveTo(left, cy - minor);
        for (int i = 0; i < teeth; ++i) {
            const double x = left + i * pitch;
            body.lineTo(x + rootFlat, cy - minor);
            body.lineTo(x + rootFlat + flank, cy - major);
            body.lineTo(x + pitch - flank, cy - major);
            body.lineTo(x + pitch, cy - minor);
        }
        const double right = left + teeth * pitch;
        body.lineTo(right, cy + minor);
        for (int i = teeth - 1; i >= 0; --i) {
            const double x = left + i * pitch;
            body.lineTo(x + pitch, cy + minor);
            body.lineTo(x + pitch - flank, cy + major);
            body.lineTo(x + rootFlat + flank, cy + major);
            body.lineTo(x + rootFlat, cy + minor);
            body.lineTo(x, cy + minor);
        }
        body.closeSubpath();
        p.setPen(QPen(c.outline, 1));
        p.setBrush(c.body);
        p.drawPath(body);

        title(tr("Thread"), kThreadWidth);

        const double firstCrest = left + rootFlat + flank;
        heightOf(p, cy - major, cy + major, 92, firstCrest, tr("Major diameter\n(nominal)"), false);
        heightOf(p, cy - minor, cy + minor, 122, left, QString(), true);
        extension(p, {122, cy + minor}, {122, cy + major + 6});
        label(p, QRectF(122 - 95, cy + major + 6, 190, 28), Qt::AlignHCenter | Qt::AlignTop,
              tr("Minor diameter\n(about the tap drill)"));

        // Pitch: crest to crest along the bottom.
        const double c1 = left + 3 * pitch + pitch / 2, c2 = c1 + pitch, yDim = cy + major + 12;
        extension(p, {c1, cy + major}, {c1, yDim + 4});
        extension(p, {c2, cy + major}, {c2, yDim + 4});
        dimension(p, {c1, yDim}, {c2, yDim});
        label(p, QRectF(c1 - 30, yDim + 2, 80, 14), Qt::AlignLeft, tr("Pitch"));

        // Thread angle: the flanks of one tooth, extended to where they meet.
        const double tx = left + 4 * pitch, slope = flank / (major - minor);
        const double apexY = cy - major - (pitch - 2 * flank) / 2 / slope;
        const QPointF apex(tx + pitch / 2, apexY);
        p.setPen(QPen(kDimension, 1.2, Qt::DashLine));
        p.drawLine(QPointF(tx + rootFlat, cy - minor), apex + QPointF(14 * slope, -14));
        p.drawLine(QPointF(tx + pitch, cy - minor), apex + QPointF(-14 * slope, -14));
        label(p, QRectF(apex.x() - 40, apex.y() - 36, 80, 14), Qt::AlignCenter, tr("Thread angle"));
    }

    // --- Heads in a section through their hole ----------------------------------------------
    const double top = 78, bottom = 190, cx = (kBlockLeft + kBlockRight) / 2;
    const double shaft = 12, hole = 14;
    auto drawBlock = [&](double offset, const QPainterPath &material) {
        p.save();
        p.translate(offset, 0);
        p.setPen(QPen(c.outline, 1));
        p.setBrush(c.section);
        p.drawPath(material);
        p.restore();
    };
    auto block = [&](std::initializer_list<QPointF> hollow) {
        // Material is the slab with the hole outline cut out of its top.
        QPainterPath path;
        path.moveTo(kBlockLeft, top);
        for (const QPointF &pt : hollow)
            path.lineTo(pt);
        path.lineTo(kBlockRight, top);
        path.lineTo(kBlockRight, bottom);
        path.lineTo(kBlockLeft, bottom);
        path.closeSubpath();
        return path;
    };

    // Cylindrical head: counterbored hole.
    {
        const double offset = kThreadWidth, cb = 30, depth = 30, head = 26, headTop = top + 4;
        drawBlock(offset, block({{cx - cb, top}, {cx - cb, top + depth}, {cx - hole, top + depth},
                                 {cx - hole, bottom}, {cx + hole, bottom},
                                 {cx + hole, top + depth}, {cx + cb, top + depth},
                                 {cx + cb, top}}));
        p.save();
        p.translate(offset, 0);
        p.setPen(QPen(c.outline, 1));
        p.setBrush(c.body);
        p.drawRect(QRectF(cx - 24, headTop, 48, head));                  // Head.
        p.drawRect(QRectF(cx - shaft, headTop + head, 2 * shaft, bottom + 8 - headTop - head));
        p.setPen(c.text);
        p.drawText(QRectF(0, 0, kHeadWidth, 18), Qt::AlignCenter, tr("Cylindrical head"));

        widthOf(p, cx - 24, cx + 24, top - 14, headTop, tr("Head diameter"));
        widthOf(p, cx - cb, cx + cb, top - 38, top, tr("Counterbore diameter"));
        heightOf(p, headTop, headTop + head, cx + cb + 14, cx + 24, tr("Head height"), true);
        heightOf(p, top, top + depth, cx - cb - 12, cx - cb, tr("Depth"), false, 40);
        widthOf(p, cx - hole, cx + hole, bottom + 14, bottom, tr("Clearance hole"), true);
        p.restore();
    }

    // Conical head: countersunk hole.
    {
        const double offset = kThreadWidth + kHeadWidth, sink = 34, cone = 30, depth = 20;
        drawBlock(offset, block({{cx - sink, top}, {cx - hole, top + depth}, {cx - hole, bottom},
                                 {cx + hole, bottom}, {cx + hole, top + depth}, {cx + sink, top}}));
        p.save();
        p.translate(offset, 0);
        p.setPen(QPen(c.outline, 1));
        p.setBrush(c.body);
        QPainterPath screw;
        screw.moveTo(cx - cone, top + 1);
        screw.lineTo(cx + cone, top + 1);
        screw.lineTo(cx + shaft, top + 1 + (cone - shaft));
        screw.lineTo(cx + shaft, bottom + 8);
        screw.lineTo(cx - shaft, bottom + 8);
        screw.lineTo(cx - shaft, top + 1 + (cone - shaft));
        screw.closeSubpath();
        p.drawPath(screw);
        p.setPen(c.text);
        p.drawText(QRectF(0, 0, kHeadWidth, 18), Qt::AlignCenter, tr("Conical head"));

        widthOf(p, cx - cone, cx + cone, top - 14, top, tr("Head diameter"));
        widthOf(p, cx - sink, cx + sink, top - 38, top, tr("Countersink diameter"));
        widthOf(p, cx - hole, cx + hole, bottom + 14, bottom, tr("Clearance hole"), true);
        p.restore();
    }
}
