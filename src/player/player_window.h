#ifndef PLAYER_WINDOW_H
#define PLAYER_WINDOW_H

#include "../view/typewritereffect.h"

#include <QMainWindow>

class QGraphicsScene;
class QGraphicsView;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QTemporaryDir;
class QJsonObject;
class QKeyEvent;
class QResizeEvent;
class QEvent;
class QShowEvent;

class PlayerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget *parent = nullptr);
    ~PlayerWindow() override;

    bool loadGame(const QString &path, QString *errorMsg = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onAdvanceRequested();
    void onTextUpdated(const QString &text);
    void onTextFinished();

private:
    struct RuntimeLine {
        int id = 0;
        QString characterId;
        QString text;
        QString backgroundPath;
        QString voicePath;
        int portraitScale = 100;
    };

    bool loadFromDirectory(const QString &dirPath, QString *errorMsg);
    bool extractPackageToTemp(const QString &packagePath, QString *outDir, QString *errorMsg);
    void renderCurrentLine();
    void updateViewTransform();
    void toggleFullScreenMode();
    void saveQuickSlot();
    void loadQuickSlot();
    QString resolveAssetPath(const QString &assetRelPath) const;

    QGraphicsView *m_view = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
    QGraphicsPixmapItem *m_portraitItem = nullptr;
    QGraphicsRectItem *m_dialogBox = nullptr;
    QGraphicsTextItem *m_nameItem = nullptr;
    QGraphicsTextItem *m_textItem = nullptr;

    TypewriterEffect m_typewriter;
    bool m_fullScreen = false;
    QString m_title;
    QString m_baseDir;
    QString m_packagePath;
    QTemporaryDir *m_extractTempDir = nullptr;

    QList<RuntimeLine> m_script;
    QHash<QString, QString> m_charDisplayNames;
    QHash<QString, QString> m_charPortraitPaths;
    int m_currentIndex = 0;
};

#endif // PLAYER_WINDOW_H
