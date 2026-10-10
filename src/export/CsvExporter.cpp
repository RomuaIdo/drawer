#include "CsvExporter.h"

#include <QRegularExpression>
#include <QStringList>

QString CsvExporter::escape(const QString &field)
{
    if (!field.contains(QRegularExpression("[\",\\r\\n]")))
        return field;
    QString quoted = field;
    quoted.replace('"', "\"\"");
    return '"' + quoted + '"';
}

void CsvExporter::write(const QList<MappingRow> &rows, QIODevice &out) const
{
    auto writeLine = [&out](const QStringList &fields) {
        QStringList escaped;
        for (const QString &f : fields)
            escaped.append(escape(f));
        out.write(escaped.join(',').toUtf8() + "\r\n");
    };

    if (m_writeBom)
        out.write("\xEF\xBB\xBF");

    writeLine({"cabinet", "group", "row", "column", "drawer", "size", "item", "category",
               "value", "package", "quantity", "min_stock", "low_stock", "notes"});
    for (const MappingRow &r : rows) {
        writeLine({r.cabinet, r.group, QString::number(r.row), QString::number(r.col),
                   r.drawerLabel, drawerSizeToString(r.drawerSize), r.itemName, r.category,
                   r.value, r.package, r.hasItem ? QString::number(r.quantity) : QString(),
                   r.minStock ? QString::number(*r.minStock) : QString(),
                   r.lowStock ? QString("yes") : QString(), r.notes});
    }
}
