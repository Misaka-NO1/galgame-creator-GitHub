#include "exporter.h"

#include "../models/background.h"
#include "../models/character.h"
#include "../models/dialogue.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QOperatingSystemVersion>

namespace {

QString resolveProjectPath(const QString &projectDir, const QString &path)
{
    if (path.trimmed().isEmpty()) {
        return {};
    }

    if (QDir::isAbsolutePath(path)) {
        return QDir::cleanPath(path);
    }

    return QDir(projectDir).absoluteFilePath(path);
}

bool ensureParentDir(const QString &filePath)
{
    const QFileInfo info(filePath);
    QDir parent(info.absolutePath());
    return parent.exists() || parent.mkpath(".");
}

bool copyFileSafe(const QString &fromPath, const QString &toPath)
{
    if (!ensureParentDir(toPath)) {
        return false;
    }
    QFile::remove(toPath);
    return QFile::copy(fromPath, toPath);
}

bool copyDirectoryRecursively(const QString &sourceDir, const QString &targetDir)
{
    QDir src(sourceDir);
    if (!src.exists()) {
        return false;
    }

    QDir dst(targetDir);
    if (!dst.exists() && !dst.mkpath(".")) {
        return false;
    }

    const QFileInfoList entries = src.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    for (const QFileInfo &entry : entries) {
        const QString srcPath = entry.absoluteFilePath();
        const QString dstPath = dst.filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyDirectoryRecursively(srcPath, dstPath)) {
                return false;
            }
        } else {
            if (!copyFileSafe(srcPath, dstPath)) {
                return false;
            }
        }
    }
    return true;
}

bool runZipCommand(const QString &sourceDir, const QString &outputZipPath, QString *errorMsg)
{
#ifdef Q_OS_WIN
    const QString zipPath = QDir::toNativeSeparators(outputZipPath);
    const QString srcPattern = QDir::toNativeSeparators(QDir(sourceDir).absoluteFilePath("*"));
    const QString command = QString("Compress-Archive -Path \"%1\" -DestinationPath \"%2\" -Force").arg(srcPattern, zipPath);

    QProcess process;
    process.start("powershell", {"-NoProfile", "-Command", command});
    process.waitForFinished(-1);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMsg) {
            *errorMsg = QString::fromLocal8Bit(process.readAllStandardError());
        }
        return false;
    }
    return true;
#else
    QProcess process;
    process.setWorkingDirectory(sourceDir);
    process.start("zip", {"-r", outputZipPath, "."});
    process.waitForFinished(-1);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMsg) {
            *errorMsg = QString::fromLocal8Bit(process.readAllStandardError());
        }
        return false;
    }
    return true;
#endif
}

QString playerBinaryName()
{
#ifdef Q_OS_WIN
    return "galplayer.exe";
#else
    return "galplayer";
#endif
}

QString detectBuildConfigName(const QString &appDirPath)
{
    const QString lower = QFileInfo(appDirPath).fileName().toLower();
    if (lower == "debug") {
        return "Debug";
    }
    if (lower == "release") {
        return "Release";
    }
    if (lower == "relwithdebinfo") {
        return "RelWithDebInfo";
    }
    if (lower == "minsizerel") {
        return "MinSizeRel";
    }
    return {};
}

QString findNearestCMakeBuildDir(const QString &startPath)
{
    QDir dir(startPath);
    for (int i = 0; i < 8; ++i) {
        if (QFileInfo::exists(dir.filePath("CMakeCache.txt"))) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return {};
}

bool runBuildGalplayer(const QString &buildDir, const QString &config, QString *errorMsg)
{
    if (buildDir.isEmpty()) {
        return false;
    }

    QProcess process;
    QStringList args = {"--build", buildDir, "--target", "galplayer"};
    if (!config.isEmpty()) {
        args << "--config" << config;
    }
    process.start("cmake", args);
    process.waitForFinished(-1);
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        return true;
    }

    if (errorMsg) {
        *errorMsg = QString::fromLocal8Bit(process.readAllStandardError());
    }
    return false;
}

QString locateBuiltPlayerBinary(const QString &buildDir, const QString &config)
{
    const QString binary = playerBinaryName();
    const QString direct = QDir(buildDir).filePath(binary);
    if (QFileInfo::exists(direct)) {
        return direct;
    }

    if (!config.isEmpty()) {
        const QString inConfig = QDir(buildDir).filePath(QDir::cleanPath(config + "/" + binary));
        if (QFileInfo::exists(inConfig)) {
            return inConfig;
        }
    }
    return {};
}

bool ensureGalplayerBinaryReady(QString *outPlayerPath, QString *errorMsg)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString binary = playerBinaryName();

    const QString localBinary = QDir(appDir).filePath(binary);
    if (QFileInfo::exists(localBinary)) {
        *outPlayerPath = localBinary;
        return true;
    }

    const QString buildDir = findNearestCMakeBuildDir(appDir);
    if (buildDir.isEmpty()) {
        if (errorMsg) {
            *errorMsg = "未找到 CMake 构建目录，无法自动构建 galplayer。";
        }
        return false;
    }

    const QString config = detectBuildConfigName(appDir);
    QString buildError;
    if (!runBuildGalplayer(buildDir, config, &buildError)) {
        // 再尝试一次无 config 构建（兼容单配置生成器）。
        if (!runBuildGalplayer(buildDir, QString(), &buildError)) {
            if (errorMsg) {
                *errorMsg = QString("自动构建 galplayer 失败：%1").arg(buildError);
            }
            return false;
        }
    }

    QString builtPath = locateBuiltPlayerBinary(buildDir, config);
    if (builtPath.isEmpty()) {
        builtPath = locateBuiltPlayerBinary(buildDir, QString());
    }
    if (builtPath.isEmpty()) {
        if (QFileInfo::exists(localBinary)) {
            builtPath = localBinary;
        } else {
            if (errorMsg) {
                *errorMsg = "已执行自动构建，但未找到 galplayer 可执行文件。";
            }
            return false;
        }
    }

    *outPlayerPath = builtPath;
    return true;
}

bool copyPlayerRuntimeDependencies(const QString &rootDir, QString *errorMsg)
{
    const QString appDirPath = QCoreApplication::applicationDirPath();
    QDir appDir(appDirPath);

#ifdef Q_OS_WIN
    const QFileInfoList dlls = appDir.entryInfoList({"Qt6*.dll", "lib*.dll", "icu*.dll"}, QDir::Files);
    for (const QFileInfo &dll : dlls) {
        const QString dst = QDir(rootDir).filePath(dll.fileName());
        copyFileSafe(dll.absoluteFilePath(), dst);
    }
#endif

    const QStringList pluginDirs = {"platforms", "imageformats", "styles", "iconengines", "multimedia"};
    for (const QString &name : pluginDirs) {
        const QString src = appDir.filePath(name);
        if (!QDir(src).exists()) {
            continue;
        }
        const QString dst = QDir(rootDir).filePath(name);
        if (!copyDirectoryRecursively(src, dst)) {
            if (errorMsg) {
                *errorMsg = QString("复制运行时目录失败：%1").arg(name);
            }
            return false;
        }
    }
    return true;
}

bool writeLauncherScript(const QString &rootDir, QString *errorMsg)
{
#ifdef Q_OS_WIN
    const QString launcherPath = QDir(rootDir).filePath("启动游戏.bat");
    QFile launcher(launcherPath);
    if (!launcher.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "无法创建启动脚本。";
        }
        return false;
    }
    const QByteArray content =
        "@echo off\r\n"
        "setlocal\r\n"
        "set ROOT=%~dp0\r\n"
        "\"%ROOT%galplayer.exe\" \"%ROOT%\"\r\n";
    launcher.write(content);
    return true;
#else
    const QString launcherPath = QDir(rootDir).filePath("run_game.sh");
    QFile launcher(launcherPath);
    if (!launcher.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "无法创建启动脚本。";
        }
        return false;
    }
    const QByteArray content =
        "#!/usr/bin/env bash\n"
        "ROOT=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n"
        "\"$ROOT/galplayer\" \"$ROOT\"\n";
    launcher.write(content);
    launcher.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                            QFileDevice::ReadOther | QFileDevice::ExeOther);
    return true;
#endif
}

bool copyPlayerBinaryTo(const QString &rootDir, QString *errorMsg)
{
    QString playerPath;
    if (!ensureGalplayerBinaryReady(&playerPath, errorMsg)) {
        return false;
    }

    const QString dst = QDir(rootDir).filePath(playerBinaryName());
    if (!copyFileSafe(playerPath, dst)) {
        if (errorMsg) {
            *errorMsg = "复制播放器失败。";
        }
        return false;
    }

    if (!copyPlayerRuntimeDependencies(rootDir, errorMsg)) {
        return false;
    }
    return true;
}

} // namespace

bool Exporter::exportGame(const Project &project,
                          const QString &projectDir,
                          const ExportOptions &opts,
                          QString *errorMsg,
                          QProgressBar *progressBar)
{
    if (progressBar) {
        progressBar->setValue(0);
    }

    QString validationError;
    if (!validateProject(project, projectDir, &validationError)) {
        if (errorMsg) {
            *errorMsg = validationError;
        }
        return false;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        if (errorMsg) {
            *errorMsg = "无法创建临时目录。";
        }
        return false;
    }

    const QString tempRoot = tempDir.path();
    const QString playerTemplateDir = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("player_template");
    if (QDir(playerTemplateDir).exists()) {
        copyDirectoryRecursively(playerTemplateDir, tempRoot);
    }

    if (!copyPlayerBinaryTo(tempRoot, errorMsg)) {
        return false;
    }
    if (!writeLauncherScript(tempRoot, errorMsg)) {
        return false;
    }

    QJsonObject assetsObject;
    if (!copyResources(project, projectDir, tempRoot, &assetsObject, errorMsg, progressBar)) {
        return false;
    }

    if (!generateGameJson(project, opts, assetsObject, QDir(tempRoot).filePath("game.json"), errorMsg)) {
        return false;
    }

    if (progressBar) {
        progressBar->setValue(90);
    }
    if (!packageGame(tempRoot, opts.outputPath, opts.outputFolderOnly, errorMsg)) {
        return false;
    }

    if (progressBar) {
        progressBar->setValue(100);
    }
    return true;
}

bool Exporter::validateOnly(const Project &project, const QString &projectDir, QString *errorMsg)
{
    return validateProject(project, projectDir, errorMsg);
}

bool Exporter::validateProject(const Project &project, const QString &projectDir, QString *errorMsg)
{
    const QList<Dialogue *> dialogues = project.dialogues();
    const QList<Character *> characters = project.characters();

    if (dialogues.isEmpty()) {
        if (errorMsg) {
            *errorMsg = "项目没有对话内容。";
        }
        return false;
    }
    if (characters.isEmpty()) {
        if (errorMsg) {
            *errorMsg = "项目没有角色。";
        }
        return false;
    }

    for (Character *character : characters) {
        if (character->portraitPath().trimmed().isEmpty()) {
            if (errorMsg) {
                *errorMsg = QString("角色 %1 缺少立绘。").arg(character->name());
            }
            return false;
        }
        const QString portraitAbs = resolveProjectPath(projectDir, character->portraitPath());
        if (!QFileInfo::exists(portraitAbs)) {
            if (errorMsg) {
                *errorMsg = QString("角色 %1 的立绘文件不存在。").arg(character->name());
            }
            return false;
        }
    }

    for (Dialogue *dialogue : dialogues) {
        if (!project.getCharacter(dialogue->characterId())) {
            if (errorMsg) {
                *errorMsg = QString("对话 #%1 没有关联有效角色。").arg(dialogue->id());
            }
            return false;
        }
    }

    const int maxId = dialogues.size();
    for (int id = 1; id <= maxId; ++id) {
        if (!project.getBackgroundForDialogue(id)) {
            if (errorMsg) {
                *errorMsg = QString("对话 #%1 没有背景覆盖。").arg(id);
            }
            return false;
        }
    }

    return true;
}

bool Exporter::copyResources(const Project &project,
                             const QString &projectDir,
                             const QString &tempDir,
                             QJsonObject *assetsObject,
                             QString *errorMsg,
                             QProgressBar *progressBar)
{
    QDir root(tempDir);
    root.mkpath("assets/characters");
    root.mkpath("assets/backgrounds");
    root.mkpath("assets/voices");

    QJsonObject characterAssets;
    QJsonObject backgroundAssets;
    QJsonObject voiceAssets;

    const QList<Character *> characters = project.characters();
    const QList<Background *> backgrounds = project.backgrounds();
    const QList<Dialogue *> dialogues = project.dialogues();

    int total = characters.size() + backgrounds.size() + dialogues.size();
    if (total <= 0) {
        total = 1;
    }
    int done = 0;

    for (Character *character : characters) {
        const QString src = resolveProjectPath(projectDir, character->portraitPath());
        const QFileInfo srcInfo(src);
        const QString outName = QString("%1_%2").arg(character->id(), srcInfo.fileName());
        const QString rel = QDir::cleanPath(QString("assets/characters/%1").arg(outName));
        const QString dst = root.filePath(rel);
        if (!copyFileSafe(src, dst)) {
            if (errorMsg) {
                *errorMsg = QString("复制角色资源失败：%1").arg(src);
            }
            return false;
        }
        characterAssets.insert(character->id(), rel);
        ++done;
        if (progressBar) {
            progressBar->setValue(10 + (done * 70 / total));
        }
    }

    for (Background *background : backgrounds) {
        const QString src = resolveProjectPath(projectDir, background->imagePath());
        const QFileInfo srcInfo(src);
        const QString outName = QString("%1_%2").arg(background->id(), srcInfo.fileName());
        const QString rel = QDir::cleanPath(QString("assets/backgrounds/%1").arg(outName));
        const QString dst = root.filePath(rel);
        if (!copyFileSafe(src, dst)) {
            if (errorMsg) {
                *errorMsg = QString("复制背景资源失败：%1").arg(src);
            }
            return false;
        }
        backgroundAssets.insert(background->id(), rel);
        ++done;
        if (progressBar) {
            progressBar->setValue(10 + (done * 70 / total));
        }
    }

    for (Dialogue *dialogue : dialogues) {
        const QString voice = dialogue->voiceFile().trimmed();
        if (voice.isEmpty()) {
            ++done;
            if (progressBar) {
                progressBar->setValue(10 + (done * 70 / total));
            }
            continue;
        }

        const QString src = resolveProjectPath(projectDir, QDir::cleanPath(QString("resources/voices/%1").arg(voice)));
        if (!QFileInfo::exists(src)) {
            ++done;
            if (progressBar) {
                progressBar->setValue(10 + (done * 70 / total));
            }
            continue;
        }

        const QString rel = QDir::cleanPath(QString("assets/voices/%1").arg(QFileInfo(src).fileName()));
        const QString dst = root.filePath(rel);
        copyFileSafe(src, dst);
        voiceAssets.insert(voice, rel);
        ++done;
        if (progressBar) {
            progressBar->setValue(10 + (done * 70 / total));
        }
    }

    assetsObject->insert("characters", characterAssets);
    assetsObject->insert("backgrounds", backgroundAssets);
    assetsObject->insert("voices", voiceAssets);
    return true;
}

bool Exporter::generateGameJson(const Project &project,
                                const ExportOptions &opts,
                                const QJsonObject &assetsObject,
                                const QString &path,
                                QString *errorMsg)
{
    QJsonArray scriptArray;
    const QJsonObject backgroundAssets = assetsObject.value("backgrounds").toObject();
    const QJsonObject voiceAssets = assetsObject.value("voices").toObject();

    for (Dialogue *dialogue : project.dialogues()) {
        const Background *bg = project.getBackgroundForDialogue(dialogue->id());

        QJsonObject scriptLine;
        scriptLine["type"] = "dialogue";
        scriptLine["id"] = dialogue->id();
        scriptLine["char"] = dialogue->characterId();
        scriptLine["text"] = dialogue->text();
        scriptLine["bg"] = bg ? bg->id() : QString();
        scriptLine["bgPath"] = bg ? backgroundAssets.value(bg->id()).toString() : QString();
        scriptLine["voice"] = dialogue->voiceFile();
        scriptLine["voicePath"] = voiceAssets.value(dialogue->voiceFile()).toString();
        scriptLine["effect"] = dialogue->toJson().value("specialEffect").toString("none");
        const Character *character = project.getCharacter(dialogue->characterId());
        scriptLine["portraitScale"] = character ? character->portraitScale() : 100;
        scriptArray.append(scriptLine);
    }

    QJsonObject root;
    root["title"] = opts.gameTitle;
    root["width"] = opts.windowSize.width();
    root["height"] = opts.windowSize.height();
    root["assets"] = assetsObject;
    root["script"] = scriptArray;

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "写入 game.json 失败。";
        }
        return false;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool Exporter::packageGame(const QString &tempDir, const QString &outputPath, bool outputFolderOnly, QString *errorMsg)
{
    if (outputPath.trimmed().isEmpty()) {
        if (errorMsg) {
            *errorMsg = "导出路径为空。";
        }
        return false;
    }

    if (outputFolderOnly) {
        QDir outDir(outputPath);
        if (!outDir.exists() && !outDir.mkpath(".")) {
            if (errorMsg) {
                *errorMsg = "无法创建导出目录。";
            }
            return false;
        }
        return copyDirectoryRecursively(tempDir, outDir.absolutePath());
    }

    QString zipOut = outputPath;
    if (!zipOut.endsWith(".galgame", Qt::CaseInsensitive)) {
        zipOut += ".galgame";
    }

    const QString zipTemp = zipOut + ".zip";
    QFile::remove(zipTemp);
    QFile::remove(zipOut);

    if (!runZipCommand(tempDir, zipTemp, errorMsg)) {
        return false;
    }

    if (!QFile::rename(zipTemp, zipOut)) {
        QFile::remove(zipOut);
        if (!QFile::copy(zipTemp, zipOut)) {
            if (errorMsg) {
                *errorMsg = "生成 .galgame 文件失败。";
            }
            return false;
        }
        QFile::remove(zipTemp);
    }
    return true;
}

ExportDialog::ExportDialog(Project *project, const QString &projectDir, QWidget *parent)
    : QDialog(parent)
    , m_project(project)
    , m_projectDir(projectDir)
{
    setWindowTitle("导出游戏");
    resize(560, 280);

    m_outputPathEdit = new QLineEdit(this);
    auto *browseButton = new QPushButton("浏览...", this);
    auto *pathRow = new QHBoxLayout();
    pathRow->addWidget(m_outputPathEdit);
    pathRow->addWidget(browseButton);

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setText(project ? project->name() : QString("Galgame"));

    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->addItem("1280x720", QSize(1280, 720));
    m_resolutionCombo->addItem("1920x1080", QSize(1920, 1080));

    m_outputModeCombo = new QComboBox(this);
    m_outputModeCombo->addItem("导出.galgame包");
    m_outputModeCombo->addItem("导出文件夹(调试)");
    m_outputModeCombo->setCurrentIndex(1);

    auto *form = new QFormLayout();
    form->addRow("输出路径", pathRow);
    form->addRow("游戏标题", m_titleEdit);
    form->addRow("窗口尺寸", m_resolutionCombo);
    form->addRow("导出模式", m_outputModeCombo);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setText("等待导出");

    m_validateButton = new QPushButton("验证项目", this);
    m_exportButton = new QPushButton("开始导出", this);
    auto *closeButton = new QPushButton("关闭", this);
    auto *buttonRow = new QHBoxLayout();
    buttonRow->addWidget(m_validateButton);
    buttonRow->addWidget(m_exportButton);
    buttonRow->addStretch();
    buttonRow->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_statusLabel);
    layout->addLayout(buttonRow);

    connect(browseButton, &QPushButton::clicked, this, &ExportDialog::onBrowseOutputPath);
    connect(m_validateButton, &QPushButton::clicked, this, &ExportDialog::onValidateClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &ExportDialog::onExportClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
}

void ExportDialog::onBrowseOutputPath()
{
    if (m_outputModeCombo->currentIndex() == 1) {
        const QString dir = QFileDialog::getExistingDirectory(this, "选择导出目录");
        if (!dir.isEmpty()) {
            m_outputPathEdit->setText(dir);
        }
    } else {
        const QString file = QFileDialog::getSaveFileName(this, "选择输出文件", "game.galgame", "Galgame Package (*.galgame)");
        if (!file.isEmpty()) {
            m_outputPathEdit->setText(file);
        }
    }
}

void ExportDialog::onValidateClicked()
{
    if (!m_project) {
        QMessageBox::warning(this, "失败", "当前没有可导出的项目。");
        return;
    }

    Exporter exporter;
    QString error;
    if (!exporter.validateOnly(*m_project, m_projectDir, &error)) {
        QMessageBox::warning(this, "验证失败", error);
        m_statusLabel->setText("验证失败");
        return;
    }

    m_statusLabel->setText("验证通过");
    QMessageBox::information(this, "验证通过", "项目完整性检查通过。");
}

void ExportDialog::onExportClicked()
{
    if (!m_project) {
        QMessageBox::warning(this, "失败", "当前没有可导出的项目。");
        return;
    }

    const ExportOptions opts = buildOptions();
    if (opts.outputPath.trimmed().isEmpty()) {
        QMessageBox::warning(this, "失败", "请先选择输出路径。");
        return;
    }

    Exporter exporter;
    QString error;
    m_statusLabel->setText("正在导出...");
    qApp->processEvents();

    if (!exporter.exportGame(*m_project, m_projectDir, opts, &error, m_progressBar)) {
        m_statusLabel->setText("导出失败");
        QMessageBox::warning(this, "导出失败", error);
        return;
    }

    m_statusLabel->setText("导出完成");
    QMessageBox::information(this, "完成", "导出成功。");
}

ExportOptions ExportDialog::buildOptions() const
{
    ExportOptions opts;
    opts.outputPath = m_outputPathEdit->text().trimmed();
    opts.gameTitle = m_titleEdit->text().trimmed();
    if (opts.gameTitle.isEmpty() && m_project) {
        opts.gameTitle = m_project->name();
    }
    opts.windowSize = m_resolutionCombo->currentData().toSize();
    opts.outputFolderOnly = (m_outputModeCombo->currentIndex() == 1);
    return opts;
}
