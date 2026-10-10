#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
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

        MainWindow window(cabinets, drawers, items);
        window.show();
        return app.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Drawer", e.what());
        return 1;
    }
}
