#include "ItemCompleter.h"

#include <QApplication>
#include <QListView>
#include <QPainter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionMenuItem>
#include <QSet>
#include <QStandardItemModel>
#include <algorithm>

namespace {

// "Name - value, package (category)", leaving out what is empty.
QString describe(const Item &item)
{
    QStringList details;
    for (const QString &part : {item.value, item.package}) {
        if (!part.isEmpty())
            details << part;
    }
    QString text = item.name;
    if (!details.isEmpty())
        text += " - " + details.join(", ");
    if (!item.category.isEmpty())
        text += " (" + item.category + ")";
    return text;
}

QString identity(const Item &item)
{
    const QChar separator(0x1f);
    return (item.name + separator + item.category + separator + item.value + separator
            + item.package).toLower();
}

// Draws each row like an entry of a drop-down list (the category list), in the current style.
class MenuItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QStyleOptionMenuItem menu = menuOption(option, index);
        style(option)->drawControl(QStyle::CE_MenuItem, &menu, painter, option.widget);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const QStyleOptionMenuItem menu = menuOption(option, index);
        return style(option)->sizeFromContents(QStyle::CT_MenuItem, &menu,
                                               option.fontMetrics.size(Qt::TextSingleLine, menu.text),
                                               option.widget);
    }

private:
    static QStyle *style(const QStyleOptionViewItem &option)
    {
        return option.widget ? option.widget->style() : QApplication::style();
    }

    static QStyleOptionMenuItem menuOption(const QStyleOptionViewItem &option,
                                           const QModelIndex &index)
    {
        QStyleOptionMenuItem menu;
        menu.palette = option.palette;
        menu.state = QStyle::State_Enabled;
        if (option.state & QStyle::State_Selected)
            menu.state |= QStyle::State_Selected;
        menu.menuItemType = QStyleOptionMenuItem::Normal;
        menu.checkType = QStyleOptionMenuItem::NotCheckable;
        menu.text = index.data(Qt::DisplayRole).toString();
        menu.font = option.font;
        menu.fontMetrics = option.fontMetrics;
        menu.rect = option.rect;
        menu.menuRect = option.rect;
        menu.maxIconWidth = 0;
        menu.reservedShortcutWidth = 0;
        return menu;
    }
};

} // namespace

ItemCompleter::ItemCompleter(QObject *parent)
    : QCompleter(parent)
    , m_model(new QStandardItemModel(this))
{
    setModel(m_model);
    setCaseSensitivity(Qt::CaseInsensitive);
    setFilterMode(Qt::MatchContains);
    setCompletionMode(QCompleter::PopupCompletion);
    auto *list = new QListView;
    list->setItemDelegate(new MenuItemDelegate(list));
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setFrameStyle(list->style()->styleHint(QStyle::SH_ComboBox_PopupFrameStyle, nullptr, list));
    setPopup(list);
    connect(this, qOverload<const QModelIndex &>(&QCompleter::activated), this,
            [this](const QModelIndex &index) {
                const int row = index.data(Qt::UserRole).toInt();
                if (row >= 0 && row < m_items.size())
                    emit itemChosen(m_items[row]);
            });
}

void ItemCompleter::setItems(const QList<Item> &items)
{
    m_items.clear();
    QSet<QString> seen;
    for (const Item &item : items) {
        if (!item.name.trimmed().isEmpty() && !seen.contains(identity(item))) {
            seen.insert(identity(item));
            m_items.append(item);
        }
    }
    std::sort(m_items.begin(), m_items.end(), [](const Item &a, const Item &b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });

    m_model->clear();
    for (int row = 0; row < m_items.size(); ++row) {
        auto *entry = new QStandardItem(describe(m_items[row]));
        entry->setData(row, Qt::UserRole);
        m_model->appendRow(entry);
    }
}

// What gets typed into the field is the name alone, not the whole description.
QString ItemCompleter::pathFromIndex(const QModelIndex &index) const
{
    const int row = index.data(Qt::UserRole).toInt();
    return row >= 0 && row < m_items.size() ? m_items[row].name : QCompleter::pathFromIndex(index);
}
