#include "dialogueeditdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLineEdit>

namespace {
constexpr const char *kNarratorCharacterId = "__narrator__";

QString effectToText(Dialogue::SpecialEffect effect)
{
    switch (effect) {
    case Dialogue::SpecialEffect::None:
        return "none";
    case Dialogue::SpecialEffect::Shake:
        return "shake";
    case Dialogue::SpecialEffect::Flash:
        return "flash";
    case Dialogue::SpecialEffect::Fade:
        return "fade";
    }
    return "none";
}

Dialogue::SpecialEffect textToEffect(const QString &text)
{
    if (text == "shake") {
        return Dialogue::SpecialEffect::Shake;
    }
    if (text == "flash") {
        return Dialogue::SpecialEffect::Flash;
    }
    if (text == "fade") {
        return Dialogue::SpecialEffect::Fade;
    }
    return Dialogue::SpecialEffect::None;
}

} // namespace

DialogueEditDialog::DialogueEditDialog(Dialogue *dialogue, Project *project, QWidget *parent)
    : QDialog(parent)
    , m_dialogue(dialogue)
    , m_project(project)
{
    setWindowTitle("编辑对话");
    resize(480, 320);

    m_characterCombo = new QComboBox(this);
    m_textEdit = new QTextEdit(this);
    m_voiceFileEdit = new QLineEdit(this);
    m_effectCombo = new QComboBox(this);
    m_effectCombo->addItems({"none", "shake", "flash", "fade"});

    if (m_project) {
        m_characterCombo->addItem(QStringLiteral("旁白"), QString::fromLatin1(kNarratorCharacterId));
        const QList<Character *> characters = m_project->characters();
        for (Character *character : characters) {
            m_characterCombo->addItem(character->name(), character->id());
        }
    }

    if (m_dialogue) {
        const int index = m_characterCombo->findData(m_dialogue->characterId());
        if (index >= 0) {
            m_characterCombo->setCurrentIndex(index);
        }
        m_textEdit->setPlainText(m_dialogue->text());
        m_voiceFileEdit->setText(m_dialogue->voiceFile());
        m_effectCombo->setCurrentText(effectToText(m_dialogue->specialEffect()));
    }

    auto *form = new QFormLayout();
    form->addRow("角色", m_characterCombo);
    form->addRow("文本", m_textEdit);
    form->addRow("语音文件", m_voiceFileEdit);
    form->addRow("特效", m_effectCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &DialogueEditDialog::applyAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &DialogueEditDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void DialogueEditDialog::applyAndAccept()
{
    if (!m_dialogue) {
        accept();
        return;
    }

    if (m_characterCombo->currentIndex() < 0) {
        QMessageBox::information(this, "提示", "请先选择角色。");
        return;
    }

    if (m_textEdit->toPlainText().trimmed().isEmpty()) {
        QMessageBox::information(this, "提示", "对话文本不能为空。");
        return;
    }

    m_dialogue->setCharacterId(m_characterCombo->currentData().toString());
    m_dialogue->setText(m_textEdit->toPlainText());
    m_dialogue->setVoiceFile(m_voiceFileEdit->text().trimmed());
    m_dialogue->setSpecialEffect(textToEffect(m_effectCombo->currentText()));
    accept();
}
