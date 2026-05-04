#ifndef PLAYER_WINDOW_H
#define PLAYER_WINDOW_H

#include "../view/typewritereffect.h"

#include <QMainWindow>
#include <QMediaPlayer>

class QGraphicsScene;
class QGraphicsView;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QTemporaryDir;
class QLabel;
class QPushButton;
class QMediaPlayer;
class QAudioOutput;
class QTimer;
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
    void onVoicePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onSettingsButtonClicked();
    void onSaveButtonClicked();

private:
    struct SaveSlotMeta {
        bool exists = false;
        int index = 0;
        QString timestamp;
        QString preview;
    };

    struct RuntimeLine {
        int id = 0;
        QString characterId;
        QString text;
        QString backgroundPath;
        QString bgmPath;
        QString voicePath;
        int portraitScale = 100;
        int portraitX = 320;
        int portraitY = -60;
        int nameFontSizeOverride = 0;
        QString nameFontColorOverride;
        int textFontSizeOverride = 0;
        QString textFontColorOverride;
    };

    bool loadFromDirectory(const QString &dirPath, QString *errorMsg);
    bool extractPackageToTemp(const QString &packagePath, QString *outDir, QString *errorMsg);
    bool showStartMenu(QString *errorMsg);
    bool saveToSlot(int slot, QString *errorMsg);
    bool loadFromSlot(int slot, QString *errorMsg);
    bool clearAllSaveSlots(QString *errorMsg);
    SaveSlotMeta readSaveSlotMeta(int slot) const;
    QString gameSaveDir() const;
    QString slotSavePath(int slot) const;
    void showSettingsDialog(bool *requestBackToStart = nullptr);
    void enterStartScreenState();
    void updateSaveButtonGeometry();
    void applyDialogueTextStyle();
    void playLineAudio(const RuntimeLine &line);
    void renderCurrentLine();
    void updateViewTransform();
    void toggleFullScreenMode();
    void returnToStartMenuAfterStoryEnd();
    void saveQuickSlot();
    void loadQuickSlot();
    void tryScheduleAutoAdvance();
    QString resolveAssetPath(const QString &assetRelPath) const;
    void applyHudButtonStyles();

    QGraphicsView *m_view = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
    QGraphicsPixmapItem *m_portraitItem = nullptr;
    QGraphicsRectItem *m_dialogBox = nullptr;
    QGraphicsTextItem *m_nameItem = nullptr;
    QGraphicsTextItem *m_textItem = nullptr;
    QPushButton *m_saveButton = nullptr;
    QLabel *m_autoPlayIndicator = nullptr;
    QMediaPlayer *m_voicePlayer = nullptr;
    QAudioOutput *m_voiceOutput = nullptr;
    QMediaPlayer *m_bgmPlayer = nullptr;
    QAudioOutput *m_bgmOutput = nullptr;
    QTimer *m_autoAdvanceTimer = nullptr;
    bool m_autoPlayEnabled = false;
    bool m_waitingVoiceFinishForAutoAdvance = false;
    bool m_waitingVoiceFinishForStoryEnd = false;
    QString m_currentBgmPath;

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
    QString m_startMenuBackgroundPath;
    QString m_startMenuBgmPath;
    int m_startMenuFontSize = 22;
    QString m_startMenuFontColor = "#FFFFFF";
    QString m_autoPlayIndicatorColor = "#7CFC00";
    QString m_settingsButtonColor = "#FFFFFF";
    int m_dialogueNameFontSize = 14;
    QString m_dialogueNameFontColor = "#FFFFFF";
    int m_dialogueTextFontSize = 12;
    QString m_dialogueTextFontColor = "#FFFFFF";
    QString m_dialogueBoxColor = "#000000";
    bool m_startMenuPending = false;
    bool m_startMenuShown = false;
};

#endif // PLAYER_WINDOW_H
