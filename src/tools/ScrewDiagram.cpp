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
    p.setPen(kDimension);
    p.drawText(rect, flags, text);
}

enum class TextAt { Centered, Left, Right };

// Horizontal measure `text` between x1 and x2 at height y, with extension lines from `from`.
// The text sits over the line, centered or beside the extension lines (so they never cross it),
// or under it.
void widthOf(QPainter &p, double x1, double x2, double y, double from, const QString &text,
             TextAt at = TextAt::Centered, bool textBelow = false)
{
    extension(p, {x1, from}, {x1, y});
    extension(p, {x2, from}, {x2, y});
    dimension(p, {x1, y}, {x2, y});
    const double h = p.fontMetrics().height();
    const double top = textBelow ? y + 2 : y - h - 1;
    const QRectF box = at == TextAt::Left   ? QRectF(x1 - 12 - 200, top, 200, h)
                       : at == TextAt::Right ? QRectF(x2 + 12, top, 200, h)
                                             : QRectF((x1 + x2) / 2 - 100, top, 200, h);
    label(p, box, at == TextAt::Left ? Qt::AlignRight : at == TextAt::Right ? Qt::AlignLeft : Qt::AlignCenter, text);
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
    const double h = 2 * p.fontMetrics().height();
    const QRectF box(textRight ? x + 4 : x - textWidth - 4, mid - h / 2, textWidth, h);
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
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() - 1);
    p.setFont(small);
    const Palette c = paletteFor(this);

    // Titles are centered over the drawing they name.
    auto title = [&](const QString &text, double center) {
        p.setPen(c.text);
        p.drawText(QRectF(center - 100, 0, 200, 18), Qt::AlignCenter, text);
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

        title(tr("Thread"), 190);

        const double firstCrest = left + rootFlat + flank;
        heightOf(p, cy - major, cy + major, 92, firstCrest, tr("Major\ndiameter"), false);
        heightOf(p, cy - minor, cy + minor, 122, left, QString(), true);
        extension(p, {122, cy + minor}, {122, cy + major + 6});
        label(p, QRectF(122 - 95, cy + major + 6, 190, 2 * p.fontMetrics().height()),
              Qt::AlignHCenter | Qt::AlignTop,
              tr("Minor diameter\n(about the tap drill)"));

        // Pitch: crest to crest along the bottom.
        const double c1 = left + 3 * pitch + pitch / 2, c2 = c1 + pitch, yDim = cy + major + 12;
        extension(p, {c1, cy + major}, {c1, yDim + 4});
        extension(p, {c2, cy + major}, {c2, yDim + 4});
        dimension(p, {c1, yDim}, {c2, yDim});
        label(p, QRectF(c1 - 30, yDim + 2, 80, 14), Qt::AlignLeft, tr("Pitch"));

        // Thread angle: the walls of one groove extended upward, with an arc between them.
        const double grooveX = left + 3 * pitch + pitch + rootFlat / 2, slope = flank / (major - minor);
        const double vertexY = cy - minor + (rootFlat / 2) / slope; // Where the walls meet.
        const double rise = 26;
        p.setPen(QPen(kDimension, 1));
        for (const int side : {-1, 1}) {
            const double y1 = cy - major, y2 = cy - major - rise;
            p.drawLine(QPointF(grooveX + side * slope * (vertexY - y1), y1),
                       QPointF(grooveX + side * slope * (vertexY - y2), y2));
        }
        const double radius = vertexY - (cy - major - rise + 8);
        const double half = std::atan(slope);
        const QRectF circle(grooveX - radius, vertexY - radius, 2 * radius, 2 * radius);
        constexpr double kRad = 180.0 / M_PI;
        p.setBrush(Qt::NoBrush);
        p.drawArc(circle, int((90 - half * kRad) * 16), int(2 * half * kRad * 16));
        for (const int side : {-1, 1}) {
            // Arrow at each end of the arc, pointing along its tangent.
            const double theta = M_PI / 2 + side * half;
            const QPointF tip(grooveX + radius * std::cos(theta), vertexY - radius * std::sin(theta));
            const QPointF tangent(-side * std::sin(theta), -side * std::cos(theta)); // Outward.
            arrowHead(p, tip, tip - tangent * 12);
        }
        label(p, QRectF(grooveX - 100, vertexY - radius - p.fontMetrics().height() - 4, 200,
                        p.fontMetrics().height()),
              Qt::AlignCenter, tr("Thread angle"));
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
        p.drawText(QRectF(cx - 100, 0, 200, 18), Qt::AlignCenter, tr("Cylindrical head"));

        widthOf(p, cx - 24, cx + 24, top - 14, headTop, tr("Head diameter"), TextAt::Right);
        widthOf(p, cx - cb, cx + cb, top - 38, top, tr("Counterbore diameter"), TextAt::Left);
        heightOf(p, headTop, headTop + head, cx + cb + 14, cx + 24, tr("Head\nheight"), true);
        heightOf(p, top, top + depth, cx - cb - 12, cx - cb, tr("Depth"), false, 40);
        widthOf(p, cx - hole, cx + hole, bottom + 14, bottom, tr("Clearance hole"), TextAt::Centered, true);
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
        p.drawText(QRectF(cx - 100, 0, 200, 18), Qt::AlignCenter, tr("Conical head"));

        widthOf(p, cx - cone, cx + cone, top - 14, top, tr("Head diameter"), TextAt::Right);
        widthOf(p, cx - sink, cx + sink, top - 38, top, tr("Countersink diameter"), TextAt::Left);
        widthOf(p, cx - hole, cx + hole, bottom + 14, bottom, tr("Clearance hole"), TextAt::Centered, true);
        p.restore();
    }
}
