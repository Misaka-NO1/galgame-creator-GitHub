#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "editors/charactereditor.h"
#include "editors/dialogueeditdialog.h"
#include "io/exporter.h"
#include "player/player_window.h"
#include "view/gamecanvas.h"

#include <QAbstractItemView>
#include <QAbstractListModel>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QDateTime>

namespace {

constexpr int kTreePointerRole = Qt::UserRole + 1;

quintptr ptrToValue(const void *ptr)
{
    return reinterpret_cast<quintptr>(ptr);
}

template <typename T>
T *valueToPtr(const QVariant &value)
{
    return reinterpret_cast<T *>(value.value<quintptr>());
}

QString dialogueSummary(const Dialogue *dialogue, const Character *character)
{
    if (!dialogue) {
        return {};
    }

    QString text = dialogue->text().trimmed();
    if (text.size() > 20) {
        text = text.left(20) + "...";
    }

    const QString speaker = character ? character->name() : QStringLiteral("未知角色");
    return QString("#%1 %2: %3").arg(dialogue->id()).arg(speaker, text);
}

QString positionToText(Character::Position position)
{
    switch (position) {
    case Character::Position::Left:
        return QStringLiteral("左");
    case Character::Position::Center:
        return QStringLiteral("中");
    case Character::Position::Right:
        return QStringLiteral("右");
    }
    return QStringLiteral("中");
}

Character::Position textToPosition(const QString &text)
{
    if (text == QStringLiteral("左")) {
        return Character::Position::Left;
    }
    if (text == QStringLiteral("右")) {
        return Character::Position::Right;
    }
    return Character::Position::Center;
}

QString effectToText(Dialogue::SpecialEffect effect)
{
    switch (effect) {
    case Dialogue::SpecialEffect::None:
        return QStringLiteral("none");
    case Dialogue::SpecialEffect::Shake:
        return QStringLiteral("shake");
    case Dialogue::SpecialEffect::Flash:
        return QStringLiteral("flash");
    case Dialogue::SpecialEffect::Fade:
        return QStringLiteral("fade");
    }
    return QStringLiteral("none");
}

Dialogue::SpecialEffect textToEffect(const QString &text)
{
    if (text == QStringLiteral("shake")) {
        return Dialogue::SpecialEffect::Shake;
    }
    if (text == QStringLiteral("flash")) {
        return Dialogue::SpecialEffect::Flash;
    }
    if (text == QStringLiteral("fade")) {
        return Dialogue::SpecialEffect::Fade;
    }
    return Dialogue::SpecialEffect::None;
}

QStringList collectVoiceFilesByPrefix(const QString &projectPath, const QString &prefix)
{
    const QString trimmedPrefix = prefix.trimmed();
    if (trimmedPrefix.isEmpty()) {
        return {};
    }

    QDir voicesDir(QDir(projectPath).filePath("resources/voices"));
    if (!voicesDir.exists()) {
        return {};
    }

    const QStringList filters = {"*.wav", "*.ogg", "*.mp3", "*.flac", "*.m4a"};
    const QFileInfoList files = voicesDir.entryInfoList(filters, QDir::Files, QDir::Name);

    QStringList matched;
    for (const QFileInfo &file : files) {
        if (file.fileName().startsWith(trimmedPrefix, Qt::CaseInsensitive)) {
            matched.append(file.fileName());
        }
    }
    return matched;
}

} // namespace

class DialogueTimelineModel final : public QAbstractListModel
{
public:
    explicit DialogueTimelineModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    void setProject(Project *project)
    {
        beginResetModel();
        m_project = project;
        endResetModel();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() || !m_project) {
            return 0;
        }
        return m_project->dialogueCount();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!m_project || !index.isValid() || role != Qt::DisplayRole) {
            return {};
        }

        Dialogue *dialogue = m_project->dialogueAt(index.row());
        const Character *character = dialogue ? m_project->getCharacter(dialogue->characterId()) : nullptr;
        return dialogueSummary(dialogue, character);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Qt::ItemFlags base = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
        if (index.isValid()) {
            base |= Qt::ItemIsDragEnabled;
        }
        return base;
    }

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationChild) override
    {
        if (!m_project || sourceParent.isValid() || destinationParent.isValid() || count != 1) {
            return false;
        }

        if (sourceRow < 0 || sourceRow >= m_project->dialogueCount()) {
            return false;
        }

        const int destination = qBound(0, destinationChild, m_project->dialogueCount());
        if (destination == sourceRow || destination == sourceRow + 1) {
            return false;
        }

        const int finalTo = destination > sourceRow ? destination - 1 : destination;
        if (finalTo < 0 || finalTo >= m_project->dialogueCount()) {
            return false;
        }

        beginMoveRows({}, sourceRow, sourceRow, {}, destination);
        const bool moved = m_project->moveDialogue(sourceRow, finalTo);
        endMoveRows();

        if (!moved) {
            return false;
        }

        emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
        return true;
    }

private:
    Project *m_project = nullptr;
};

MainWindow::MainWindow(const QString &projectPath, QWidget *parent)
    : QMainWindow(parent)
    , m_projectPath(projectPath)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupLayout();
    setupMenusAndToolbar();
    connect(this, &MainWindow::dataChanged, this, [this]() {
        refreshAllViews();
    });
    createOrLoadProject(projectPath, true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_previewMode) {
        onTogglePreviewMode(false);
        m_actionPreviewMode->setChecked(false);
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onTreeContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem *item = m_treeResources->itemAt(pos);
    QMenu contextMenu(this);

    if (!item) {
        contextMenu.addAction("添加角色", this, &MainWindow::addCharacter);
        contextMenu.addAction("添加背景", this, &MainWindow::addBackground);
    } else {
        const TreeItemType type = itemType(item);

        if (type == TreeItemType::Root) {
            contextMenu.addAction("添加角色", this, &MainWindow::addCharacter);
            contextMenu.addAction("添加背景", this, &MainWindow::addBackground);
        } else if (type == TreeItemType::CharactersGroup || type == TreeItemType::Character) {
            contextMenu.addAction("添加角色", this, &MainWindow::addCharacter);
            if (type == TreeItemType::Character) {
                contextMenu.addAction("删除角色", this, &MainWindow::onDeleteSelectedItem);
            }
        } else if (type == TreeItemType::BackgroundsGroup || type == TreeItemType::Background) {
            contextMenu.addAction("添加背景", this, &MainWindow::addBackground);
            if (type == TreeItemType::Background) {
                contextMenu.addAction("删除背景", this, &MainWindow::onDeleteSelectedItem);
            }
        } else if (type == TreeItemType::Dialogue) {
            contextMenu.addAction("删除对话", this, &MainWindow::onDeleteSelectedItem);
        }
    }

    if (!contextMenu.actions().isEmpty()) {
        contextMenu.exec(m_treeResources->viewport()->mapToGlobal(pos));
    }
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem *item, int)
{
    if (!item) {
        return;
    }

    refreshPropertyPage();

    if (itemType(item) == TreeItemType::Dialogue) {
        Dialogue *dialogue = valueToPtr<Dialogue>(item->data(0, kTreePointerRole));
        for (int i = 0; i < m_project->dialogueCount(); ++i) {
            if (m_project->dialogueAt(i) == dialogue) {
                previewDialogueByRow(i);
                break;
            }
        }
    }
}

void MainWindow::onTreeItemDoubleClicked(QTreeWidgetItem *item, int)
{
    if (!item) {
        return;
    }

    const TreeItemType type = itemType(item);
    if (type == TreeItemType::Character) {
        Character *character = valueToPtr<Character>(item->data(0, kTreePointerRole));
        CharacterEditor dialog(character, m_projectPath, this);
        if (dialog.exec() == QDialog::Accepted) {
            emit dataChanged();
        }
    } else if (type == TreeItemType::Dialogue) {
        Dialogue *dialogue = valueToPtr<Dialogue>(item->data(0, kTreePointerRole));
        DialogueEditDialog dialog(dialogue, m_project, this);
        if (dialog.exec() == QDialog::Accepted) {
            emit dataChanged();
        }
    }
}

void MainWindow::onTimelineClicked(const QModelIndex &index)
{
    previewDialogueByRow(index.row());
}

void MainWindow::onTimelineDoubleClicked(const QModelIndex &index)
{
    Dialogue *dialogue = m_project ? m_project->dialogueAt(index.row()) : nullptr;
    if (!dialogue) {
        return;
    }

    DialogueEditDialog dialog(dialogue, m_project, this);
    if (dialog.exec() == QDialog::Accepted) {
        emit dataChanged();
    }
}

void MainWindow::onCanvasAdvanceRequested()
{
    if (!m_project || m_project->dialogueCount() <= 0) {
        return;
    }

    const int next = qBound(0, m_currentTimelineRow + 1, m_project->dialogueCount() - 1);
    previewDialogueByRow(next);
}

void MainWindow::onNewProject()
{
    const QString parentDir = QFileDialog::getExistingDirectory(this, "选择项目保存目录");
    if (parentDir.isEmpty()) {
        return;
    }

    const QString name = QInputDialog::getText(this, "新建项目", "项目名称");
    if (name.trimmed().isEmpty()) {
        return;
    }

    const QString projectDir = QDir(parentDir).filePath(QString("%1.galproj").arg(name.trimmed()));
    createOrLoadProject(projectDir, false);
}

void MainWindow::onOpenProject()
{
    const QString projectDir = QFileDialog::getExistingDirectory(this, "打开项目目录");
    if (projectDir.isEmpty()) {
        return;
    }

    createOrLoadProject(projectDir, true);
}

void MainWindow::onSaveProject()
{
    if (!m_project) {
        return;
    }

    if (!m_project->saveToFile(m_projectPath)) {
        QMessageBox::warning(this, "保存失败", "项目保存失败，请检查路径或数据有效性。");
        return;
    }

    statusBar()->showMessage("保存成功", 2000);
}

void MainWindow::onAddDialogue()
{
    addDialogue();
}

void MainWindow::onBatchAddDialogues()
{
    if (!m_project) {
        return;
    }

    const QList<Character *> characters = m_project->characters();
    if (characters.isEmpty()) {
        QMessageBox::information(this, "提示", "请先添加至少一个角色。");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("批量生成对话");
    dialog.resize(520, 420);

    auto *characterCombo = new QComboBox(&dialog);
    for (Character *character : characters) {
        characterCombo->addItem(character->name(), character->id());
    }

    auto *countSpin = new QSpinBox(&dialog);
    countSpin->setRange(1, 200);
    countSpin->setValue(10);

    auto *insertRowSpin = new QSpinBox(&dialog);
    insertRowSpin->setRange(1, m_project->dialogueCount() + 1);
    int defaultInsertRow = m_project->dialogueCount() + 1;
    if (m_timelineView && m_timelineView->currentIndex().isValid()) {
        defaultInsertRow = m_timelineView->currentIndex().row() + 2;
    }
    insertRowSpin->setValue(qBound(1, defaultInsertRow, m_project->dialogueCount() + 1));

    auto *templateEdit = new QLineEdit(&dialog);
    templateEdit->setPlaceholderText("例如：第 {n} 句测试文本（{n} 会替换为序号）");

    auto *voicePrefixEdit = new QLineEdit(&dialog);
    voicePrefixEdit->setPlaceholderText("可选：语音名前缀，如 test_");
    if (!characters.isEmpty()) {
        voicePrefixEdit->setText(characters.first()->voicePrefix());
    }

    auto *linesEdit = new QTextEdit(&dialog);
    linesEdit->setPlaceholderText("按行导入台词：每行一条。\n若填写此区域，将优先按行生成并忽略“生成数量/文本模板”。");

    auto *form = new QFormLayout();
    form->addRow("角色", characterCombo);
    form->addRow("插入行号(从1开始)", insertRowSpin);
    form->addRow("生成数量", countSpin);
    form->addRow("文本模板", templateEdit);
    form->addRow("语音前缀", voicePrefixEdit);
    form->addRow("按行导入", linesEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addLayout(form);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString charId = characterCombo->currentData().toString();
    int count = countSpin->value();
    const int requestedInsertRow = insertRowSpin->value();
    int insertIndex = qBound(0, requestedInsertRow - 1, m_project->dialogueCount());
    const QString textTemplate = templateEdit->text().trimmed();
    QString voicePrefix = voicePrefixEdit->text().trimmed();
    if (voicePrefix.isEmpty()) {
        if (const Character *selectedChar = m_project->getCharacter(charId)) {
            voicePrefix = selectedChar->voicePrefix().trimmed();
        }
    }
    const QString linesText = linesEdit->toPlainText();
    const QStringList matchedVoiceFiles = collectVoiceFilesByPrefix(m_projectPath, voicePrefix);

    QStringList importedLines;
    for (const QString &line : linesText.split('\n')) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            importedLines.append(trimmed);
        }
    }

    const bool useImportedLines = !importedLines.isEmpty();
    if (useImportedLines) {
        count = importedLines.size();
    }

    const int nextId = m_project->getNextDialogueId();
    for (int i = 0; i < count; ++i) {
        auto *d = new Dialogue(m_project);
        const int dialogueId = nextId + i;
        d->setId(dialogueId);
        d->setCharacterId(charId);

        QString text;
        if (useImportedLines) {
            text = importedLines.at(i);
        } else if (textTemplate.isEmpty()) {
            text = QString("新对话 %1").arg(dialogueId);
        } else {
            text = textTemplate;
            text.replace("{n}", QString::number(i + 1));
            text.replace("{id}", QString::number(dialogueId));
        }
        d->setText(text);
        QString voiceFile;
        if (!voicePrefix.isEmpty()) {
            if (i < matchedVoiceFiles.size()) {
                voiceFile = matchedVoiceFiles.at(i);
            } else {
                voiceFile = QString("%1%2.wav").arg(voicePrefix).arg(i + 1, 3, 10, QLatin1Char('0'));
            }
        }
        d->setVoiceFile(voiceFile);
        d->setSpecialEffect(Dialogue::SpecialEffect::None);
        m_project->insertDialogueAt(d, insertIndex + i);
    }
    m_project->normalizeDialogueIds();

    emit dataChanged();
    previewDialogueByRow(insertIndex);

    if (!voicePrefix.isEmpty()) {
        if (matchedVoiceFiles.isEmpty()) {
            QMessageBox::information(this,
                                     "语音匹配提示",
                                     "未在 resources/voices 中找到匹配此前缀的语音文件，已按命名规则填充语音文件名。");
        } else if (matchedVoiceFiles.size() < count) {
            QMessageBox::information(this,
                                     "语音匹配提示",
                                     QString("找到 %1 个匹配语音文件，其余已按命名规则填充。").arg(matchedVoiceFiles.size()));
        }
    }
}

void MainWindow::onTogglePreviewMode(bool enabled)
{
    m_previewMode = enabled;
    m_resourcesDock->setVisible(!enabled);
    m_propertiesDock->setVisible(!enabled);
    m_timelineDock->setVisible(!enabled);

    if (enabled) {
        showFullScreen();
    } else {
        showNormal();
    }
}

void MainWindow::onExportGame()
{
    if (!m_project) {
        QMessageBox::warning(this, "失败", "当前没有可导出的项目。");
        return;
    }

    ExportDialog dialog(m_project, m_projectPath, this);
    dialog.exec();
}

void MainWindow::onTestRun()
{
    if (!m_project) {
        QMessageBox::warning(this, "失败", "当前没有可测试运行的项目。");
        return;
    }

    // 不能使用函数内 QTemporaryDir：函数返回会自动销毁，播放器会读不到 game.json。
    const QString uniqueName = QString("galgame_test_run_%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString tempBase = QDir::temp().filePath(uniqueName);
    QDir tempDir(tempBase);
    if (tempDir.exists()) {
        tempDir.removeRecursively();
    }
    if (!tempDir.mkpath(".")) {
        QMessageBox::warning(this, "失败", "无法创建临时测试目录。");
        return;
    }
    m_lastTestRunDir = tempBase;

    ExportOptions opts;
    opts.outputPath = m_lastTestRunDir;
    opts.gameTitle = m_project->name();
    opts.windowSize = QSize(1280, 720);
    opts.outputFolderOnly = true;

    Exporter exporter;
    QString error;
    if (!exporter.exportGame(*m_project, m_projectPath, opts, &error)) {
        QMessageBox::warning(this, "失败", QString("测试导出失败：%1").arg(error));
        return;
    }

    auto *player = new PlayerWindow();
    player->setAttribute(Qt::WA_DeleteOnClose);
    QString loadError;
    if (!player->loadGame(m_lastTestRunDir, &loadError)) {
        QMessageBox::warning(this, "失败", QString("测试播放器加载失败：%1").arg(loadError));
        player->deleteLater();
        return;
    }
    player->showMaximized();
}

void MainWindow::onShowPropertiesDock()
{
    if (!m_propertiesDock) {
        return;
    }
    m_propertiesDock->setVisible(true);
    m_propertiesDock->raise();
    if (m_stackProperties) {
        m_stackProperties->setFocus();
    }
}

void MainWindow::onProjectPropertiesChanged()
{
    if (m_updatingPropertyWidgets || !m_project) {
        return;
    }
    m_project->setStartMenuBackgroundPath(m_projectStartBgEdit->text().trimmed());
    m_project->setStartMenuBgmPath(m_projectStartBgmEdit->text().trimmed());
}

void MainWindow::onCharacterPropertiesChanged()
{
    if (m_updatingPropertyWidgets) {
        return;
    }

    Character *character = selectedCharacter();
    if (!character) {
        return;
    }

    character->setName(m_charNameEdit->text().trimmed());
    character->setPortraitPath(m_charPortraitEdit->text().trimmed());
    character->setPosition(textToPosition(m_charPositionCombo->currentText()));
    character->setPortraitScale(m_charScaleSpin->value());
    character->setVoicePrefix(m_charVoicePrefixEdit->text().trimmed());

    // 角色文本输入过程中不做全量刷新，避免树和属性页重建导致重入/闪退。
}

void MainWindow::onBackgroundPropertiesChanged()
{
    if (m_updatingPropertyWidgets) {
        return;
    }

    Background *background = selectedBackground();
    if (!background) {
        return;
    }

    int start = m_bgStartSpin->value();
    int end = m_bgEndSpin->value();
    if (start > end) {
        std::swap(start, end);
        m_bgStartSpin->setValue(start);
        m_bgEndSpin->setValue(end);
    }

    background->setImagePath(m_bgImageEdit->text().trimmed());
    background->setBgmPath(m_bgBgmEdit->text().trimmed());
    background->setStartDialogueId(start);
    background->setEndDialogueId(end);
    refreshBackgroundConflictLabel(background);

    // 背景行号输入过程中不做全量刷新，避免控件重建导致焦点丢失。
}

void MainWindow::onDialoguePropertiesChanged()
{
    if (m_updatingPropertyWidgets) {
        return;
    }

    Dialogue *dialogue = selectedDialogue();
    if (!dialogue) {
        return;
    }

    dialogue->setCharacterId(m_dialogueCharacterCombo->currentData().toString());
    dialogue->setText(m_dialogueTextEdit->toPlainText());
    dialogue->setVoiceFile(m_dialogueVoiceEdit->text().trimmed());
    dialogue->setSpecialEffect(textToEffect(m_dialogueEffectCombo->currentText()));

    // 文本输入过程中不做全量刷新，避免重建树/属性页导致编辑焦点丢失。
    QObject *trigger = sender();
    if (trigger == m_dialogueCharacterCombo || trigger == m_dialogueEffectCombo) {
        emit dataChanged();
    }
}

void MainWindow::onBrowseBackgroundImage()
{
    if (!m_project) {
        return;
    }

    Background *background = selectedBackground();
    if (!background) {
        QMessageBox::information(this, "提示", "请先在左侧选择一个背景。");
        return;
    }

    const QString sourcePath = QFileDialog::getOpenFileName(this, "选择背景图片", QString(), "Images (*.png *.jpg *.jpeg *.webp)");
    if (sourcePath.isEmpty()) {
        return;
    }

    QDir projectDir(QDir::cleanPath(m_projectPath));
    if (!projectDir.exists() && !projectDir.mkpath(".")) {
        QMessageBox::warning(this, "失败", "项目目录不可用。");
        return;
    }
    if (!projectDir.mkpath("resources/backgrounds")) {
        QMessageBox::warning(this, "失败", "无法创建 resources/backgrounds 目录。");
        return;
    }

    const QFileInfo sourceInfo(sourcePath);
    const QString ext = sourceInfo.suffix().isEmpty() ? "png" : sourceInfo.suffix();
    const QString fileName = QString("bg_%1.%2").arg(background->id(), ext);
    const QString relativePath = QDir::cleanPath(QString("resources/backgrounds/%1").arg(fileName));
    const QString absoluteTarget = projectDir.filePath(relativePath);

    QFile::remove(absoluteTarget);
    if (!QFile::copy(sourcePath, absoluteTarget)) {
        QMessageBox::warning(this, "失败", "复制背景图片失败。");
        return;
    }

    background->setImagePath(relativePath);
    m_bgImageEdit->setText(relativePath);
    emit dataChanged();
}

void MainWindow::onBrowseBackgroundBgm()
{
    if (!m_project) {
        return;
    }

    Background *background = selectedBackground();
    if (!background) {
        QMessageBox::information(this, "提示", "请先在左侧选择一个背景。");
        return;
    }

    const QString sourcePath = QFileDialog::getOpenFileName(this,
                                                            "选择背景音乐",
                                                            QString(),
                                                            "Audio (*.mp3 *.ogg *.wav *.flac *.m4a)");
    if (sourcePath.isEmpty()) {
        return;
    }

    QDir projectDir(QDir::cleanPath(m_projectPath));
    if (!projectDir.exists() && !projectDir.mkpath(".")) {
        QMessageBox::warning(this, "失败", "项目目录不可用。");
        return;
    }
    if (!projectDir.mkpath("resources/bgm")) {
        QMessageBox::warning(this, "失败", "无法创建 resources/bgm 目录。");
        return;
    }

    const QFileInfo sourceInfo(sourcePath);
    const QString ext = sourceInfo.suffix().isEmpty() ? "ogg" : sourceInfo.suffix();
    const QString fileName = QString("bgm_%1.%2").arg(background->id(), ext);
    const QString relativePath = QDir::cleanPath(QString("resources/bgm/%1").arg(fileName));
    const QString absoluteTarget = projectDir.filePath(relativePath);

    QFile::remove(absoluteTarget);
    if (!QFile::copy(sourcePath, absoluteTarget)) {
        QMessageBox::warning(this, "失败", "复制背景音乐失败。");
        return;
    }

    background->setBgmPath(relativePath);
    m_bgBgmEdit->setText(relativePath);
    emit dataChanged();
}

void MainWindow::onBrowseDialogueVoice()
{
    if (!m_project) {
        return;
    }

    Dialogue *dialogue = selectedDialogue();
    if (!dialogue) {
        QMessageBox::information(this, "提示", "请先在左侧选择一条对话。");
        return;
    }

    const QString sourcePath = QFileDialog::getOpenFileName(this,
                                                            "选择语音文件",
                                                            QString(),
                                                            "Audio (*.wav *.ogg *.mp3 *.flac *.m4a)");
    if (sourcePath.isEmpty()) {
        return;
    }

    QDir projectDir(QDir::cleanPath(m_projectPath));
    if (!projectDir.exists() && !projectDir.mkpath(".")) {
        QMessageBox::warning(this, "失败", "项目目录不可用。");
        return;
    }
    if (!projectDir.mkpath("resources/voices")) {
        QMessageBox::warning(this, "失败", "无法创建 resources/voices 目录。");
        return;
    }

    const QFileInfo sourceInfo(sourcePath);
    const QString ext = sourceInfo.suffix().isEmpty() ? "wav" : sourceInfo.suffix();
    const QString fileName = QString("voice_%1.%2").arg(dialogue->id()).arg(ext);
    const QString relativePath = QDir::cleanPath(QString("resources/voices/%1").arg(fileName));
    const QString absoluteTarget = projectDir.filePath(relativePath);

    QFile::remove(absoluteTarget);
    if (!QFile::copy(sourcePath, absoluteTarget)) {
        QMessageBox::warning(this, "失败", "复制语音文件失败。");
        return;
    }

    // 对话里只存文件名，导出器会按 resources/voices/<voiceFile> 查找。
    const QString voiceFileName = QFileInfo(relativePath).fileName();
    dialogue->setVoiceFile(voiceFileName);
    m_dialogueVoiceEdit->setText(voiceFileName);
    emit dataChanged();
}

void MainWindow::onBrowseStartMenuBackground()
{
    if (!m_project) {
        return;
    }

    const QString sourcePath = QFileDialog::getOpenFileName(this, "选择开始界面背景", QString(), "Images (*.png *.jpg *.jpeg *.webp)");
    if (sourcePath.isEmpty()) {
        return;
    }

    QDir projectDir(QDir::cleanPath(m_projectPath));
    if (!projectDir.exists() && !projectDir.mkpath(".")) {
        QMessageBox::warning(this, "失败", "项目目录不可用。");
        return;
    }
    if (!projectDir.mkpath("resources/ui")) {
        QMessageBox::warning(this, "失败", "无法创建 resources/ui 目录。");
        return;
    }

    const QFileInfo sourceInfo(sourcePath);
    const QString ext = sourceInfo.suffix().isEmpty() ? "png" : sourceInfo.suffix();
    const QString relativePath = QDir::cleanPath(QString("resources/ui/start_bg.%1").arg(ext));
    const QString absoluteTarget = projectDir.filePath(relativePath);
    QFile::remove(absoluteTarget);
    if (!QFile::copy(sourcePath, absoluteTarget)) {
        QMessageBox::warning(this, "失败", "复制开始界面背景失败。");
        return;
    }

    m_project->setStartMenuBackgroundPath(relativePath);
    m_projectStartBgEdit->setText(relativePath);
    emit dataChanged();
}

void MainWindow::onBrowseStartMenuBgm()
{
    if (!m_project) {
        return;
    }

    const QString sourcePath = QFileDialog::getOpenFileName(this,
                                                            "选择开始界面背景音乐",
                                                            QString(),
                                                            "Audio (*.mp3 *.ogg *.wav *.flac *.m4a)");
    if (sourcePath.isEmpty()) {
        return;
    }

    QDir projectDir(QDir::cleanPath(m_projectPath));
    if (!projectDir.exists() && !projectDir.mkpath(".")) {
        QMessageBox::warning(this, "失败", "项目目录不可用。");
        return;
    }
    if (!projectDir.mkpath("resources/bgm")) {
        QMessageBox::warning(this, "失败", "无法创建 resources/bgm 目录。");
        return;
    }

    const QFileInfo sourceInfo(sourcePath);
    const QString ext = sourceInfo.suffix().isEmpty() ? "ogg" : sourceInfo.suffix();
    const QString relativePath = QDir::cleanPath(QString("resources/bgm/start_menu.%1").arg(ext));
    const QString absoluteTarget = projectDir.filePath(relativePath);
    QFile::remove(absoluteTarget);
    if (!QFile::copy(sourcePath, absoluteTarget)) {
        QMessageBox::warning(this, "失败", "复制开始界面音乐失败。");
        return;
    }

    m_project->setStartMenuBgmPath(relativePath);
    m_projectStartBgmEdit->setText(relativePath);
    emit dataChanged();
}

void MainWindow::onDeleteSelectedItem()
{
    QTreeWidgetItem *item = m_treeResources ? m_treeResources->currentItem() : nullptr;
    if (!item || !m_project) {
        return;
    }

    const TreeItemType type = itemType(item);
    if (type == TreeItemType::Character) {
        deleteCharacter(valueToPtr<Character>(item->data(0, kTreePointerRole)));
    } else if (type == TreeItemType::Background) {
        deleteBackground(valueToPtr<Background>(item->data(0, kTreePointerRole)));
    } else if (type == TreeItemType::Dialogue) {
        deleteDialogue(valueToPtr<Dialogue>(item->data(0, kTreePointerRole)));
    }
}

void MainWindow::setupLayout()
{
    setupCanvas();
    setupTreeDock();
    setupPropertyDock();
    setupTimelineDock();
}

void MainWindow::setupMenusAndToolbar()
{
    auto *fileMenu = menuBar()->addMenu("文件");
    fileMenu->addAction("新建项目", this, &MainWindow::onNewProject);
    fileMenu->addAction("打开项目", this, &MainWindow::onOpenProject);
    fileMenu->addAction("保存项目", this, &MainWindow::onSaveProject);

    auto *editMenu = menuBar()->addMenu("编辑");
    editMenu->addAction("添加角色", this, &MainWindow::addCharacter);
    editMenu->addAction("添加背景", this, &MainWindow::addBackground);
    editMenu->addAction("添加对话", this, &MainWindow::onAddDialogue);
    editMenu->addAction("批量生成对话", this, &MainWindow::onBatchAddDialogues);

    auto *toolsMenu = menuBar()->addMenu("工具");
    toolsMenu->addAction("导出游戏", this, &MainWindow::onExportGame);
    toolsMenu->addAction("测试运行", this, &MainWindow::onTestRun);
    toolsMenu->addAction("打开属性编辑器", this, &MainWindow::onShowPropertiesDock);

    auto *toolbar = addToolBar("主工具栏");
    toolbar->addAction("添加角色", this, &MainWindow::addCharacter);
    toolbar->addAction("添加背景", this, &MainWindow::addBackground);
    toolbar->addAction("添加对话", this, &MainWindow::onAddDialogue);
    toolbar->addAction("批量生成对话", this, &MainWindow::onBatchAddDialogues);
    toolbar->addAction("属性编辑器", this, &MainWindow::onShowPropertiesDock);
    m_actionPreviewMode = toolbar->addAction("预览模式");
    m_actionPreviewMode->setCheckable(true);
    connect(m_actionPreviewMode, &QAction::toggled, this, &MainWindow::onTogglePreviewMode);
}

void MainWindow::setupTreeDock()
{
    m_resourcesDock = new QDockWidget("资源管理器", this);
    m_resourcesDock->setMinimumWidth(250);
    m_resourcesDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    m_treeResources = new QTreeWidget(m_resourcesDock);
    m_treeResources->setObjectName("treeResources");
    m_treeResources->setHeaderHidden(true);
    m_treeResources->setContextMenuPolicy(Qt::CustomContextMenu);
    m_resourcesDock->setWidget(m_treeResources);
    addDockWidget(Qt::LeftDockWidgetArea, m_resourcesDock);

    connect(m_treeResources, &QTreeWidget::customContextMenuRequested, this, &MainWindow::onTreeContextMenuRequested);
    connect(m_treeResources, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);
    connect(m_treeResources, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onTreeItemDoubleClicked);
}

void MainWindow::setupPropertyDock()
{
    m_propertiesDock = new QDockWidget("属性编辑器", this);
    m_propertiesDock->setMinimumWidth(300);

    m_stackProperties = new QStackedWidget(m_propertiesDock);
    m_stackProperties->setObjectName("stackProperties");
    m_propertiesDock->setWidget(m_stackProperties);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);

    auto *page0 = new QWidget();
    auto *page0Layout = new QVBoxLayout(page0);
    m_emptyPropertyLabel = new QLabel("项目开始界面设置", page0);
    page0Layout->addWidget(m_emptyPropertyLabel);
    auto *projectForm = new QFormLayout();
    m_projectStartBgEdit = new QLineEdit(page0);
    m_projectStartBgBrowseButton = new QPushButton("浏览...", page0);
    auto *projectBgRow = new QHBoxLayout();
    projectBgRow->addWidget(m_projectStartBgEdit);
    projectBgRow->addWidget(m_projectStartBgBrowseButton);

    m_projectStartBgmEdit = new QLineEdit(page0);
    m_projectStartBgmBrowseButton = new QPushButton("浏览...", page0);
    auto *projectBgmRow = new QHBoxLayout();
    projectBgmRow->addWidget(m_projectStartBgmEdit);
    projectBgmRow->addWidget(m_projectStartBgmBrowseButton);

    projectForm->addRow("开始界面背景", projectBgRow);
    projectForm->addRow("开始界面音乐", projectBgmRow);
    page0Layout->addLayout(projectForm);
    page0Layout->addStretch(1);
    m_stackProperties->addWidget(page0);

    auto *page1 = new QWidget();
    auto *form1 = new QFormLayout(page1);
    m_charNameEdit = new QLineEdit(page1);
    m_charPortraitEdit = new QLineEdit(page1);
    m_charPositionCombo = new QComboBox(page1);
    m_charPositionCombo->addItems({"左", "中", "右"});
    m_charScaleSpin = new QSpinBox(page1);
    m_charScaleSpin->setRange(10, 300);
    m_charScaleSpin->setSuffix("%");
    m_charVoicePrefixEdit = new QLineEdit(page1);
    form1->addRow("名称", m_charNameEdit);
    form1->addRow("立绘路径", m_charPortraitEdit);
    form1->addRow("位置", m_charPositionCombo);
    form1->addRow("立绘大小", m_charScaleSpin);
    form1->addRow("语音前缀", m_charVoicePrefixEdit);
    m_stackProperties->addWidget(page1);

    auto *page2 = new QWidget();
    auto *form2 = new QFormLayout(page2);
    m_bgImageEdit = new QLineEdit(page2);
    m_bgBrowseButton = new QPushButton("浏览...", page2);
    auto *bgImageRow = new QHBoxLayout();
    bgImageRow->addWidget(m_bgImageEdit);
    bgImageRow->addWidget(m_bgBrowseButton);
    m_bgBgmEdit = new QLineEdit(page2);
    m_bgBgmBrowseButton = new QPushButton("浏览...", page2);
    auto *bgBgmRow = new QHBoxLayout();
    bgBgmRow->addWidget(m_bgBgmEdit);
    bgBgmRow->addWidget(m_bgBgmBrowseButton);
    m_bgStartSpin = new QSpinBox(page2);
    m_bgEndSpin = new QSpinBox(page2);
    m_bgStartSpin->setRange(1, 99999);
    m_bgEndSpin->setRange(1, 99999);
    m_bgConflictLabel = new QLabel(page2);
    m_bgConflictLabel->setStyleSheet("color:red;");
    form2->addRow("图片路径", bgImageRow);
    form2->addRow("背景音乐", bgBgmRow);
    form2->addRow("起始句", m_bgStartSpin);
    form2->addRow("结束句", m_bgEndSpin);
    form2->addRow("冲突检测", m_bgConflictLabel);
    m_stackProperties->addWidget(page2);

    auto *page3 = new QWidget();
    auto *form3 = new QFormLayout(page3);
    m_dialogueCharacterCombo = new QComboBox(page3);
    m_dialogueTextEdit = new QTextEdit(page3);
    m_dialogueVoiceEdit = new QLineEdit(page3);
    m_dialogueVoiceBrowseButton = new QPushButton("浏览...", page3);
    auto *dialogueVoiceRow = new QHBoxLayout();
    dialogueVoiceRow->addWidget(m_dialogueVoiceEdit);
    dialogueVoiceRow->addWidget(m_dialogueVoiceBrowseButton);
    m_dialogueEffectCombo = new QComboBox(page3);
    m_dialogueEffectCombo->addItems({"none", "shake", "flash", "fade"});
    form3->addRow("角色", m_dialogueCharacterCombo);
    form3->addRow("文本", m_dialogueTextEdit);
    form3->addRow("语音文件", dialogueVoiceRow);
    form3->addRow("特效", m_dialogueEffectCombo);
    m_stackProperties->addWidget(page3);

    m_stackProperties->setCurrentIndex(0);

    connect(m_charNameEdit, &QLineEdit::textEdited, this, &MainWindow::onCharacterPropertiesChanged);
    connect(m_charPortraitEdit, &QLineEdit::textEdited, this, &MainWindow::onCharacterPropertiesChanged);
    connect(m_charPositionCombo, &QComboBox::currentTextChanged, this, &MainWindow::onCharacterPropertiesChanged);
    connect(m_charScaleSpin, &QSpinBox::valueChanged, this, &MainWindow::onCharacterPropertiesChanged);
    connect(m_charVoicePrefixEdit, &QLineEdit::textEdited, this, &MainWindow::onCharacterPropertiesChanged);
    connect(m_charNameEdit, &QLineEdit::editingFinished, this, [this]() { emit dataChanged(); });
    connect(m_charPortraitEdit, &QLineEdit::editingFinished, this, [this]() { emit dataChanged(); });
    connect(m_charVoicePrefixEdit, &QLineEdit::editingFinished, this, [this]() { emit dataChanged(); });

    connect(m_bgImageEdit, &QLineEdit::textEdited, this, &MainWindow::onBackgroundPropertiesChanged);
    connect(m_bgBrowseButton, &QPushButton::clicked, this, &MainWindow::onBrowseBackgroundImage);
    connect(m_bgBgmEdit, &QLineEdit::textEdited, this, &MainWindow::onBackgroundPropertiesChanged);
    connect(m_bgBgmBrowseButton, &QPushButton::clicked, this, &MainWindow::onBrowseBackgroundBgm);
    connect(m_bgStartSpin, &QSpinBox::valueChanged, this, &MainWindow::onBackgroundPropertiesChanged);
    connect(m_bgEndSpin, &QSpinBox::valueChanged, this, &MainWindow::onBackgroundPropertiesChanged);
    connect(m_bgImageEdit, &QLineEdit::editingFinished, this, [this]() { emit dataChanged(); });
    connect(m_bgBgmEdit, &QLineEdit::editingFinished, this, [this]() { emit dataChanged(); });
    connect(m_bgStartSpin, &QSpinBox::editingFinished, this, [this]() { emit dataChanged(); });
    connect(m_bgEndSpin, &QSpinBox::editingFinished, this, [this]() { emit dataChanged(); });

    connect(m_dialogueCharacterCombo, &QComboBox::currentTextChanged, this, &MainWindow::onDialoguePropertiesChanged);
    connect(m_dialogueTextEdit, &QTextEdit::textChanged, this, &MainWindow::onDialoguePropertiesChanged);
    connect(m_dialogueVoiceEdit, &QLineEdit::textEdited, this, &MainWindow::onDialoguePropertiesChanged);
    connect(m_dialogueVoiceBrowseButton, &QPushButton::clicked, this, &MainWindow::onBrowseDialogueVoice);
    connect(m_dialogueEffectCombo, &QComboBox::currentTextChanged, this, &MainWindow::onDialoguePropertiesChanged);

    connect(m_projectStartBgEdit, &QLineEdit::textEdited, this, &MainWindow::onProjectPropertiesChanged);
    connect(m_projectStartBgmEdit, &QLineEdit::textEdited, this, &MainWindow::onProjectPropertiesChanged);
    connect(m_projectStartBgBrowseButton, &QPushButton::clicked, this, &MainWindow::onBrowseStartMenuBackground);
    connect(m_projectStartBgmBrowseButton, &QPushButton::clicked, this, &MainWindow::onBrowseStartMenuBgm);
    connect(m_projectStartBgEdit, &QLineEdit::editingFinished, this, [this]() { emit dataChanged(); });
    connect(m_projectStartBgmEdit, &QLineEdit::editingFinished, this, [this]() { emit dataChanged(); });
}

void MainWindow::setupTimelineDock()
{
    m_timelineDock = new QDockWidget("对话时间轴", this);
    m_timelineDock->setMinimumHeight(150);

    m_timelineView = new QListView(m_timelineDock);
    m_timelineModel = new DialogueTimelineModel(this);
    m_timelineView->setModel(m_timelineModel);
    m_timelineView->setDragDropMode(QAbstractItemView::InternalMove);
    m_timelineView->setDefaultDropAction(Qt::MoveAction);
    m_timelineView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_timelineDock->setWidget(m_timelineView);
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);

    connect(m_timelineView, &QListView::clicked, this, &MainWindow::onTimelineClicked);
    connect(m_timelineView, &QListView::doubleClicked, this, &MainWindow::onTimelineDoubleClicked);
    connect(m_timelineModel, &QAbstractItemModel::rowsMoved, this, [this]() {
        emit dataChanged();
    });
}

void MainWindow::setupCanvas()
{
    m_canvas = new GameCanvas(this);
    m_canvas->setResourceBaseDir(m_projectPath);
    setCentralWidget(m_canvas);
    connect(m_canvas, &GameCanvas::advanceRequested, this, &MainWindow::onCanvasAdvanceRequested);
}

void MainWindow::createOrLoadProject(const QString &projectPath, bool loadIfExists)
{
    if (m_project) {
        m_project->deleteLater();
        m_project = nullptr;
    }

    m_projectPath = QDir::cleanPath(projectPath);
    if (m_canvas) {
        m_canvas->setResourceBaseDir(m_projectPath);
    }

    Project *loaded = nullptr;
    const QString projectJsonPath = QDir(m_projectPath).filePath("project.json");
    QString loadError;
    if (loadIfExists && QFileInfo::exists(projectJsonPath)) {
        loaded = Project::loadFromFile(m_projectPath, this, &loadError);
        if (!loaded && !loadError.isEmpty()) {
            QMessageBox::warning(this, "打开项目失败", loadError);
        }
    }

    if (!loaded) {
        loaded = new Project(this);
        loaded->setName(QFileInfo(m_projectPath).baseName());
    }

    m_project = loaded;
    m_timelineModel->setProject(m_project);
    refreshAllViews();
}

void MainWindow::refreshAllViews()
{
    refreshTree();
    refreshTimeline();
    refreshPropertyPage();

    // 进入编辑器后默认预览第一条对话，避免中间画布初始黑屏。
    if (m_project && m_project->dialogueCount() > 0) {
        previewDialogueByRow(0);
    } else if (m_canvas) {
        m_canvas->clearScene();
        m_currentTimelineRow = -1;
    }
}

void MainWindow::refreshTree()
{
    m_treeResources->clear();
    if (!m_project) {
        return;
    }

    m_rootItem = new QTreeWidgetItem(m_treeResources, {m_project->name()});
    setItemType(m_rootItem, TreeItemType::Root);
    m_rootItem->setFlags(m_rootItem->flags() & ~Qt::ItemIsEditable);

    m_charactersGroupItem = new QTreeWidgetItem(m_rootItem, {"[角色]"});
    setItemType(m_charactersGroupItem, TreeItemType::CharactersGroup);

    m_backgroundsGroupItem = new QTreeWidgetItem(m_rootItem, {"[背景]"});
    setItemType(m_backgroundsGroupItem, TreeItemType::BackgroundsGroup);

    m_dialoguesGroupItem = new QTreeWidgetItem(m_rootItem, {"[对话]"});
    setItemType(m_dialoguesGroupItem, TreeItemType::DialoguesGroup);

    for (Character *character : m_project->characters()) {
        auto *item = new QTreeWidgetItem(m_charactersGroupItem, {character->name()});
        setItemType(item, TreeItemType::Character);
        item->setData(0, kTreePointerRole, QVariant::fromValue(ptrToValue(character)));
    }

    for (Background *background : m_project->backgrounds()) {
        const QString text = QString("%1 [%2-%3]").arg(background->id()).arg(background->startDialogueId()).arg(background->endDialogueId());
        auto *item = new QTreeWidgetItem(m_backgroundsGroupItem, {text});
        setItemType(item, TreeItemType::Background);
        item->setData(0, kTreePointerRole, QVariant::fromValue(ptrToValue(background)));
    }

    for (Dialogue *dialogue : m_project->dialogues()) {
        const Character *character = m_project->getCharacter(dialogue->characterId());
        auto *item = new QTreeWidgetItem(m_dialoguesGroupItem, {dialogueSummary(dialogue, character)});
        setItemType(item, TreeItemType::Dialogue);
        item->setData(0, kTreePointerRole, QVariant::fromValue(ptrToValue(dialogue)));
    }

    m_treeResources->expandAll();
}

void MainWindow::refreshTimeline()
{
    m_timelineModel->setProject(m_project);
}

void MainWindow::refreshPropertyPage()
{
    m_updatingPropertyWidgets = true;

    if (!m_project) {
        m_stackProperties->setCurrentIndex(0);
        m_updatingPropertyWidgets = false;
        return;
    }

    m_dialogueCharacterCombo->clear();
    m_projectStartBgEdit->setText(m_project->startMenuBackgroundPath());
    m_projectStartBgmEdit->setText(m_project->startMenuBgmPath());
    const QList<Character *> characters = m_project->characters();
    for (Character *character : characters) {
        m_dialogueCharacterCombo->addItem(character->name(), character->id());
    }

    Character *character = selectedCharacter();
    if (character) {
        m_stackProperties->setCurrentIndex(1);
        m_charNameEdit->setText(character->name());
        m_charPortraitEdit->setText(character->portraitPath());
        m_charPositionCombo->setCurrentText(positionToText(character->position()));
        m_charScaleSpin->setValue(character->portraitScale());
        m_charVoicePrefixEdit->setText(character->voicePrefix());
        m_updatingPropertyWidgets = false;
        return;
    }

    Background *background = selectedBackground();
    if (background) {
        m_stackProperties->setCurrentIndex(2);
        m_bgImageEdit->setText(background->imagePath());
        m_bgBgmEdit->setText(background->bgmPath());
        m_bgStartSpin->setValue(background->startDialogueId());
        m_bgEndSpin->setValue(background->endDialogueId());
        refreshBackgroundConflictLabel(background);
        m_updatingPropertyWidgets = false;
        return;
    }

    Dialogue *dialogue = selectedDialogue();
    if (dialogue) {
        m_stackProperties->setCurrentIndex(3);
        const int index = m_dialogueCharacterCombo->findData(dialogue->characterId());
        if (index >= 0) {
            m_dialogueCharacterCombo->setCurrentIndex(index);
        }
        m_dialogueTextEdit->setPlainText(dialogue->text());
        m_dialogueVoiceEdit->setText(dialogue->voiceFile());
        m_dialogueEffectCombo->setCurrentText(effectToText(dialogue->specialEffect()));
        m_updatingPropertyWidgets = false;
        return;
    }

    m_stackProperties->setCurrentIndex(0);
    m_updatingPropertyWidgets = false;
}

void MainWindow::refreshBackgroundConflictLabel(Background *target)
{
    if (!target || !m_project) {
        m_bgConflictLabel->clear();
        return;
    }

    bool conflicted = false;
    for (Background *background : m_project->backgrounds()) {
        if (background == target) {
            continue;
        }
        if (target->conflictsWith(*background)) {
            conflicted = true;
            break;
        }
    }

    m_bgConflictLabel->setText(conflicted ? "警告：范围冲突" : "无冲突");
}

void MainWindow::previewDialogueByRow(int row)
{
    if (!m_project) {
        return;
    }

    Dialogue *dialogue = m_project->dialogueAt(row);
    if (!dialogue) {
        return;
    }

    const Character *character = m_project->getCharacter(dialogue->characterId());
    if (!character) {
        return;
    }

    const Background *background = m_project->getBackgroundForDialogue(dialogue->id());
    Background fallback;
    fallback.setImagePath(QString());

    m_canvas->setDialogue(*dialogue, *character, background ? *background : fallback);
    m_currentTimelineRow = row;

    const QModelIndex index = m_timelineModel->index(row, 0);
    if (index.isValid()) {
        m_timelineView->setCurrentIndex(index);
        m_timelineView->scrollTo(index);
    }
}

void MainWindow::addCharacter()
{
    if (!m_project) {
        return;
    }

    const int nextIndex = m_project->characters().size() + 1;
    auto *character = new Character(m_project);
    character->setId(QString("char%1").arg(nextIndex));
    character->setName(QString("角色%1").arg(nextIndex));
    character->setPosition(Character::Position::Center);
    m_project->addCharacter(character);

    CharacterEditor dialog(character, m_projectPath, this);
    dialog.exec();
    emit dataChanged();
}

void MainWindow::addBackground()
{
    if (!m_project) {
        return;
    }

    const int nextIndex = m_project->backgrounds().size() + 1;
    auto *background = new Background(m_project);
    background->setId(QString("bg%1").arg(nextIndex));
    background->setStartDialogueId(1);
    background->setEndDialogueId(1);
    m_project->addBackground(background);
    emit dataChanged();
}

void MainWindow::addDialogue()
{
    if (!m_project) {
        return;
    }

    const QList<Character *> characters = m_project->characters();
    if (characters.isEmpty()) {
        QMessageBox::information(this, "提示", "请先添加至少一个角色。");
        return;
    }

    auto *dialogue = new Dialogue(m_project);
    dialogue->setId(m_project->getNextDialogueId());
    dialogue->setCharacterId(characters.first()->id());
    dialogue->setText("新对话");
    dialogue->setVoiceFile(QString());
    dialogue->setSpecialEffect(Dialogue::SpecialEffect::None);
    m_project->addDialogue(dialogue);

    emit dataChanged();

    const int row = m_project->dialogueCount() - 1;
    if (row >= 0) {
        previewDialogueByRow(row);
    }
}

void MainWindow::deleteCharacter(Character *character)
{
    if (!character || !m_project) {
        return;
    }

    const auto ret = QMessageBox::question(this,
                                           "删除角色",
                                           QString("确定删除角色“%1”？关联对话也会删除。").arg(character->name()));
    if (ret != QMessageBox::Yes) {
        return;
    }

    if (m_project->removeCharacterById(character->id())) {
        emit dataChanged();
    }
}

void MainWindow::deleteBackground(Background *background)
{
    if (!background || !m_project) {
        return;
    }

    const auto ret = QMessageBox::question(this, "删除背景", QString("确定删除背景“%1”？").arg(background->id()));
    if (ret != QMessageBox::Yes) {
        return;
    }

    if (m_project->removeBackgroundById(background->id())) {
        emit dataChanged();
    }
}

void MainWindow::deleteDialogue(Dialogue *dialogue)
{
    if (!dialogue || !m_project) {
        return;
    }

    const auto ret = QMessageBox::question(this, "删除对话", QString("确定删除对话 #%1？").arg(dialogue->id()));
    if (ret != QMessageBox::Yes) {
        return;
    }

    if (m_project->removeDialogueById(dialogue->id())) {
        emit dataChanged();
    }
}

Character *MainWindow::selectedCharacter() const
{
    QTreeWidgetItem *item = m_treeResources->currentItem();
    if (!item || itemType(item) != TreeItemType::Character) {
        return nullptr;
    }
    Character *candidate = valueToPtr<Character>(item->data(0, kTreePointerRole));
    if (!candidate || !m_project) {
        return nullptr;
    }
    for (Character *character : m_project->characters()) {
        if (character == candidate) {
            return character;
        }
    }
    return nullptr;
}

Background *MainWindow::selectedBackground() const
{
    QTreeWidgetItem *item = m_treeResources->currentItem();
    if (!item || itemType(item) != TreeItemType::Background) {
        return nullptr;
    }
    Background *candidate = valueToPtr<Background>(item->data(0, kTreePointerRole));
    if (!candidate || !m_project) {
        return nullptr;
    }
    for (Background *background : m_project->backgrounds()) {
        if (background == candidate) {
            return background;
        }
    }
    return nullptr;
}

Dialogue *MainWindow::selectedDialogue() const
{
    QTreeWidgetItem *item = m_treeResources->currentItem();
    if (!item || itemType(item) != TreeItemType::Dialogue) {
        return nullptr;
    }
    Dialogue *candidate = valueToPtr<Dialogue>(item->data(0, kTreePointerRole));
    if (!candidate || !m_project) {
        return nullptr;
    }
    for (Dialogue *dialogue : m_project->dialogues()) {
        if (dialogue == candidate) {
            return dialogue;
        }
    }
    return nullptr;
}

MainWindow::TreeItemType MainWindow::itemType(const QTreeWidgetItem *item) const
{
    return static_cast<TreeItemType>(item->data(0, Qt::UserRole).toInt());
}

void MainWindow::setItemType(QTreeWidgetItem *item, TreeItemType type) const
{
    item->setData(0, Qt::UserRole, static_cast<int>(type));
}
