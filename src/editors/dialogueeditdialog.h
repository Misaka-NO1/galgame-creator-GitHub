#ifndef DIALOGUEEDITDIALOG_H
#define DIALOGUEEDITDIALOG_H

#include "../models/dialogue.h"
#include "../models/project.h"

#include <QDialog>

class QComboBox;
class QTextEdit;
class QLineEdit;

class DialogueEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DialogueEditDialog(Dialogue *dialogue, Project *project, QWidget *parent = nullptr);

private slots:
    void applyAndAccept();

private:
    Dialogue *m_dialogue = nullptr;
    Project *m_project = nullptr;

    QComboBox *m_characterCombo = nullptr;
    QTextEdit *m_textEdit = nullptr;
    QLineEdit *m_voiceFileEdit = nullptr;
    QComboBox *m_effectCombo = nullptr;
};

#endif // DIALOGUEEDITDIALOG_H
