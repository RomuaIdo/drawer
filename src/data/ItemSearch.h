#pragma once

#include <QList>
#include <QString>

#include "core/Models.h"
#include "data/Repositories.h"

struct SearchQuery {
    QString text;     // Matches text fields, or an item's value ("1uF" finds 1u, 1000n, ...).
    QString category; // Restricts the result to this category; empty for any.

    bool isEmpty() const { return text.trimmed().isEmpty() && category.trimmed().isEmpty(); }
};

namespace ItemSearch {

// Items matching the query. The text matches name, category, value, package and notes
// (case-insensitive), and also any item whose value equals the typed value, however it is
// written. With only a category, every item of it; an empty query matches nothing.
QList<Item> run(const IItemRepository &items, const SearchQuery &query);

} // namespace ItemSearch
