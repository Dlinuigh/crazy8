#include "game.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[]) {
    if (argc > 1) {
        const int number = strtol(argv[1], nullptr, 10);
        Server server(number);
        server.radiate();
    } else {
        QApplication a(argc, argv);

        QTranslator translator;
        const QStringList uiLanguages = QLocale::system().uiLanguages();
        for (const QString &locale: uiLanguages) {
            const QString baseName = "crazy8_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                a.installTranslator(&translator);
                break;
            }
        }
        Game w;
        w.show();
        return a.exec();
    }
}
