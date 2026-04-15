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
    void onTogglePreviewMode(bool enabled);
    void onExportGame();
    void onTestRun();

    void onCharacterPropertiesChanged();
    void onBackgroundPropertiesChanged();
    void onDialoguePropertiesChanged();
    void onBrowseBackgroundImage();
    void onDeleteSelectedItem();

private:
    enum class TreeItemType {
        Root,
        CharactersGroup,
        Character,
        BackgroundsGroup,
        Background,
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
    void addDialogue();
    void deleteCharacter(Character *character);
    void deleteBackground(Background *background);
    void deleteDialogue(Dialogue *dialogue);

    Character *selectedCharacter() const;
    Background *selectedBackground() const;
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
    QTreeWidgetItem *m_dialoguesGroupItem = nullptr;

    GameCanvas *m_canvas = nullptr;

    QStackedWidget *m_stackProperties = nullptr;
    QLabel *m_emptyPropertyLabel = nullptr;

    QLineEdit *m_charNameEdit = nullptr;
    QLineEdit *m_charPortraitEdit = nullptr;
    QComboBox *m_charPositionCombo = nullptr;
    QLineEdit *m_charVoicePrefixEdit = nullptr;

    QLineEdit *m_bgImageEdit = nullptr;
    QPushButton *m_bgBrowseButton = nullptr;
    QSpinBox *m_bgStartSpin = nullptr;
    QSpinBox *m_bgEndSpin = nullptr;
    QLabel *m_bgConflictLabel = nullptr;

    QComboBox *m_dialogueCharacterCombo = nullptr;
    QTextEdit *m_dialogueTextEdit = nullptr;
    QLineEdit *m_dialogueVoiceEdit = nullptr;
    QComboBox *m_dialogueEffectCombo = nullptr;

    QListView *m_timelineView = nullptr;
    DialogueTimelineModel *m_timelineModel = nullptr;
    int m_currentTimelineRow = -1;

    QAction *m_actionPreviewMode = nullptr;
    QString m_lastTestRunDir;
};

#endif // MAINWINDOW_H
