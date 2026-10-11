#pragma once

#include <QObject>

#include "core/Models.h"
#include "data/Repositories.h"

class QWidget;

// The dialogs that change the structure of the cabinets: cabinets, drawer groups and single
// drawers. Saves the result and tells the window what to reload.
class CabinetEditor : public QObject
{
    Q_OBJECT

public:
    CabinetEditor(ICabinetRepository &cabinets, IDrawerRepository &drawers, QWidget *parent);

    void addCabinet();
    void editCabinet(int cabinetId);
    void deleteCabinet(int cabinetId, const QString &name);
    void addGroup(int cabinetId);
    void editGroup(const DrawerGroup &group);
    void deleteGroup(const DrawerGroup &group);
    void moveGroups(const QList<DrawerGroup> &groups);
    void editDrawer(const Drawer &drawer);

signals:
    // The cabinet list changed; `selectId` is the cabinet to show (0 keeps the current one).
    void cabinetsChanged(int selectId);
    // Groups or drawers of the shown cabinet changed.
    void layoutChanged();

private:
    QWidget *window() const;

    ICabinetRepository &m_cabinets;
    IDrawerRepository &m_drawers;
};
