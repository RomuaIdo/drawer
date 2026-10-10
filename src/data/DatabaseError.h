#pragma once

#include <stdexcept>
#include <QString>

class DatabaseError : public std::runtime_error
{
public:
    explicit DatabaseError(const QString &message)
        : std::runtime_error(message.toStdString()) {}
};
