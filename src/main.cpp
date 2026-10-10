#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteMappingRepository.h"
#include "data/SqliteOrganizationRepository.h"
#include "data/UndoLog.h"
#include "export/CsvExporter.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Drawer");

    try {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        Database database(dir + "/drawer.db");
        SqliteCabinetRepository cabinets(database.connection());
        SqliteDrawerRepository drawers(database.connection());
        SqliteItemRepository items(database.connection());
        SqliteMappingRepository mapping(database.connection());
        SqliteOrganizationRepository organization(database.connection());
        CsvExporter exporter;

        MainWindow window({cabinets, drawers, items, mapping, organization, database.undoLog()}, exporter);
        window.show();
        return app.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Drawer", e.what());
        return 1;
    }
}
