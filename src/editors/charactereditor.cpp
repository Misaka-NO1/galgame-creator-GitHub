#include "charactereditor.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString positionToText(Character::Position position)
{
    switch (position) {
    case Character::Position::Left:
        return "左";
    case Character::Position::Center:
        return "中";
    case Character::Position::Right:
        return "右";
    }
    return "中";
}

Character::Position textToPosition(const QString &text)
{
    if (text == "左") {
        return Character::Position::Left;
    }
    if (text == "右") {
        return Character::Position::Right;
    }
    return Character::Position::Center;
}

} // namespace

CharacterEditor::CharacterEditor(Character *character, const QString &projectPath, QWidget *parent)
    : QDialog(parent)
    , m_character(character)
    , m_projectPath(projectPath)
{
    setWindowTitle("角色编辑");
    resize(420, 420);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(220, 300);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background:#222;color:#fff;border:1px solid #444;");

    m_nameEdit = new QLineEdit(this);
    m_portraitPathEdit = new QLineEdit(this);
    m_voicePrefixEdit = new QLineEdit(this);
    m_positionCombo = new QComboBox(this);
    m_positionCombo->addItems({"左", "中", "右"});
    m_browseButton = new QPushButton("浏览...", this);

    auto *portraitRow = new QHBoxLayout();
    portraitRow->addWidget(m_portraitPathEdit);
    portraitRow->addWidget(m_browseButton);

    auto *formLayout = new QFormLayout();
    formLayout->addRow("名称", m_nameEdit);
    formLayout->addRow("立绘路径", portraitRow);
    formLayout->addRow("位置", m_positionCombo);
    formLayout->addRow("语音前缀", m_voicePrefixEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_previewLabel, 0, Qt::AlignHCenter);
    layout->addLayout(formLayout);
    layout->addWidget(buttons);

    if (m_character) {
        m_nameEdit->setText(m_character->name());
        m_portraitPathEdit->setText(m_character->portraitPath());
        m_positionCombo->setCurrentText(positionToText(m_character->position()));
        m_voicePrefixEdit->setText(m_character->voicePrefix());
        updatePreview(m_character->portraitPath());
    }

    connect(m_browseButton, &QPushButton::clicked, this, &CharacterEditor::browsePortrait);
    connect(buttons, &QDialogButtonBox::accepted, this, &CharacterEditor::applyAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &CharacterEditor::reject);
}

void CharacterEditor::browsePortrait()
{
    const QString sourcePath = QFileDialog::getOpenFileName(this, "选择立绘", QString(), "Images (*.png *.jpg *.jpeg *.webp)");
    if (sourcePath.isEmpty()) {
        return;
    }

    const QString savedPath = savePortraitToProject(sourcePath);
    if (savedPath.isEmpty()) {
        QMessageBox::warning(this, "保存失败", "无法复制立绘到项目资源目录。");
        return;
    }

    m_portraitPathEdit->setText(savedPath);
    updatePreview(savedPath);
}

void CharacterEditor::applyAndAccept()
{
    if (!m_character) {
        accept();
        return;
    }

    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::information(this, "提示", "角色名称不能为空。");
        return;
    }

    m_character->setName(m_nameEdit->text().trimmed());
    m_character->setPortraitPath(m_portraitPathEdit->text().trimmed());
    m_character->setPosition(textToPosition(m_positionCombo->currentText()));
    m_character->setVoicePrefix(m_voicePrefixEdit->text().trimmed());
    accept();
}

void CharacterEditor::updatePreview(const QString &path)
{
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        m_previewLabel->setText("无预览");
        m_previewLabel->setPixmap(QPixmap());
        return;
    }

    m_previewLabel->setText(QString());
    m_previewLabel->setPixmap(pixmap.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QString CharacterEditor::savePortraitToProject(const QString &sourcePath) const
{
    if (!m_character || sourcePath.isEmpty()) {
        return {};
    }

    const QString projectRoot = QDir::cleanPath(m_projectPath);
    QDir rootDir(projectRoot);
    if (!rootDir.exists() && !rootDir.mkpath(".")) {
        return {};
    }

    if (!rootDir.mkpath("resources/characters")) {
        return {};
    }

    const QFileInfo sourceInfo(sourcePath);
    const QString ext = sourceInfo.suffix().isEmpty() ? "png" : sourceInfo.suffix();
    const QString fileName = QString("char_%1.%2").arg(m_character->id(), ext);
    const QString absoluteTarget = rootDir.filePath(QDir::cleanPath(QString("resources/characters/%1").arg(fileName)));

    QFile::remove(absoluteTarget);
    if (!QFile::copy(sourcePath, absoluteTarget)) {
        return {};
    }

    return QDir::cleanPath(QString("resources/characters/%1").arg(fileName));
}
