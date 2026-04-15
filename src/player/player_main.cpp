#include "player_window.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString gamePath;
    if (argc >= 2) {
        gamePath = QString::fromLocal8Bit(argv[1]);
    } else {
        gamePath = QFileDialog::getOpenFileName(nullptr,
                                                "选择游戏包或目录",
                                                QString(),
                                                "Galgame (*.galgame);;Game Json (game.json)");
    }

    if (gamePath.isEmpty()) {
        return 0;
    }

    if (gamePath.endsWith("game.json", Qt::CaseInsensitive)) {
        gamePath = QFileInfo(gamePath).absolutePath();
    }

    PlayerWindow w;
    QString error;
    if (!w.loadGame(gamePath, &error)) {
        QMessageBox::critical(nullptr, "加载失败", error);
        return 1;
    }

    w.show();
    return app.exec();
}
