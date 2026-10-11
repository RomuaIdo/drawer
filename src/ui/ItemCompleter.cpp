#include "ItemCompleter.h"

#include <QApplication>
#include <QListView>
#include <QPainter>
#include <QStyle>
#include <QStyledItemDelegate>
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

constexpr int kRadius = 6;
constexpr int kInset = 5; // Space between the border and the rows.

// A row of the popup: text with a rounded highlight when it is the current one.
class SuggestionDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QRectF row = QRectF(option.rect).adjusted(kInset, 1, -kInset, -1);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const bool current = option.state & QStyle::State_Selected; // Keyboard.
        const bool hovered = option.state & QStyle::State_MouseOver;
        if (current || hovered) {
            // Hover is a subtle tint of the text color; the keyboard row uses the accent.
            const QPalette &p = option.palette;
            QColor tint = p.color(QPalette::Base);
            const QColor text = p.color(QPalette::Text);
            tint = QColor::fromRgbF(tint.redF() * 0.85 + text.redF() * 0.15,
                                    tint.greenF() * 0.85 + text.greenF() * 0.15,
                                    tint.blueF() * 0.85 + text.blueF() * 0.15);
            painter->setPen(Qt::NoPen);
            painter->setBrush(current ? p.color(QPalette::Highlight) : tint);
            painter->drawRoundedRect(row, kRadius - 2, kRadius - 2);
        }
        painter->setPen(option.palette.color(current ? QPalette::HighlightedText : QPalette::Text));
        painter->drawText(row.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        return {0, option.fontMetrics.height() + 12};
    }
};

// The popup: a rounded panel in the palette's colors with an accent border, like the
// drop-down of the category list.
class SuggestionList : public QListView
{
public:
    SuggestionList()
    {
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setFrameShape(QFrame::NoFrame);
        setAttribute(Qt::WA_TranslucentBackground);
        viewport()->setAttribute(Qt::WA_TranslucentBackground);
        viewport()->setAutoFillBackground(false);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setMouseTracking(true); // Hovering highlights a row, like the arrow keys do.
        viewport()->setMouseTracking(true);
    }

    // The completer replaces the delegate and the window flags when the popup is installed.
    void finishSetup()
    {
        setItemDelegate(new SuggestionDelegate(this));
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter p(viewport());
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(palette().color(QPalette::Highlight), 1));
        p.setBrush(palette().color(QPalette::Base));
        p.drawRoundedRect(QRectF(viewport()->rect()).adjusted(0.5, 0.5, -0.5, -0.5), kRadius, kRadius);
        p.end();
        QListView::paintEvent(event);
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
    auto *list = new SuggestionList;
    setPopup(list);
    list->finishSetup();
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
