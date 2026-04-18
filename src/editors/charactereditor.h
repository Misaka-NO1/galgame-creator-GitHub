#ifndef CHARACTEREDITOR_H
#define CHARACTEREDITOR_H

#include "../models/character.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QSpinBox;

class CharacterEditor : public QDialog
{
    Q_OBJECT

public:
    explicit CharacterEditor(Character *character, const QString &projectPath, QWidget *parent = nullptr);

private slots:
    void browsePortrait();
    void applyAndAccept();

private:
    void updatePreview(const QString &path);
    QString savePortraitToProject(const QString &sourcePath) const;

    Character *m_character = nullptr;
    QString m_projectPath;

    QLabel *m_previewLabel = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_portraitPathEdit = nullptr;
    QComboBox *m_positionCombo = nullptr;
    QSpinBox *m_scaleSpin = nullptr;
    QLineEdit *m_voicePrefixEdit = nullptr;
    QPushButton *m_browseButton = nullptr;
};

#endif // CHARACTEREDITOR_H
