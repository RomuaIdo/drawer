#include "SqliteCabinetRepository.h"

#include "SqlUtil.h"

QList<Cabinet> SqliteCabinetRepository::all() const
{
    QList<Cabinet> result;
    QSqlQuery q = SqlUtil::exec(m_db, "SELECT id, name, description FROM cabinets ORDER BY name");
    while (q.next())
        result.append({q.value(0).toInt(), q.value(1).toString(), q.value(2).toString()});
    return result;
}

Cabinet SqliteCabinetRepository::add(Cabinet cabinet)
{
    QSqlQuery q = SqlUtil::exec(m_db, "INSERT INTO cabinets (name, description) VALUES (?, ?)",
                                {cabinet.name, cabinet.description});
    cabinet.id = q.lastInsertId().toInt();
    return cabinet;
}

void SqliteCabinetRepository::update(const Cabinet &cabinet)
{
    SqlUtil::exec(m_db, "UPDATE cabinets SET name = ?, description = ? WHERE id = ?",
                  {cabinet.name, cabinet.description, cabinet.id});
}

void SqliteCabinetRepository::remove(int id)
{
    SqlUtil::exec(m_db, "DELETE FROM cabinets WHERE id = ?", {id});
}
