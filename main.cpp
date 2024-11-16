#include "game.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <unistd.h>

int main(int argc, char *argv[])
{
  if (argc > 1) {
    int number = atoi(argv[1]);
    Server server;
    server.radiate();
  } else {
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
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
