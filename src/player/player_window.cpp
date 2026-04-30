#include "player_window.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

namespace {

constexpr int kMaxSaveSlots = 3;

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

    m_saveButton = new QPushButton("保存进度", this);
    m_saveButton->setFixedHeight(34);
    m_saveButton->setCursor(Qt::PointingHandCursor);
    m_saveButton->setStyleSheet("QPushButton { background: rgba(0,0,0,150); color: white; border: 1px solid #777; border-radius: 6px; padding: 0 12px; }");
    connect(m_saveButton, &QPushButton::clicked, this, &PlayerWindow::onSaveButtonClicked);
    updateSaveButtonGeometry();
    m_saveButton->raise();
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

    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onAdvanceRequested();
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
    if (event->key() == Qt::Key_Return && (event->modifiers() & Qt::AltModifier)) {
        toggleFullScreenMode();
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

    m_script.clear();
    const QJsonArray script = root.value("script").toArray();
    for (const QJsonValue &v : script) {
        const QJsonObject obj = v.toObject();
        RuntimeLine line;
        line.id = obj.value("id").toInt();
        line.characterId = obj.value("char").toString();
        line.text = obj.value("text").toString();
        line.backgroundPath = obj.value("bgPath").toString();
        line.voicePath = obj.value("voicePath").toString();
        line.portraitScale = obj.value("portraitScale").toInt(100);
        m_script.append(line);
    }

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

    m_typewriter.cancel();
    m_textItem->setPlainText(QString());

    const RuntimeLine &line = m_script.at(m_currentIndex);
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

    m_typewriter.start(line.text);
    updateViewTransform();
}

bool PlayerWindow::showStartMenu(QString *errorMsg)
{
    QDialog dialog(this);
    dialog.setWindowTitle(m_title.isEmpty() ? QString("开始游戏") : m_title);
    dialog.setModal(true);
    dialog.resize(520, 320);

    auto *layout = new QVBoxLayout(&dialog);
    auto *titleLabel = new QLabel("请选择开始方式", &dialog);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size:18px;font-weight:600;");
    layout->addWidget(titleLabel);

    int selectedSlot = 0; // 0: 新游戏，1-3: 对应存档位

    auto *newGameBtn = new QPushButton("新开始游戏", &dialog);
    newGameBtn->setMinimumHeight(42);
    connect(newGameBtn, &QPushButton::clicked, &dialog, [&dialog, &selectedSlot]() {
        selectedSlot = 0;
        dialog.accept();
    });
    layout->addWidget(newGameBtn);

    for (int slot = 1; slot <= kMaxSaveSlots; ++slot) {
        const SaveSlotMeta meta = readSaveSlotMeta(slot);
        QString label;
        if (meta.exists) {
            label = QString("读取存档 %1（%2，第%3句）\n%4")
                        .arg(slot)
                        .arg(meta.timestamp)
                        .arg(meta.index + 1)
                        .arg(meta.preview);
        } else {
            label = QString("读取存档 %1（空）").arg(slot);
        }

        auto *btn = new QPushButton(label, &dialog);
        btn->setMinimumHeight(52);
        btn->setEnabled(meta.exists);
        connect(btn, &QPushButton::clicked, &dialog, [&dialog, &selectedSlot, slot]() {
            selectedSlot = slot;
            dialog.accept();
        });
        layout->addWidget(btn);
    }

    auto *cancelButtons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    connect(cancelButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(cancelButtons);

    if (dialog.exec() != QDialog::Accepted) {
        if (errorMsg) {
            *errorMsg = "已取消启动游戏。";
        }
        return false;
    }

    if (selectedSlot > 0) {
        return loadFromSlot(selectedSlot, errorMsg);
    }

    m_currentIndex = 0;
    return true;
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
    const int buttonWidth = 112;
    m_saveButton->setFixedWidth(buttonWidth);
    m_saveButton->move(this->width() - buttonWidth - margin, margin);
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
