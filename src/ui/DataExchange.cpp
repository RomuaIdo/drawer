#include "DataExchange.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "DatabaseGuard.h"
#include "export/OrganizationJson.h"

DataExchange::DataExchange(IOrganizationRepository &organization, IMappingRepository &mapping,
                           const IExporter &exporter, QWidget *parent)
    : QObject(parent)
    , m_organization(organization)
    , m_mapping(mapping)
    , m_exporter(exporter)
{
}

QWidget *DataExchange::window() const
{
    return static_cast<QWidget *>(parent());
}

void DataExchange::exportOrganization()
{
    QString path = QFileDialog::getSaveFileName(window(), tr("Export database"),
                                                "drawer-database.json",
                                                tr("Drawer database (*.json)"));
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".json";

    runGuarded(window(), [&] {
        const Organization organization = m_organization.load();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || file.write(OrganizationJson::write(organization)) < 0) {
            QMessageBox::critical(window(), tr("Export failed"), file.errorString());
            return;
        }
        emit statusMessage(
            tr("Exported %n cabinet(s) to %1", "", organization.cabinets.size()).arg(path));
    });
}

void DataExchange::importOrganization()
{
    const QString path = QFileDialog::getOpenFileName(window(), tr("Import database"), QString(),
                                                      tr("Drawer database (*.json)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(window(), tr("Import failed"), file.errorString());
        return;
    }
    try {
        const Organization organization = OrganizationJson::read(file.readAll());
        const auto answer = QMessageBox::question(
            window(), tr("Import database"),
            tr("Add %n cabinet(s) from this file? Existing cabinets are kept.", "",
               organization.cabinets.size()));
        if (answer != QMessageBox::Yes)
            return;
        m_organization.import(organization);
        emit imported();
        emit statusMessage(tr("Imported %n cabinet(s)", "", organization.cabinets.size()));
    } catch (const OrganizationFormatError &e) {
        QMessageBox::critical(window(), tr("Import failed"), QString::fromUtf8(e.what()));
    } catch (const DatabaseError &e) {
        QMessageBox::critical(window(), tr("Database error"), QString::fromUtf8(e.what()));
    }
}

void DataExchange::exportMapping()
{
    QString path = QFileDialog::getSaveFileName(window(), tr("Export mapping"), "drawer-mapping",
                                                m_exporter.fileFilter());
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += "." + m_exporter.defaultSuffix();

    runGuarded(window(), [&] {
        const QList<MappingRow> rows = m_mapping.rows();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(window(), tr("Export failed"), file.errorString());
            return;
        }
        m_exporter.write(rows, file);
        emit statusMessage(tr("Exported %n row(s) to %1", "", rows.size()).arg(path));
    });
}
