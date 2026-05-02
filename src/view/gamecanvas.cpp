#include "gamecanvas.h"

#include "../models/background.h"
#include "../models/character.h"
#include "../models/dialogue.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QDir>
#include <QVector>

namespace {

QPixmap createNotFoundPixmap(int width, int height)
{
    QPixmap placeholder(width, height);
    placeholder.fill(QColorConstants::Black);

    QPainter painter(&placeholder);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(QColorConstants::Red);

    QFont font;
    font.setPointSize(20);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, width, height), Qt::AlignCenter, QStringLiteral("Image Not Found"));

    return placeholder;
}

QString resolveImagePath(const QString &path, const QString &baseDir)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    if (QDir::isAbsolutePath(trimmed)) {
        return QDir::cleanPath(trimmed);
    }

    if (baseDir.trimmed().isEmpty()) {
        return QDir::cleanPath(trimmed);
    }

    return QDir(baseDir).absoluteFilePath(trimmed);
}

} // namespace

GameCanvas::GameCanvas(QWidget *parent)
    : QGraphicsView(parent)
    , m_typewriter(this)
{
    m_scene.setSceneRect(0, 0, kVirtualWidth, kVirtualHeight);

    setScene(&m_scene);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setBackgroundBrush(Qt::black);

    m_backgroundItem = m_scene.addPixmap(QPixmap());
    m_backgroundItem->setZValue(0.0);

    m_portraitItem = m_scene.addPixmap(QPixmap());
    m_portraitItem->setZValue(10.0);
    m_portraitItem->setVisible(false);

    setupUiLayer();

    m_autoPlayTimer.setSingleShot(true);
    connect(&m_autoPlayTimer, &QTimer::timeout, this, &GameCanvas::onAutoPlayTimeout);

    connect(&m_typewriter, &TypewriterEffect::textUpdated, this, &GameCanvas::onTypewriterTextUpdated);
    connect(&m_typewriter, &TypewriterEffect::finished, this, &GameCanvas::onTypewriterFinished);

    updateViewTransform();
}

void GameCanvas::setDialogue(const Dialogue &d, const Character &speaker, const Background &bg)
{
    m_autoPlayTimer.stop();
    m_typewriter.cancel();
    m_speakerNameItem->setPlainText(QString());
    m_dialogueTextItem->setPlainText(QString());

    hideCharacterPortrait();
    setBackgroundImage(bg.imagePath());

    if (!speaker.portraitPath().trimmed().isEmpty()) {
        showCharacterPortrait(speaker.portraitPath(), speaker.position(), speaker.portraitScale());
    }

    const int nameSize = d.nameFontSizeOverride() > 0 ? d.nameFontSizeOverride() : m_globalNameFontSize;
    const int textSize = d.textFontSizeOverride() > 0 ? d.textFontSizeOverride() : m_globalTextFontSize;
    const QString nameColor = d.nameFontColorOverride().trimmed().isEmpty() ? m_globalNameFontColor : d.nameFontColorOverride().trimmed();
    const QString textColor = d.textFontColorOverride().trimmed().isEmpty() ? m_globalTextFontColor : d.textFontColorOverride().trimmed();
    if (m_speakerNameItem) {
        QFont nameFont = m_speakerNameItem->font();
        nameFont.setPointSize(qBound(8, nameSize, 96));
        nameFont.setBold(true);
        m_speakerNameItem->setFont(nameFont);
        const QColor color(nameColor);
        m_speakerNameItem->setDefaultTextColor(color.isValid() ? color : QColorConstants::White);
    }
    if (m_dialogueTextItem) {
        QFont textFont = m_dialogueTextItem->font();
        textFont.setPointSize(qBound(8, textSize, 96));
        m_dialogueTextItem->setFont(textFont);
        const QColor color(textColor);
        m_dialogueTextItem->setDefaultTextColor(color.isValid() ? color : QColorConstants::White);
    }
    showText(speaker.name(), d.text());
}

void GameCanvas::clearScene()
{
    m_autoPlayTimer.stop();
    m_typewriter.cancel();
    m_backgroundItem->setPixmap(QPixmap());
    hideCharacterPortrait();
    m_speakerNameItem->setPlainText(QString());
    m_dialogueTextItem->setPlainText(QString());
    m_fullText.clear();
}

void GameCanvas::showCharacterPortrait(const QString &imagePath, Character::Position pos, int scalePercent)
{
    const QString resolvedPath = resolveImagePath(imagePath, m_resourceBaseDir);
    QPixmap portrait(resolvedPath);
    if (portrait.isNull()) {
        portrait = createNotFoundPixmap(420, 600);
    }

    const int clampedScale = qBound(10, scalePercent, 300);
    const int targetHeight = qMax(60, 600 * clampedScale / 100);
    const QPixmap scaled = portrait.scaledToHeight(targetHeight, Qt::SmoothTransformation);
    m_portraitItem->setPixmap(scaled);

    qreal x = 100.0;
    if (pos == Character::Position::Center) {
        x = (kVirtualWidth - scaled.width()) / 2.0;
    } else if (pos == Character::Position::Right) {
        x = kVirtualWidth - scaled.width() - 100.0;
    }

    const qreal y = 540.0 - scaled.height();
    m_portraitItem->setPos(x, y);
    m_portraitItem->setVisible(true);
}

void GameCanvas::hideCharacterPortrait()
{
    m_portraitItem->setVisible(false);
}

void GameCanvas::showText(const QString &speakerName, const QString &text)
{
    const QString normalizedSpeaker = speakerName.trimmed();
    m_speakerNameItem->setPlainText(normalizedSpeaker);
    m_speakerNameItem->setVisible(!normalizedSpeaker.isEmpty());
    m_fullText = text;
    m_dialogueTextItem->setPlainText(QString());
    updateTextLayout();
    m_typewriter.start(text);
}

void GameCanvas::setTypingSpeed(int msPerChar)
{
    m_typingSpeedMs = qMax(0, msPerChar);
    m_typewriter.setSpeed(m_typingSpeedMs);
}

void GameCanvas::setClickToAdvance(bool enabled)
{
    m_clickToAdvance = enabled;
}

void GameCanvas::setAutoPlay(bool enabled, int msDelay)
{
    m_autoPlayEnabled = enabled;
    m_autoPlayDelayMs = qMax(0, msDelay);

    if (!m_autoPlayEnabled) {
        m_autoPlayTimer.stop();
        return;
    }

    if (!m_typewriter.isRunning() && !m_fullText.isEmpty()) {
        m_autoPlayTimer.start(m_autoPlayDelayMs);
    }
}

void GameCanvas::setResourceBaseDir(const QString &baseDir)
{
    m_resourceBaseDir = QDir::cleanPath(baseDir);
}

void GameCanvas::setDialogueBoxColor(const QString &color)
{
    if (!m_dialogueBoxItem) {
        return;
    }
    QColor dialogueBoxColor(color.trimmed());
    if (!dialogueBoxColor.isValid()) {
        dialogueBoxColor = QColorConstants::Black;
    }
    dialogueBoxColor.setAlpha(180);
    m_dialogueBoxItem->setBrush(QBrush(dialogueBoxColor));
}

void GameCanvas::setGlobalDialogueTextStyle(int nameFontSize,
                                            const QString &nameFontColor,
                                            int textFontSize,
                                            const QString &textFontColor)
{
    m_globalNameFontSize = qBound(8, nameFontSize, 96);
    m_globalTextFontSize = qBound(8, textFontSize, 96);
    m_globalNameFontColor = nameFontColor.trimmed();
    m_globalTextFontColor = textFontColor.trimmed();
    if (m_globalNameFontColor.isEmpty()) {
        m_globalNameFontColor = "#FFFFFF";
    }
    if (m_globalTextFontColor.isEmpty()) {
        m_globalTextFontColor = "#FFFFFF";
    }
}

void GameCanvas::mousePressEvent(QMouseEvent *e)
{
    QGraphicsView::mousePressEvent(e);
    if (e->button() == Qt::LeftButton) {
        handleAdvanceInput();
    }
}

void GameCanvas::keyPressEvent(QKeyEvent *e)
{
    const int key = e->key();
    if (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter) {
        handleAdvanceInput();
        e->accept();
        return;
    }
    QGraphicsView::keyPressEvent(e);
}

void GameCanvas::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);
    updateViewTransform();
}

void GameCanvas::onTypewriterTextUpdated(const QString &text)
{
    m_dialogueTextItem->setPlainText(text);
    updateTextLayout();
}

void GameCanvas::onTypewriterFinished()
{
    emit textDisplayFinished();

    if (m_autoPlayEnabled) {
        m_autoPlayTimer.start(m_autoPlayDelayMs);
    }
}

void GameCanvas::onAutoPlayTimeout()
{
    if (m_typewriter.isRunning()) {
        return;
    }
    emit advanceRequested();
}

void GameCanvas::setupUiLayer()
{
    QColor dialogueBoxColor = QColorConstants::Black;
    dialogueBoxColor.setAlpha(180);

    m_dialogueBoxItem = m_scene.addRect(QRectF(0.0, 540.0, 1280.0, 180.0), QPen(Qt::NoPen), QBrush(dialogueBoxColor));
    m_dialogueBoxItem->setZValue(20.0);

    QFont speakerFont;
    speakerFont.setPointSize(14);
    speakerFont.setBold(true);

    m_speakerNameItem = m_scene.addText(QString(), speakerFont);
    m_speakerNameItem->setDefaultTextColor(Qt::white);
    m_speakerNameItem->setPos(50.0, 550.0);
    m_speakerNameItem->setZValue(21.0);

    QFont textFont;
    textFont.setPointSize(12);
    textFont.setBold(false);

    m_dialogueTextItem = m_scene.addText(QString(), textFont);
    m_dialogueTextItem->setDefaultTextColor(Qt::white);
    m_dialogueTextItem->setTextWidth(1180.0);
    m_dialogueTextItem->setPos(50.0, 580.0);
    m_dialogueTextItem->setZValue(22.0);
}

void GameCanvas::setBackgroundImage(const QString &imagePath)
{
    const QString resolvedPath = resolveImagePath(imagePath, m_resourceBaseDir);
    QPixmap raw(resolvedPath);
    if (raw.isNull()) {
        raw = createNotFoundPixmap(kVirtualWidth, kVirtualHeight);
    }

    const QPixmap scaled = raw.scaled(kVirtualWidth, kVirtualHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_backgroundItem->setPixmap(scaled);
    m_backgroundItem->setPos((kVirtualWidth - scaled.width()) / 2.0, (kVirtualHeight - scaled.height()) / 2.0);
}

void GameCanvas::updateTextLayout()
{
    // 与运行时播放器保持一致：对白固定起始位置，不随文本高度上移。
    m_dialogueTextItem->setPos(50.0, 580.0);
}

void GameCanvas::updateViewTransform()
{
    fitInView(m_scene.sceneRect(), Qt::KeepAspectRatio);
}

void GameCanvas::handleAdvanceInput()
{
    if (!m_clickToAdvance) {
        return;
    }

    if (m_typewriter.isRunning()) {
        m_typewriter.skipToEnd();
        return;
    }

    m_autoPlayTimer.stop();
    emit advanceRequested();
}

#ifdef CANVAS_TEST
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMainWindow w;
    auto *canvas = new GameCanvas(&w);
    w.setCentralWidget(canvas);
    w.resize(1280, 720);

    // 测试：加载背景与立绘，并显示测试文本；点击推进切换数据。
    struct CanvasTestData {
        QString backgroundPath;
        QString portraitPath;
        QString speakerName;
        QString text;
        Character::Position position;
    };

    const QVector<CanvasTestData> testData = {
        {QStringLiteral("D:/galgame_test/background_1.jpg"),
         QStringLiteral("D:/galgame_test/portrait_1.png"),
         QStringLiteral("测试角色A"),
         QStringLiteral("这是第一段测试文本，点击或按空格继续。"),
         Character::Position::Left},
        {QStringLiteral("D:/galgame_test/background_2.jpg"),
         QStringLiteral("D:/galgame_test/portrait_2.png"),
         QStringLiteral("测试角色B"),
         QStringLiteral("这是第二段测试文本，验证打字机和立绘位置变化。"),
         Character::Position::Center},
        {QStringLiteral("D:/galgame_test/background_3.jpg"),
         QStringLiteral("D:/galgame_test/portrait_3.png"),
         QStringLiteral("测试角色C"),
         QStringLiteral("这是第三段测试文本，验证advanceRequested信号触发。"),
         Character::Position::Right},
    };

    canvas->setTypingSpeed(24);
    canvas->setClickToAdvance(true);

    int index = 0;
    auto renderAt = [&]() {
        const CanvasTestData &entry = testData.at(index % testData.size());

        Dialogue dialogue;
        dialogue.setId(index + 1);
        dialogue.setCharacterId(QStringLiteral("test_character"));
        dialogue.setText(entry.text);
        dialogue.setVoiceFile(QStringLiteral("test.wav"));
        dialogue.setSpecialEffect(Dialogue::SpecialEffect::None);

        Character speaker;
        speaker.setId(QStringLiteral("test_character"));
        speaker.setName(entry.speakerName);
        speaker.setPortraitPath(entry.portraitPath);
        speaker.setAvatarPath(QStringLiteral(""));
        speaker.setVoicePrefix(QStringLiteral("test_"));
        speaker.setPosition(entry.position);

        Background background;
        background.setId(QStringLiteral("test_bg"));
        background.setImagePath(entry.backgroundPath);
        background.setStartDialogueId(1);
        background.setEndDialogueId(9999);
        background.setBgmPath(QStringLiteral(""));

        canvas->setDialogue(dialogue, speaker, background);
    };

    QObject::connect(canvas, &GameCanvas::advanceRequested, canvas, [&]() {
        ++index;
        renderAt();
    });

    renderAt();
    w.show();
    return a.exec();
}
#endif
