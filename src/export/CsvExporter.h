#pragma once

#include "Exporter.h"

// RFC 4180 CSV with CRLF line endings.
class CsvExporter : public IExporter
{
public:
    // The UTF-8 BOM makes Excel pick the right encoding.
    explicit CsvExporter(bool writeBom = true) : m_writeBom(writeBom) {}

    QString name() const override { return "CSV"; }
    QString fileFilter() const override { return "CSV files (*.csv)"; }
    QString defaultSuffix() const override { return "csv"; }
    void write(const QList<MappingRow> &rows, QIODevice &out) const override;

    static QString escape(const QString &field);

private:
    bool m_writeBom;
};
