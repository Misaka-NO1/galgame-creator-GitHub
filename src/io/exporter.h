#ifndef EXPORTER_H
#define EXPORTER_H

#include "../models/project.h"

#include <QDialog>
#include <QSize>

class QLineEdit;
class QComboBox;
class QProgressBar;
class QPushButton;
class QLabel;

struct ExportOptions
{
    QString outputPath;
    QString gameTitle;
    QSize windowSize = QSize(1920, 1080);
    bool outputFolderOnly = false;
};

class Exporter
{
public:
    bool exportGame(const Project &project,
                    const QString &projectDir,
                    const ExportOptions &opts,
                    QString *errorMsg = nullptr,
                    QProgressBar *progressBar = nullptr);

    bool validateOnly(const Project &project, const QString &projectDir, QString *errorMsg = nullptr);

private:
    bool validateProject(const Project &project, const QString &projectDir, QString *errorMsg);
    bool copyResources(const Project &project,
                       const QString &projectDir,
                       const QString &tempDir,
                       QJsonObject *assetsObject,
                       QString *errorMsg,
                       QProgressBar *progressBar);
    bool generateGameJson(const Project &project,
                          const ExportOptions &opts,
                          const QJsonObject &assetsObject,
                          const QString &path,
                          QString *errorMsg);
    bool packageGame(const QString &tempDir, const QString &outputPath, bool outputFolderOnly, QString *errorMsg);
};

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(Project *project, const QString &projectDir, QWidget *parent = nullptr);

private slots:
    void onBrowseOutputPath();
    void onValidateClicked();
    void onExportClicked();

private:
    ExportOptions buildOptions() const;

    Project *m_project = nullptr;
    QString m_projectDir;

    QLineEdit *m_outputPathEdit = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QComboBox *m_outputModeCombo = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_validateButton = nullptr;
    QPushButton *m_exportButton = nullptr;
};

#endif // EXPORTER_H
