#include "OrganizationJson.h"

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <cmath>

#include "core/Layout.h"

namespace OrganizationJson {

namespace {

const char *kFormat = "drawer-organization";
constexpr int kVersion = 1;
constexpr int kMaxGroupSide = 100;

QJsonObject toJson(const Item &item)
{
    return {{"name", item.name}, {"category", item.category}, {"value", item.value},
            {"package", item.package}, {"quantity", item.quantity}, {"notes", item.notes}};
}

QJsonObject toJson(const DrawerEntry &entry)
{
    QJsonArray items;
    for (const Item &item : entry.items)
        items.append(toJson(item));
    return {{"row", entry.drawer.row}, {"col", entry.drawer.col},
            {"size", drawerSizeToString(entry.drawer.size)}, {"label", entry.drawer.label},
            {"items", items}};
}

QJsonObject toJson(const GroupEntry &entry)
{
    QJsonArray drawers;
    for (const DrawerEntry &d : entry.drawers)
        drawers.append(toJson(d));
    const DrawerGroup &g = entry.group;
    QJsonObject object{{"name", g.name}, {"rows", g.rows}, {"cols", g.cols}, {"posX", g.posX},
                       {"posY", g.posY}, {"defaultSize", drawerSizeToString(g.defaultSize)},
                       {"drawers", drawers}};
    if (!g.color.isEmpty())
        object["color"] = g.color;
    return object;
}

QJsonObject toJson(const CabinetEntry &entry)
{
    QJsonArray groups;
    for (const GroupEntry &g : entry.groups)
        groups.append(toJson(g));
    return {{"name", entry.cabinet.name}, {"description", entry.cabinet.description},
            {"groups", groups}};
}

// Helpers that fail with a message naming the offending location.
[[noreturn]] void fail(const QString &where, const QString &what)
{
    throw OrganizationFormatError(QString("%1: %2").arg(where, what));
}

QString requireString(const QJsonObject &o, const QString &key, const QString &where,
                      bool required = true)
{
    const QJsonValue v = o.value(key);
    if (v.isUndefined() && !required)
        return {};
    if (!v.isString())
        fail(where, QString("\"%1\" must be a string").arg(key));
    return v.toString();
}

double requireNumber(const QJsonObject &o, const QString &key, const QString &where)
{
    const QJsonValue v = o.value(key);
    if (!v.isDouble())
        fail(where, QString("\"%1\" must be a number").arg(key));
    return v.toDouble();
}

int requireInt(const QJsonObject &o, const QString &key, const QString &where, int min, int max)
{
    const double v = requireNumber(o, key, where);
    if (v != std::floor(v) || v < min || v > max)
        fail(where, QString("\"%1\" must be an integer between %2 and %3").arg(key).arg(min).arg(max));
    return int(v);
}

QJsonArray requireArray(const QJsonObject &o, const QString &key, const QString &where)
{
    const QJsonValue v = o.value(key);
    if (v.isUndefined())
        return {};
    if (!v.isArray())
        fail(where, QString("\"%1\" must be an array").arg(key));
    return v.toArray();
}

DrawerSize requireSize(const QJsonObject &o, const QString &key, const QString &where)
{
    const QString text = requireString(o, key, where);
    const DrawerSize size = drawerSizeFromString(text);
    if (drawerSizeToString(size).compare(text, Qt::CaseInsensitive) != 0)
        fail(where, QString("unknown size \"%1\"").arg(text));
    return size;
}

Item itemFromJson(const QJsonObject &o, const QString &where)
{
    Item item;
    item.name = requireString(o, "name", where).trimmed();
    if (item.name.isEmpty())
        fail(where, "item name must not be empty");
    item.category = requireString(o, "category", where, false);
    item.value = requireString(o, "value", where, false);
    item.package = requireString(o, "package", where, false);
    item.notes = requireString(o, "notes", where, false);
    item.quantity = o.contains("quantity") ? requireInt(o, "quantity", where, 0, 100000000) : 0;
    return item;
}

GroupEntry groupFromJson(const QJsonObject &o, const QString &where)
{
    GroupEntry entry;
    DrawerGroup &g = entry.group;
    g.name = requireString(o, "name", where).trimmed();
    if (g.name.isEmpty())
        fail(where, "group name must not be empty");
    g.rows = requireInt(o, "rows", where, 1, kMaxGroupSide);
    g.cols = requireInt(o, "cols", where, 1, kMaxGroupSide);
    g.posX = requireNumber(o, "posX", where);
    g.posY = requireNumber(o, "posY", where);
    if (g.posX < 0 || g.posY < 0)
        fail(where, "position must not be negative");
    g.defaultSize = requireSize(o, "defaultSize", where);

    const QString color = requireString(o, "color", where, false);
    if (!color.isEmpty()) {
        const QColor parsed(color);
        if (!parsed.isValid())
            fail(where, QString("invalid color \"%1\"").arg(color));
        g.color = parsed.name(); // Normalized to #rrggbb.
    }

    // Start from default drawers, then apply the ones described in the file.
    QList<DrawerEntry> cells;
    for (int r = 0; r < g.rows; ++r) {
        for (int c = 0; c < g.cols; ++c) {
            DrawerEntry cell;
            cell.drawer.row = r;
            cell.drawer.col = c;
            cell.drawer.size = g.defaultSize;
            cell.drawer.label = QString("R%1C%2").arg(r + 1).arg(c + 1);
            cells.append(cell);
        }
    }

    QSet<int> seen;
    const QJsonArray drawers = requireArray(o, "drawers", where);
    for (int i = 0; i < drawers.size(); ++i) {
        const QString dwhere = QString("%1, drawer %2").arg(where).arg(i + 1);
        if (!drawers[i].isObject())
            fail(dwhere, "must be an object");
        const QJsonObject d = drawers[i].toObject();
        const int row = requireInt(d, "row", dwhere, 0, g.rows - 1);
        const int col = requireInt(d, "col", dwhere, 0, g.cols - 1);
        const int cellKey = row * kMaxGroupSide + col;
        if (seen.contains(cellKey))
            fail(dwhere, QString("duplicate drawer at row %1, col %2").arg(row).arg(col));
        seen.insert(cellKey);

        DrawerEntry &cell = cells[row * g.cols + col];
        cell.drawer.size = requireSize(d, "size", dwhere);
        const QString label = requireString(d, "label", dwhere, false);
        if (!label.isEmpty())
            cell.drawer.label = label;
        const QJsonArray items = requireArray(d, "items", dwhere);
        for (int k = 0; k < items.size(); ++k) {
            const QString iwhere = QString("%1, item %2").arg(dwhere).arg(k + 1);
            if (!items[k].isObject())
                fail(iwhere, "must be an object");
            cell.items.append(itemFromJson(items[k].toObject(), iwhere));
        }
    }
    entry.drawers = cells;
    return entry;
}

CabinetEntry cabinetFromJson(const QJsonObject &o, const QString &where)
{
    CabinetEntry entry;
    entry.cabinet.name = requireString(o, "name", where).trimmed();
    if (entry.cabinet.name.isEmpty())
        fail(where, "cabinet name must not be empty");
    entry.cabinet.description = requireString(o, "description", where, false);

    QList<DrawerGroup> placed;
    const QJsonArray groups = requireArray(o, "groups", where);
    for (int i = 0; i < groups.size(); ++i) {
        const QString gwhere = QString("%1, group %2").arg(where).arg(i + 1);
        if (!groups[i].isObject())
            fail(gwhere, "must be an object");
        GroupEntry group = groupFromJson(groups[i].toObject(), gwhere);
        group.group.id = i + 1; // Temporary id, only used for the overlap check.
        if (Layout::collides(group.group, placed))
            fail(gwhere, "overlaps another group");
        placed.append(group.group);
        entry.groups.append(group);
    }
    return entry;
}

} // namespace

QByteArray write(const Organization &organization)
{
    QJsonArray cabinets;
    for (const CabinetEntry &c : organization.cabinets)
        cabinets.append(toJson(c));
    const QJsonObject root{{"format", kFormat}, {"version", kVersion}, {"cabinets", cabinets}};
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

Organization read(const QByteArray &data)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (doc.isNull())
        throw OrganizationFormatError(QString("Invalid JSON: %1").arg(error.errorString()));
    if (!doc.isObject())
        throw OrganizationFormatError("Invalid file: expected a JSON object");

    const QJsonObject root = doc.object();
    if (root.value("format").toString() != kFormat)
        throw OrganizationFormatError("Not a drawer organization file");
    if (root.value("version").toInt() != kVersion)
        throw OrganizationFormatError(
            QString("Unsupported file version %1").arg(root.value("version").toInt()));

    Organization organization;
    const QJsonArray cabinets = requireArray(root, "cabinets", "File");
    for (int i = 0; i < cabinets.size(); ++i) {
        const QString where = QString("Cabinet %1").arg(i + 1);
        if (!cabinets[i].isObject())
            fail(where, "must be an object");
        organization.cabinets.append(cabinetFromJson(cabinets[i].toObject(), where));
    }
    return organization;
}

} // namespace OrganizationJson
