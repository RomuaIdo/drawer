#pragma once

#include <QCompleter>
#include <QList>

#include "core/Models.h"

class QStandardItemModel;

// Suggests the items already in stock while a name is typed, like a search box. Each
// suggestion shows the name with its details, and choosing one inserts the name and reports
// the whole item so the caller can fill other fields.
class ItemCompleter : public QCompleter
{
    Q_OBJECT

public:
    explicit ItemCompleter(QObject *parent = nullptr);

    // Keeps one suggestion per distinct item (name, category, value and package).
    void setItems(const QList<Item> &items);
    QString pathFromIndex(const QModelIndex &index) const override;

signals:
    void itemChosen(const Item &item);

private:
    QStandardItemModel *m_model;
    QList<Item> m_items;
};
