#pragma once

#include <QIODevice>
#include <QList>
#include <QString>

#include "core/Models.h"

// Strategy for serializing the cabinet mapping to a file format.
class IExporter
{
public:
    virtual ~IExporter() = default;
    virtual QString name() const = 0;
    // File dialog filter, e.g. "CSV files (*.csv)".
    virtual QString fileFilter() const = 0;
    virtual QString defaultSuffix() const = 0;
    virtual void write(const QList<MappingRow> &rows, QIODevice &out) const = 0;
};
