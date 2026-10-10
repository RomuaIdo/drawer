#include "ItemSearch.h"

#include <QSet>
#include <cmath>

#include "tools/Engineering.h"

namespace ItemSearch {

namespace {

bool sameValue(double a, double b)
{
    return std::abs(a - b) <= 1e-9 * std::max(std::abs(a), std::abs(b));
}

bool inCategory(const Item &item, const QString &category)
{
    return item.category.trimmed().compare(category, Qt::CaseInsensitive) == 0;
}

} // namespace

QList<Item> run(const IItemRepository &items, const SearchQuery &query)
{
    const QString text = query.text.trimmed();
    const QString category = query.category.trimmed();
    if (query.isEmpty())
        return {};

    QList<Item> candidates;
    if (text.isEmpty()) {
        candidates = items.all();
    } else {
        candidates = items.search(text);

        // A typed value also finds items whose value is written differently.
        if (const auto wanted = Engineering::parse(text); wanted && *wanted > 0) {
            QSet<int> known;
            for (const Item &item : candidates)
                known.insert(item.id);
            for (const Item &item : items.all()) {
                const auto value = Engineering::parse(item.value);
                if (value && sameValue(*value, *wanted) && !known.contains(item.id))
                    candidates.append(item);
            }
        }
    }

    if (category.isEmpty())
        return candidates;
    QList<Item> result;
    for (const Item &item : candidates) {
        if (inCategory(item, category))
            result.append(item);
    }
    return result;
}

} // namespace ItemSearch
