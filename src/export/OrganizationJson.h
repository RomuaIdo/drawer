#pragma once

#include <QByteArray>
#include <stdexcept>

#include "core/Organization.h"

// Thrown when an organization file is malformed or inconsistent.
class OrganizationFormatError : public std::runtime_error
{
public:
    explicit OrganizationFormatError(const QString &message)
        : std::runtime_error(message.toStdString()) {}
};

// JSON file holding a complete organization, for backup and sharing.
namespace OrganizationJson {

QByteArray write(const Organization &organization);

// Validates the whole file; throws OrganizationFormatError describing the first problem.
// Cells missing from a group are filled with default drawers.
Organization read(const QByteArray &data);

} // namespace OrganizationJson
