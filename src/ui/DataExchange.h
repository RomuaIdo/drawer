#pragma once

#include <QObject>

#include "data/Repositories.h"
#include "export/Exporter.h"

class QWidget;

// File import and export: the whole database as JSON, and the item mapping through the
// chosen exporter. Asks for the file and reports problems on `parent`.
class DataExchange : public QObject
{
    Q_OBJECT

public:
    DataExchange(IOrganizationRepository &organization, IMappingRepository &mapping,
                 const IExporter &exporter, QWidget *parent);

    void exportOrganization();
    void importOrganization();
    void exportMapping();

signals:
    void statusMessage(const QString &message);
    // Cabinets were added to the database.
    void imported();

private:
    QWidget *window() const;

    IOrganizationRepository &m_organization;
    IMappingRepository &m_mapping;
    const IExporter &m_exporter;
};
