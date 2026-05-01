#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "models/project.h"

#include <QMainWindow>
#include <QPersistentModelIndex>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class GameCanvas;
class QDockWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QStackedWidget;
class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QTextEdit;
class QListView;
class QAction;
class QPushButton;
class DialogueTimelineModel;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &projectPath, QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void dataChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTreeContextMenuRequested(const QPoint &pos);
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);

    void onTimelineClicked(const QModelIndex &index);
    void onTimelineDoubleClicked(const QModelIndex &index);

    void onCanvasAdvanceRequested();

    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onAddDialogue();
    void onAddBackgroundMusic();
    void onBatchAddDialogues();
    void onTogglePreviewMode(bool enabled);
    void onExportGame();
    void onTestRun();
    void onShowPropertiesDock();
    void onOpenProjectStyleEditor();

    void onProjectPropertiesChanged();
    void onCharacterPropertiesChanged();
    void onBackgroundPropertiesChanged();
    void onDialoguePropertiesChanged();
    void onBrowseBackgroundImage();
    void onBrowseBackgroundBgm();
    void onBrowseStartMenuBackground();
    void onBrowseStartMenuBgm();
    void onPickStartMenuFontColor();
    void onPickDialogueNameFontColor();
    void onPickDialogueTextFontColor();
    void onPickDialogueLineNameFontColor();
    void onPickDialogueLineTextFontColor();
    void onBrowseDialogueVoice();
    void onDeleteSelectedItem();

private:
    enum class TreeItemType {
        Root,
        CharactersGroup,
        Character,
        BackgroundsGroup,
        Background,
        BgmsGroup,
        BgmTrack,
        DialoguesGroup,
        Dialogue
    };

    void setupLayout();
    void setupMenusAndToolbar();
    void setupTreeDock();
    void setupPropertyDock();
    void setupTimelineDock();
    void setupCanvas();

    void createOrLoadProject(const QString &projectPath, bool loadIfExists);
    void refreshAllViews();
    void refreshTree();
    void refreshTimeline();
    void refreshPropertyPage();
    void refreshBackgroundConflictLabel(Background *target);
    void previewDialogueByRow(int row);

    void addCharacter();
    void addBackground();
    void addBackgroundMusic();
    void addDialogue();
    void deleteCharacter(Character *character);
    void deleteBackground(Background *background);
    void clearBackgroundBgm(BgmTrack *track);
    void deleteDialogue(Dialogue *dialogue);

    Character *selectedCharacter() const;
    Background *selectedBackground() const;
    BgmTrack *selectedBgmTrack() const;
    Dialogue *selectedDialogue() const;

    TreeItemType itemType(const QTreeWidgetItem *item) const;
    void setItemType(QTreeWidgetItem *item, TreeItemType type) const;

    QString m_projectPath;
    Ui::MainWindow *ui = nullptr;
    Project *m_project = nullptr;
    bool m_updatingPropertyWidgets = false;
    bool m_previewMode = false;

    QDockWidget *m_resourcesDock = nullptr;
    QDockWidget *m_propertiesDock = nullptr;
    QDockWidget *m_timelineDock = nullptr;

    QTreeWidget *m_treeResources = nullptr;
    QTreeWidgetItem *m_rootItem = nullptr;
    QTreeWidgetItem *m_charactersGroupItem = nullptr;
    QTreeWidgetItem *m_backgroundsGroupItem = nullptr;
    QTreeWidgetItem *m_bgmsGroupItem = nullptr;
    QTreeWidgetItem *m_dialoguesGroupItem = nullptr;

    GameCanvas *m_canvas = nullptr;

    QStackedWidget *m_stackProperties = nullptr;
    QLabel *m_emptyPropertyLabel = nullptr;
    QLineEdit *m_projectStartBgEdit = nullptr;
    QPushButton *m_projectStartBgBrowseButton = nullptr;
    QLineEdit *m_projectStartBgmEdit = nullptr;
    QPushButton *m_projectStartBgmBrowseButton = nullptr;
    QSpinBox *m_projectStartFontSizeSpin = nullptr;
    QLineEdit *m_projectStartFontColorEdit = nullptr;
    QPushButton *m_projectStartFontColorButton = nullptr;
    QSpinBox *m_projectDialogueNameFontSizeSpin = nullptr;
    QLineEdit *m_projectDialogueNameFontColorEdit = nullptr;
    QPushButton *m_projectDialogueNameFontColorButton = nullptr;
    QSpinBox *m_projectDialogueTextFontSizeSpin = nullptr;
    QLineEdit *m_projectDialogueTextFontColorEdit = nullptr;
    QPushButton *m_projectDialogueTextFontColorButton = nullptr;

    QLineEdit *m_charNameEdit = nullptr;
    QLineEdit *m_charPortraitEdit = nullptr;
    QComboBox *m_charPositionCombo = nullptr;
    QSpinBox *m_charScaleSpin = nullptr;
    QLineEdit *m_charVoicePrefixEdit = nullptr;

    QLineEdit *m_bgImageEdit = nullptr;
    QPushButton *m_bgBrowseButton = nullptr;
    QLineEdit *m_bgBgmEdit = nullptr;
    QPushButton *m_bgBgmBrowseButton = nullptr;
    QSpinBox *m_bgStartSpin = nullptr;
    QSpinBox *m_bgEndSpin = nullptr;
    QSpinBox *m_bgBgmStartSpin = nullptr;
    QSpinBox *m_bgBgmEndSpin = nullptr;
    QLabel *m_bgConflictLabel = nullptr;

    QComboBox *m_dialogueCharacterCombo = nullptr;
    QTextEdit *m_dialogueTextEdit = nullptr;
    QLineEdit *m_dialogueVoiceEdit = nullptr;
    QPushButton *m_dialogueVoiceBrowseButton = nullptr;
    QSpinBox *m_dialogueNameFontSizeSpin = nullptr;
    QLineEdit *m_dialogueNameFontColorEdit = nullptr;
    QPushButton *m_dialogueNameFontColorButton = nullptr;
    QSpinBox *m_dialogueTextFontSizeSpin = nullptr;
    QLineEdit *m_dialogueTextFontColorEdit = nullptr;
    QPushButton *m_dialogueTextFontColorButton = nullptr;
    QComboBox *m_dialogueEffectCombo = nullptr;

    QListView *m_timelineView = nullptr;
    DialogueTimelineModel *m_timelineModel = nullptr;
    int m_currentTimelineRow = -1;

    QAction *m_actionPreviewMode = nullptr;
    QString m_lastTestRunDir;
};

#endif // MAINWINDOW_H
