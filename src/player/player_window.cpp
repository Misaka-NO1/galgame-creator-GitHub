#include "player_window.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QAudioOutput>
#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaPlayer>
#include <QUrl>
#include <QEvent>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSlider>
#include <QStackedWidget>
#include <QShortcut>
#include <QPainter>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>

namespace {

constexpr int kMaxSaveSlots = 3;
constexpr const char *kBgmVolumeKey = "audio/bgmVolume";
constexpr const char *kVoiceVolumeKey = "audio/voiceVolume";

QString normalizeColor(const QString &raw, const QString &fallback)
{
    const QColor color(raw.trimmed());
    if (!color.isValid()) {
        return fallback;
    }
    return color.name(QColor::HexRgb).toUpper();
}

QPixmap makeNotFound(int w, int h)
{
    QPixmap pm(w, h);
    pm.fill(QColorConstants::Black);
    QPainter p(&pm);
    p.setPen(QColorConstants::Red);
    QFont f;
    f.setPointSize(18);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, "Image Not Found");
    return pm;
}

QString simplifyPreviewText(const QString &text)
{
    QString preview = text.trimmed();
    preview.replace('\n', ' ');
    if (preview.size() > 16) {
        preview = preview.left(16) + "...";
    }
    return preview;
}

} // namespace

PlayerWindow::PlayerWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_typewriter(this)
{
    m_view = new QGraphicsView(this);
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 1280, 720);
    m_view->setScene(m_scene);
    m_view->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    m_view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setBackgroundBrush(QColorConstants::Black);
    m_view->viewport()->installEventFilter(this);
    setCentralWidget(m_view);

    m_backgroundItem = m_scene->addPixmap(QPixmap());
    m_backgroundItem->setZValue(0);

    m_portraitItem = m_scene->addPixmap(QPixmap());
    m_portraitItem->setZValue(10);
    m_portraitItem->setVisible(false);

    QColor panel = QColorConstants::Black;
    panel.setAlpha(180);
    m_dialogBox = m_scene->addRect(QRectF(0, 540, 1280, 180), QPen(Qt::NoPen), QBrush(panel));
    m_dialogBox->setZValue(20);

    QFont nameFont;
    nameFont.setPointSize(14);
    nameFont.setBold(true);
    m_nameItem = m_scene->addText(QString(), nameFont);
    m_nameItem->setDefaultTextColor(Qt::white);
    m_nameItem->setPos(50, 550);
    m_nameItem->setZValue(21);

    QFont textFont;
    textFont.setPointSize(12);
    m_textItem = m_scene->addText(QString(), textFont);
    m_textItem->setDefaultTextColor(Qt::white);
    m_textItem->setTextWidth(1180);
    m_textItem->setPos(50, 580);
    m_textItem->setZValue(22);

    m_typewriter.setSpeed(20);
    connect(&m_typewriter, &TypewriterEffect::textUpdated, this, &PlayerWindow::onTextUpdated);
    connect(&m_typewriter, &TypewriterEffect::finished, this, &PlayerWindow::onTextFinished);

    m_saveButton = new QPushButton(QString::fromUtf8("\xE2\x9A\x99"), this);
    m_saveButton->setFixedSize(40, 40);
    m_saveButton->setCursor(Qt::PointingHandCursor);
    m_saveButton->setStyleSheet(
        "QPushButton {"
        "background: rgba(0,0,0,150);"
        "color: white;"
        "border: 1px solid #777;"
        "border-radius: 20px;"
        "font-size: 20px;"
        "font-weight: 700;"
        "padding: 0;"
        "}");
    connect(m_saveButton, &QPushButton::clicked, this, &PlayerWindow::onSettingsButtonClicked);

    m_autoPlayIndicator = new QLabel(">", this);
    m_autoPlayIndicator->setAlignment(Qt::AlignCenter);
    m_autoPlayIndicator->setFixedSize(34, 34);
    m_autoPlayIndicator->setStyleSheet(
        "QLabel {"
        "background: rgba(0, 0, 0, 165);"
        "color: #7CFC00;"
        "border: 1px solid rgba(255,255,255,130);"
        "border-radius: 17px;"
        "font-size: 20px;"
        "font-weight: 800;"
        "}");
    m_autoPlayIndicator->setVisible(false);

    updateSaveButtonGeometry();
    m_saveButton->raise();
    m_autoPlayIndicator->raise();

    m_voicePlayer = new QMediaPlayer(this);
    m_voiceOutput = new QAudioOutput(this);
    const double savedVoiceVolume = QSettings().value(kVoiceVolumeKey, 1.0).toDouble();
    m_voiceOutput->setVolume(qBound(0.0, savedVoiceVolume, 1.0));
    m_voicePlayer->setAudioOutput(m_voiceOutput);

    m_bgmPlayer = new QMediaPlayer(this);
    m_bgmOutput = new QAudioOutput(this);
    QSettings settings;
    const double savedBgmVolume = settings.value(kBgmVolumeKey, 0.55).toDouble();
    m_bgmOutput->setVolume(qBound(0.0, savedBgmVolume, 1.0));
    m_bgmPlayer->setAudioOutput(m_bgmOutput);
    m_bgmPlayer->setLoops(QMediaPlayer::Infinite);

    m_autoAdvanceTimer = new QTimer(this);
    m_autoAdvanceTimer->setSingleShot(true);
    connect(m_autoAdvanceTimer, &QTimer::timeout, this, [this]() {
        onAdvanceRequested();
    });
    applyDialogueTextStyle();
}

PlayerWindow::~PlayerWindow()
{
    delete m_extractTempDir;
}

bool PlayerWindow::loadGame(const QString &path, QString *errorMsg)
{
    m_packagePath = path;
    QString dir = path;
    if (path.endsWith(".galgame", Qt::CaseInsensitive)) {
        if (!extractPackageToTemp(path, &dir, errorMsg)) {
            return false;
        }
    }
    return loadFromDirectory(dir, errorMsg);
}

void PlayerWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_fullScreen || isFullScreen()) {
            m_fullScreen = false;
            showNormal();
        } else {
            close();
        }
        event->accept();
        return;
    }

    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && (event->modifiers() & Qt::AltModifier)) {
        toggleFullScreenMode();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onAdvanceRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_A) {
        m_autoPlayEnabled = !m_autoPlayEnabled;
        if (m_autoPlayIndicator) {
            m_autoPlayIndicator->setVisible(m_autoPlayEnabled);
        }
        if (!m_autoPlayEnabled && m_autoAdvanceTimer) {
            m_autoAdvanceTimer->stop();
        } else {
            tryScheduleAutoAdvance();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_S) {
        onSettingsButtonClicked();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F5) {
        saveQuickSlot();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F9) {
        loadQuickSlot();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void PlayerWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateSaveButtonGeometry();
    updateViewTransform();
}

void PlayerWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // 首次显示后再执行一次 fit，避免首帧窗口尺寸未稳定导致画面偏小。
    QTimer::singleShot(0, this, [this]() {
        updateViewTransform();
        if (!m_startMenuPending || m_startMenuShown || m_script.isEmpty()) {
            return;
        }
        m_startMenuShown = true;
        QString startError;
        if (!showStartMenu(&startError)) {
            close();
            return;
        }
        renderCurrentLine();
    });
}

bool PlayerWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::MouseButtonPress) {
        onAdvanceRequested();
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

void PlayerWindow::onAdvanceRequested()
{
    if (m_typewriter.isRunning()) {
        m_typewriter.skipToEnd();
        return;
    }

    if (m_currentIndex + 1 < m_script.size()) {
        ++m_currentIndex;
        renderCurrentLine();
    }
}

void PlayerWindow::onTextUpdated(const QString &text)
{
    m_textItem->setPlainText(text);
}

void PlayerWindow::onTextFinished()
{
    tryScheduleAutoAdvance();
}

void PlayerWindow::onSettingsButtonClicked()
{
    bool requestBackToStart = false;
    showSettingsDialog(&requestBackToStart);
    if (!requestBackToStart) {
        return;
    }

    enterStartScreenState();
    QString error;
    if (showStartMenu(&error)) {
        renderCurrentLine();
    }
}

void PlayerWindow::showSettingsDialog(bool *requestBackToStart)
{
    QDialog dialog(this);
    dialog.setWindowTitle("设置");
    dialog.setModal(true);
    dialog.resize(380, 280);

    auto *layout = new QVBoxLayout(&dialog);
    auto *toggleShortcut = new QShortcut(QKeySequence(Qt::Key_S), &dialog);
    connect(toggleShortcut, &QShortcut::activated, &dialog, &QDialog::reject);

    auto *bgmLabel = new QLabel("背景音乐音量", &dialog);
    layout->addWidget(bgmLabel);

    auto *bgmSlider = new QSlider(Qt::Horizontal, &dialog);
    bgmSlider->setRange(0, 100);
    const int currentVolume = m_bgmOutput ? qRound(m_bgmOutput->volume() * 100.0) : 55;
    bgmSlider->setValue(currentVolume);
    layout->addWidget(bgmSlider);

    auto *valueLabel = new QLabel(QString("%1%").arg(currentVolume), &dialog);
    valueLabel->setAlignment(Qt::AlignRight);
    layout->addWidget(valueLabel);

    connect(bgmSlider, &QSlider::valueChanged, &dialog, [this, valueLabel](int value) {
        if (m_bgmOutput) {
            m_bgmOutput->setVolume(value / 100.0);
        }
        QSettings().setValue(kBgmVolumeKey, value / 100.0);
        valueLabel->setText(QString("%1%").arg(value));
    });

    auto *voiceLabel = new QLabel("角色语音音量", &dialog);
    layout->addWidget(voiceLabel);
    auto *voiceSlider = new QSlider(Qt::Horizontal, &dialog);
    voiceSlider->setRange(0, 100);
    const int currentVoiceVolume = m_voiceOutput ? qRound(m_voiceOutput->volume() * 100.0) : 100;
    voiceSlider->setValue(currentVoiceVolume);
    layout->addWidget(voiceSlider);
    auto *voiceValueLabel = new QLabel(QString("%1%").arg(currentVoiceVolume), &dialog);
    voiceValueLabel->setAlignment(Qt::AlignRight);
    layout->addWidget(voiceValueLabel);
    connect(voiceSlider, &QSlider::valueChanged, &dialog, [this, voiceValueLabel](int value) {
        if (m_voiceOutput) {
            m_voiceOutput->setVolume(value / 100.0);
        }
        QSettings().setValue(kVoiceVolumeKey, value / 100.0);
        voiceValueLabel->setText(QString("%1%").arg(value));
    });

    auto *saveProgressBtn = new QPushButton("保存游戏进度", &dialog);
    connect(saveProgressBtn, &QPushButton::clicked, &dialog, [this]() {
        onSaveButtonClicked();
    });
    layout->addWidget(saveProgressBtn);

    auto *clearSavesBtn = new QPushButton("清空当前游戏存档", &dialog);
    connect(clearSavesBtn, &QPushButton::clicked, &dialog, [this]() {
        const auto ret = QMessageBox::question(this,
                                               "确认清空",
                                               "确定要清空当前游戏的 3 个存档位吗？此操作不可撤销。");
        if (ret != QMessageBox::Yes) {
            return;
        }
        QString error;
        if (!clearAllSaveSlots(&error)) {
            QMessageBox::warning(this, "清空失败", error);
            return;
        }
        QMessageBox::information(this, "完成", "当前游戏存档已清空。");
    });
    layout->addWidget(clearSavesBtn);

    auto *backToStartBtn = new QPushButton("回到开始界面", &dialog);
    connect(backToStartBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        dialog.setProperty("requestBackToStart", true);
        dialog.accept();
    });
    layout->addWidget(backToStartBtn);

    auto *closeBtn = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(closeBtn, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(closeBtn);

    dialog.exec();
    if (requestBackToStart) {
        *requestBackToStart = dialog.property("requestBackToStart").toBool();
    }
}

void PlayerWindow::onSaveButtonClicked()
{
    if (m_script.isEmpty()) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("保存进度");
    dialog.setModal(true);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("选择要保存到的存档位（最多 3 个）：", &dialog));

    int selectedSlot = 0;
    for (int slot = 1; slot <= kMaxSaveSlots; ++slot) {
        const SaveSlotMeta meta = readSaveSlotMeta(slot);
        QString label = QString("存档 %1").arg(slot);
        if (meta.exists) {
            label += QString("（覆盖：%1，第%2句）").arg(meta.timestamp).arg(meta.index + 1);
        } else {
            label += "（空）";
        }

        auto *btn = new QPushButton(label, &dialog);
        connect(btn, &QPushButton::clicked, &dialog, [&dialog, &selectedSlot, slot]() {
            selectedSlot = slot;
            dialog.accept();
        });
        layout->addWidget(btn);
    }

    auto *cancel = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    connect(cancel, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(cancel);

    if (dialog.exec() != QDialog::Accepted || selectedSlot <= 0) {
        return;
    }

    QString error;
    if (!saveToSlot(selectedSlot, &error)) {
        QMessageBox::warning(this, "保存失败", error);
        return;
    }
    QMessageBox::information(this, "保存成功", QString("已保存到存档 %1。").arg(selectedSlot));
}

bool PlayerWindow::loadFromDirectory(const QString &dirPath, QString *errorMsg)
{
    m_baseDir = QDir::cleanPath(dirPath);
    QFile file(QDir(m_baseDir).filePath("game.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMsg) {
            *errorMsg = "无法打开 game.json。";
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMsg) {
            *errorMsg = "game.json 解析失败。";
        }
        return false;
    }

    const QJsonObject root = doc.object();
    m_title = root.value("title").toString("Galgame");
    setWindowTitle(m_title);
    resize(root.value("width").toInt(1280), root.value("height").toInt(720));

    m_charDisplayNames.clear();
    m_charPortraitPaths.clear();
    const QJsonObject chars = root.value("assets").toObject().value("characters").toObject();
    for (auto it = chars.begin(); it != chars.end(); ++it) {
        m_charDisplayNames.insert(it.key(), it.key());
        m_charPortraitPaths.insert(it.key(), it.value().toString());
    }
    const QJsonObject charNames = root.value("characterNames").toObject();
    for (auto it = charNames.begin(); it != charNames.end(); ++it) {
        const QString id = it.key();
        const QString name = it.value().toString().trimmed();
        if (!id.isEmpty() && !name.isEmpty()) {
            m_charDisplayNames.insert(id, name);
        }
    }

    m_script.clear();
    const QJsonArray script = root.value("script").toArray();
    for (const QJsonValue &v : script) {
        const QJsonObject obj = v.toObject();
        RuntimeLine line;
        line.id = obj.value("id").toInt();
        line.characterId = obj.value("char").toString();
        const QString inlineName = obj.value("charName").toString().trimmed();
        if (!line.characterId.isEmpty() && !inlineName.isEmpty()) {
            m_charDisplayNames.insert(line.characterId, inlineName);
        }
        line.text = obj.value("text").toString();
        line.backgroundPath = obj.value("bgPath").toString();
        line.bgmPath = obj.value("bgmPath").toString();
        line.voicePath = obj.value("voicePath").toString();
        line.portraitScale = obj.value("portraitScale").toInt(100);
        line.nameFontSizeOverride = obj.value("nameFontSizeOverride").toInt(0);
        line.nameFontColorOverride = obj.value("nameFontColorOverride").toString();
        line.textFontSizeOverride = obj.value("textFontSizeOverride").toInt(0);
        line.textFontColorOverride = obj.value("textFontColorOverride").toString();
        m_script.append(line);
    }

    const QJsonObject startMenu = root.value("startMenu").toObject();
    m_startMenuBackgroundPath = startMenu.value("bgPath").toString();
    m_startMenuBgmPath = startMenu.value("bgmPath").toString();

    const QJsonObject uiStyle = root.value("uiStyle").toObject();
    const QJsonObject startStyle = uiStyle.value("startMenu").toObject();
    const QJsonObject dialogueStyle = uiStyle.value("dialogue").toObject();
    m_startMenuFontSize = qBound(8, startStyle.value("fontSize").toInt(22), 96);
    m_startMenuFontColor = normalizeColor(startStyle.value("fontColor").toString("#FFFFFF"), "#FFFFFF");
    m_dialogueNameFontSize = qBound(8, dialogueStyle.value("nameFontSize").toInt(14), 96);
    m_dialogueNameFontColor = normalizeColor(dialogueStyle.value("nameFontColor").toString("#FFFFFF"), "#FFFFFF");
    m_dialogueTextFontSize = qBound(8, dialogueStyle.value("textFontSize").toInt(12), 96);
    m_dialogueTextFontColor = normalizeColor(dialogueStyle.value("textFontColor").toString("#FFFFFF"), "#FFFFFF");
    applyDialogueTextStyle();

    if (m_script.isEmpty()) {
        if (errorMsg) {
            *errorMsg = "脚本为空。";
        }
        return false;
    }

    m_currentIndex = 0;
    m_startMenuPending = true;
    m_startMenuShown = false;
    return true;
}

bool PlayerWindow::extractPackageToTemp(const QString &packagePath, QString *outDir, QString *errorMsg)
{
    delete m_extractTempDir;
    m_extractTempDir = new QTemporaryDir();
    if (!m_extractTempDir->isValid()) {
        if (errorMsg) {
            *errorMsg = "无法创建临时解包目录。";
        }
        return false;
    }

    const QString outPath = m_extractTempDir->path();

#ifdef Q_OS_WIN
    const QString command = QString("Expand-Archive -Path \"%1\" -DestinationPath \"%2\" -Force")
                                .arg(QDir::toNativeSeparators(packagePath), QDir::toNativeSeparators(outPath));
    QProcess p;
    p.start("powershell", {"-NoProfile", "-Command", command});
    p.waitForFinished(-1);
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (errorMsg) {
            *errorMsg = "解包 .galgame 失败。";
        }
        return false;
    }
#else
    QProcess p;
    p.start("unzip", {"-o", packagePath, "-d", outPath});
    p.waitForFinished(-1);
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (errorMsg) {
            *errorMsg = "解包 .galgame 失败。";
        }
        return false;
    }
#endif

    *outDir = outPath;
    return true;
}

void PlayerWindow::renderCurrentLine()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_script.size()) {
        return;
    }
    if (m_autoAdvanceTimer) {
        m_autoAdvanceTimer->stop();
    }

    m_typewriter.cancel();
    m_textItem->setPlainText(QString());

    const RuntimeLine &line = m_script.at(m_currentIndex);
    const int nameSize = line.nameFontSizeOverride > 0 ? line.nameFontSizeOverride : m_dialogueNameFontSize;
    const int textSize = line.textFontSizeOverride > 0 ? line.textFontSizeOverride : m_dialogueTextFontSize;
    const QString nameColor = line.nameFontColorOverride.trimmed().isEmpty()
        ? m_dialogueNameFontColor
        : normalizeColor(line.nameFontColorOverride, m_dialogueNameFontColor);
    const QString textColor = line.textFontColorOverride.trimmed().isEmpty()
        ? m_dialogueTextFontColor
        : normalizeColor(line.textFontColorOverride, m_dialogueTextFontColor);
    if (m_nameItem) {
        QFont font = m_nameItem->font();
        font.setPointSize(nameSize);
        font.setBold(true);
        m_nameItem->setFont(font);
        m_nameItem->setDefaultTextColor(QColor(nameColor));
    }
    if (m_textItem) {
        QFont font = m_textItem->font();
        font.setPointSize(textSize);
        m_textItem->setFont(font);
        m_textItem->setDefaultTextColor(QColor(textColor));
    }
    m_nameItem->setPlainText(m_charDisplayNames.value(line.characterId, line.characterId));

    QPixmap bg(resolveAssetPath(line.backgroundPath));
    if (bg.isNull()) {
        bg = makeNotFound(1280, 720);
    }
    const QPixmap scaled = bg.scaled(1280, 720, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_backgroundItem->setPixmap(scaled);
    m_backgroundItem->setPos((1280 - scaled.width()) / 2.0, (720 - scaled.height()) / 2.0);

    const QString portraitRelPath = m_charPortraitPaths.value(line.characterId);
    QPixmap portrait(resolveAssetPath(portraitRelPath));
    if (portrait.isNull()) {
        m_portraitItem->setVisible(false);
    } else {
        const int clampedScale = qBound(10, line.portraitScale, 300);
        const int targetHeight = qMax(60, 600 * clampedScale / 100);
        const QPixmap portraitScaled = portrait.scaledToHeight(targetHeight, Qt::SmoothTransformation);
        m_portraitItem->setPixmap(portraitScaled);
        const qreal x = (1280 - portraitScaled.width()) / 2.0;
        const qreal y = 540.0 - portraitScaled.height();
        m_portraitItem->setPos(x, y);
        m_portraitItem->setVisible(true);
    }

    playLineAudio(line);
    m_typewriter.start(line.text);
    updateViewTransform();
}

void PlayerWindow::playLineAudio(const RuntimeLine &line)
{
    if (m_bgmPlayer) {
        const QString resolvedBgm = resolveAssetPath(line.bgmPath);
        if (resolvedBgm.isEmpty()) {
            if (!m_currentBgmPath.isEmpty()) {
                m_bgmPlayer->stop();
                m_currentBgmPath.clear();
            }
        } else if (resolvedBgm != m_currentBgmPath) {
            m_currentBgmPath = resolvedBgm;
            m_bgmPlayer->setSource(QUrl::fromLocalFile(resolvedBgm));
            m_bgmPlayer->play();
        } else if (m_bgmPlayer->playbackState() != QMediaPlayer::PlayingState) {
            m_bgmPlayer->play();
        }
    }

    if (m_voicePlayer) {
        const QString resolvedVoice = resolveAssetPath(line.voicePath);
        if (resolvedVoice.isEmpty()) {
            m_voicePlayer->stop();
        } else {
            m_voicePlayer->setSource(QUrl::fromLocalFile(resolvedVoice));
            m_voicePlayer->play();
        }
    }
}

bool PlayerWindow::showStartMenu(QString *errorMsg)
{
    if (m_saveButton) {
        m_saveButton->setVisible(false);
    }

    enterStartScreenState();
    const QString startBgAbs = resolveAssetPath(m_startMenuBackgroundPath);
    if (!startBgAbs.isEmpty()) {
        QPixmap bg(startBgAbs);
        if (!bg.isNull() && m_backgroundItem) {
            const QPixmap scaled = bg.scaled(1280, 720, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            m_backgroundItem->setPixmap(scaled);
            m_backgroundItem->setPos((1280 - scaled.width()) / 2.0, (720 - scaled.height()) / 2.0);
        }
    }
    const QString startBgmAbs = resolveAssetPath(m_startMenuBgmPath);
    if (!startBgmAbs.isEmpty() && m_bgmPlayer) {
        m_currentBgmPath = startBgmAbs;
        m_bgmPlayer->setSource(QUrl::fromLocalFile(startBgmAbs));
        m_bgmPlayer->play();
    }
    updateViewTransform();

    QDialog dialog(this);
    dialog.setWindowTitle(m_title.isEmpty() ? QString("开始游戏") : m_title);
    dialog.setModal(true);
    dialog.setGeometry(this->rect());
    dialog.setWindowFlag(Qt::FramelessWindowHint, true);
    dialog.setAttribute(Qt::WA_TranslucentBackground, true);
    dialog.setStyleSheet(
        QString("QLabel { color: %1; }"
        "QPushButton {"
        "background: rgba(255, 255, 255, 28);"
        "color: %1;"
        "border: 1px solid rgba(255,255,255,65);"
        "border-radius: 0px;"
        "padding: 8px 10px;"
        "font-size: %2px;"
        "font-weight: 800;"
        "}"
        "QPushButton:disabled {"
        "color: rgba(255,255,255,120);"
        "background: rgba(255,255,255,14);"
        "border-color: rgba(255,255,255,35);"
        "}"
        "QPushButton:hover:!disabled { background: rgba(255, 255, 255, 44); }")
            .arg(m_startMenuFontColor)
            .arg(m_startMenuFontSize));

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(0, 0, 0, 0);

    int selectedSlot = 0; // 0: Start, 1-3: Load slot, -1: Exit
    QVector<QPushButton *> slotButtons;

    auto *stack = new QStackedWidget(&dialog);
    layout->addWidget(stack);

    auto *mainPage = new QWidget(stack);
    auto *mainPageLayout = new QVBoxLayout(mainPage);
    mainPageLayout->setContentsMargins(24, 24, 24, 24);
    mainPageLayout->setSpacing(16);
    mainPageLayout->addStretch(1);
    auto *mainButtonsRow = new QHBoxLayout();
    mainButtonsRow->setContentsMargins(0, 0, 0, 0);
    mainButtonsRow->setSpacing(14);
    auto *startBtn = new QPushButton("Start", mainPage);
    auto *loadBtn = new QPushButton("Load", mainPage);
    auto *settingBtn = new QPushButton("Setting", mainPage);
    auto *exitBtn = new QPushButton("Exit", mainPage);
    const QList<QPushButton *> mainButtons = {startBtn, loadBtn, settingBtn, exitBtn};
    for (QPushButton *btn : mainButtons) {
        btn->setMinimumHeight(56);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        mainButtonsRow->addWidget(btn, 1);
    }
    mainPageLayout->addLayout(mainButtonsRow);
    mainPageLayout->addStretch(2);

    auto *loadPage = new QWidget(stack);
    auto *loadPageLayout = new QVBoxLayout(loadPage);
    loadPageLayout->setContentsMargins(16, 16, 16, 16);
    auto *loadTitle = new QLabel("Load Slots", loadPage);
    loadTitle->setAlignment(Qt::AlignCenter);
    loadPageLayout->addWidget(loadTitle);
    for (int slot = 1; slot <= kMaxSaveSlots; ++slot) {
        auto *btn = new QPushButton(loadPage);
        btn->setMinimumHeight(52);
        connect(btn, &QPushButton::clicked, &dialog, [&dialog, &selectedSlot, slot]() {
            selectedSlot = slot;
            dialog.accept();
        });
        slotButtons.append(btn);
        loadPageLayout->addWidget(btn);
    }
    auto *backBtn = new QPushButton("Back", loadPage);
    backBtn->setMinimumHeight(36);
    loadPageLayout->addWidget(backBtn);

    stack->addWidget(mainPage);
    stack->addWidget(loadPage);
    stack->setCurrentWidget(mainPage);

    const auto refreshSlotButtons = [this, &slotButtons]() {
        for (int slot = 1; slot <= kMaxSaveSlots; ++slot) {
            if (slot - 1 < 0 || slot - 1 >= slotButtons.size()) {
                continue;
            }
            QPushButton *btn = slotButtons.at(slot - 1);
            const SaveSlotMeta meta = readSaveSlotMeta(slot);
            if (meta.exists) {
                btn->setText(QString("读取存档 %1（%2，第%3句）\n%4")
                                 .arg(slot)
                                 .arg(meta.timestamp)
                                 .arg(meta.index + 1)
                                 .arg(meta.preview));
                btn->setEnabled(true);
            } else {
                btn->setText(QString("读取存档 %1（空）").arg(slot));
                btn->setEnabled(false);
            }
        }
    };

    connect(loadBtn, &QPushButton::clicked, &dialog, [&refreshSlotButtons, stack, loadPage]() {
        refreshSlotButtons();
        stack->setCurrentWidget(loadPage);
    });
    connect(backBtn, &QPushButton::clicked, &dialog, [stack, mainPage]() {
        stack->setCurrentWidget(mainPage);
    });
    connect(startBtn, &QPushButton::clicked, &dialog, [&dialog, &selectedSlot]() {
        selectedSlot = 0;
        dialog.accept();
    });
    connect(settingBtn, &QPushButton::clicked, &dialog, [this, &refreshSlotButtons]() {
        showSettingsDialog();
        refreshSlotButtons();
    });
    connect(exitBtn, &QPushButton::clicked, &dialog, [&dialog, &selectedSlot]() {
        selectedSlot = -1;
        dialog.accept();
    });
    refreshSlotButtons();

    if (dialog.exec() != QDialog::Accepted) {
        if (errorMsg) {
            *errorMsg = "已取消启动游戏。";
        }
        return false;
    }

    if (selectedSlot == -1) {
        close();
        return false;
    }
    if (selectedSlot > 0) {
        if (m_saveButton) {
            m_saveButton->setVisible(true);
        }
        return loadFromSlot(selectedSlot, errorMsg);
    }

    m_currentIndex = 0;
    if (m_saveButton) {
        m_saveButton->setVisible(true);
    }
    return true;
}

void PlayerWindow::enterStartScreenState()
{
    m_typewriter.cancel();
    if (m_voicePlayer) {
        m_voicePlayer->stop();
    }
    if (m_bgmPlayer) {
        m_bgmPlayer->stop();
    }
    m_currentBgmPath.clear();

    if (m_textItem) {
        m_textItem->setPlainText(QString());
    }
    if (m_nameItem) {
        m_nameItem->setPlainText(QString());
    }
    if (m_portraitItem) {
        m_portraitItem->setVisible(false);
        m_portraitItem->setPixmap(QPixmap());
    }
    if (m_backgroundItem) {
        m_backgroundItem->setPixmap(QPixmap());
        m_backgroundItem->setPos(0, 0);
    }
}

bool PlayerWindow::saveToSlot(int slot, QString *errorMsg)
{
    if (slot < 1 || slot > kMaxSaveSlots) {
        if (errorMsg) {
            *errorMsg = "无效的存档位。";
        }
        return false;
    }
    if (m_script.isEmpty() || m_currentIndex < 0 || m_currentIndex >= m_script.size()) {
        if (errorMsg) {
            *errorMsg = "当前没有可保存的进度。";
        }
        return false;
    }

    const QString saveDir = gameSaveDir();
    if (!QDir().mkpath(saveDir)) {
        if (errorMsg) {
            *errorMsg = "无法创建存档目录。";
        }
        return false;
    }

    const RuntimeLine &line = m_script.at(m_currentIndex);
    QJsonObject save;
    save["version"] = 1;
    save["slot"] = slot;
    save["index"] = m_currentIndex;
    save["time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    save["preview"] = simplifyPreviewText(line.text);

    QFile file(slotSavePath(slot));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "写入存档失败。";
        }
        return false;
    }
    file.write(QJsonDocument(save).toJson(QJsonDocument::Compact));
    return true;
}

bool PlayerWindow::loadFromSlot(int slot, QString *errorMsg)
{
    if (slot < 1 || slot > kMaxSaveSlots) {
        if (errorMsg) {
            *errorMsg = "无效的存档位。";
        }
        return false;
    }

    QFile file(slotSavePath(slot));
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMsg) {
            *errorMsg = QString("读取存档 %1 失败。").arg(slot);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMsg) {
            *errorMsg = QString("存档 %1 已损坏。").arg(slot);
        }
        return false;
    }

    const int idx = doc.object().value("index").toInt(0);
    m_currentIndex = qBound(0, idx, m_script.size() - 1);
    return true;
}

bool PlayerWindow::clearAllSaveSlots(QString *errorMsg)
{
    bool removedAny = false;
    for (int slot = 1; slot <= kMaxSaveSlots; ++slot) {
        const QString savePath = slotSavePath(slot);
        if (QFileInfo::exists(savePath)) {
            if (!QFile::remove(savePath)) {
                if (errorMsg) {
                    *errorMsg = QString("删除存档 %1 失败。").arg(slot);
                }
                return false;
            }
            removedAny = true;
        }
    }

    QDir saveDir(gameSaveDir());
    if (removedAny && saveDir.exists()) {
        saveDir.rmdir(".");
    }
    return true;
}

PlayerWindow::SaveSlotMeta PlayerWindow::readSaveSlotMeta(int slot) const
{
    SaveSlotMeta meta;
    QFile file(slotSavePath(slot));
    if (!file.open(QIODevice::ReadOnly)) {
        return meta;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return meta;
    }

    const QJsonObject obj = doc.object();
    meta.exists = true;
    meta.index = obj.value("index").toInt(0);
    meta.timestamp = obj.value("time").toString();
    meta.preview = obj.value("preview").toString();
    return meta;
}

QString PlayerWindow::gameSaveDir() const
{
    QString key = m_packagePath.trimmed();
    if (key.isEmpty()) {
        key = m_baseDir;
    }
    key = QDir::cleanPath(key).toLower();
    key.replace(':', '_');
    key.replace('/', '_');
    key.replace('\\', '_');

    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QString("saves/%1").arg(key));
}

QString PlayerWindow::slotSavePath(int slot) const
{
    return QDir(gameSaveDir()).filePath(QString("slot_%1.json").arg(slot));
}

void PlayerWindow::updateSaveButtonGeometry()
{
    if (!m_saveButton) {
        return;
    }
    const int margin = 14;
    m_saveButton->move(this->width() - m_saveButton->width() - margin, this->height() - m_saveButton->height() - margin);
    if (m_autoPlayIndicator) {
        m_autoPlayIndicator->move(this->width() - m_autoPlayIndicator->width() - margin, margin);
    }
}

void PlayerWindow::tryScheduleAutoAdvance()
{
    if (!m_autoPlayEnabled || !m_autoAdvanceTimer) {
        return;
    }
    if (m_typewriter.isRunning()) {
        return;
    }
    if (m_currentIndex + 1 >= m_script.size()) {
        m_autoAdvanceTimer->stop();
        return;
    }
    m_autoAdvanceTimer->start(2000);
}

void PlayerWindow::applyDialogueTextStyle()
{
    if (m_nameItem) {
        QFont font = m_nameItem->font();
        font.setPointSize(m_dialogueNameFontSize);
        font.setBold(true);
        m_nameItem->setFont(font);
        m_nameItem->setDefaultTextColor(QColor(m_dialogueNameFontColor));
    }
    if (m_textItem) {
        QFont font = m_textItem->font();
        font.setPointSize(m_dialogueTextFontSize);
        m_textItem->setFont(font);
        m_textItem->setDefaultTextColor(QColor(m_dialogueTextFontColor));
    }
}

void PlayerWindow::updateViewTransform()
{
    if (!m_view || !m_scene) {
        return;
    }
    // 先重置再 fit，避免历史缩放矩阵累积导致画面越来越小。
    m_view->resetTransform();
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void PlayerWindow::toggleFullScreenMode()
{
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen) {
        showFullScreen();
    } else {
        showNormal();
    }
}

void PlayerWindow::saveQuickSlot()
{
    QString error;
    saveToSlot(1, &error);
}

void PlayerWindow::loadQuickSlot()
{
    QString error;
    if (loadFromSlot(1, &error)) {
        renderCurrentLine();
    }
}

QString PlayerWindow::resolveAssetPath(const QString &assetRelPath) const
{
    if (assetRelPath.trimmed().isEmpty()) {
        return {};
    }
    if (QDir::isAbsolutePath(assetRelPath)) {
        return QDir::cleanPath(assetRelPath);
    }
    return QDir(m_baseDir).absoluteFilePath(assetRelPath);
}
