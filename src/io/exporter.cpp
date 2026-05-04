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
#include <QLibraryInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QPushButton>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QOperatingSystemVersion>

namespace {
constexpr const char *kNarratorCharacterId = "__narrator__";

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

QString resolveExportAudioSourcePath(const QString &projectDir, const QString &rawPath, const QString &resourceSubdir)
{
    const QString cleaned = QDir::fromNativeSeparators(rawPath.trimmed());
    if (cleaned.isEmpty()) {
        return {};
    }

    const QString direct = resolveProjectPath(projectDir, cleaned);
    if (QFileInfo::exists(direct)) {
        return direct;
    }

    const QFileInfo inputInfo(cleaned);
    const QString fileNameOnly = inputInfo.fileName();
    if (fileNameOnly.isEmpty()) {
        return {};
    }

    const QString inResourceDir = QDir(projectDir).absoluteFilePath(
        QDir::cleanPath(QString("resources/%1/%2").arg(resourceSubdir, fileNameOnly)));
    if (QFileInfo::exists(inResourceDir)) {
        return inResourceDir;
    }

    return {};
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

QString findWinDeployQtExecutable()
{
#ifdef Q_OS_WIN
    const QString exeName = "windeployqt.exe";

    const QString qtBinPath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    const QString qtBinCandidate = QDir(qtBinPath).filePath(exeName);
    if (QFileInfo::exists(qtBinCandidate)) {
        return qtBinCandidate;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString appDirCandidate = QDir(appDir).filePath(exeName);
    if (QFileInfo::exists(appDirCandidate)) {
        return appDirCandidate;
    }

    const QString pathEnv = QProcessEnvironment::systemEnvironment().value("PATH");
    const QStringList paths = pathEnv.split(';', Qt::SkipEmptyParts);
    for (const QString &pathItem : paths) {
        const QString candidate = QDir(pathItem).filePath(exeName);
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
#endif
    return {};
}

bool deployWithWinDeployQt(const QString &playerPath, const QString &rootDir, QString *errorMsg)
{
#ifdef Q_OS_WIN
    const QString windeployqt = findWinDeployQtExecutable();
    if (windeployqt.isEmpty()) {
        if (errorMsg) {
            *errorMsg = "未找到 windeployqt.exe。";
        }
        return false;
    }

    QProcess process;
    process.setWorkingDirectory(rootDir);
    process.start(windeployqt, {"--dir", rootDir, "--force", playerPath});
    process.waitForFinished(-1);
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        return true;
    }

    if (errorMsg) {
        QString detail = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        if (detail.isEmpty()) {
            detail = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        }
        if (detail.isEmpty()) {
            detail = "未知错误";
        }
        *errorMsg = QString("windeployqt 执行失败：%1").arg(detail);
    }
    return false;
#else
    Q_UNUSED(playerPath)
    Q_UNUSED(rootDir)
    Q_UNUSED(errorMsg)
    return true;
#endif
}

bool copyPlayerRuntimeDependenciesFallback(const QString &rootDir, const QString &playerPath, QString *errorMsg)
{
    const QString appDirPath = QCoreApplication::applicationDirPath();
    const QString playerDirPath = QFileInfo(playerPath).absolutePath();
    const QString qtBinPath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    const QString qtPluginsPath = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    QDir appDir(appDirPath);
    QDir playerDir(playerDirPath);
    QDir qtBinDir(qtBinPath);
    QDir qtPluginsDir(qtPluginsPath);

#ifdef Q_OS_WIN
    bool copiedAnyDependency = false;
    auto copyDllByPatterns = [&](const QDir &srcDir, const QStringList &patterns) {
        if (!srcDir.exists()) {
            return;
        }
        const QFileInfoList dlls = srcDir.entryInfoList(patterns, QDir::Files);
        for (const QFileInfo &dll : dlls) {
            const QString dst = QDir(rootDir).filePath(dll.fileName());
            if (copyFileSafe(dll.absoluteFilePath(), dst)) {
                copiedAnyDependency = true;
            }
        }
    };

    // 优先从播放器目录与编辑器目录带出依赖，再从 Qt 安装目录补齐。
    copyDllByPatterns(playerDir, {"Qt6*.dll", "lib*.dll", "icu*.dll", "av*.dll", "sw*.dll"});
    copyDllByPatterns(appDir, {"Qt6*.dll", "lib*.dll", "icu*.dll", "av*.dll", "sw*.dll"});
    copyDllByPatterns(qtBinDir, {"Qt6*.dll", "lib*.dll", "icu*.dll", "av*.dll", "sw*.dll"});

    const QStringList runtimeDllNames = {
        "libgcc_s_seh-1.dll",
        "libstdc++-6.dll",
        "libwinpthread-1.dll",
        "libssl-3-x64.dll",
        "libcrypto-3-x64.dll",
        "libssl-1_1-x64.dll",
        "libcrypto-1_1-x64.dll"
    };
    for (const QString &dllName : runtimeDllNames) {
        const QString srcFromPlayer = playerDir.filePath(dllName);
        const QString srcFromApp = appDir.filePath(dllName);
        const QString srcFromQtBin = qtBinDir.filePath(dllName);
        const QString dst = QDir(rootDir).filePath(dllName);
        if (QFileInfo::exists(srcFromPlayer)) {
            if (copyFileSafe(srcFromPlayer, dst)) {
                copiedAnyDependency = true;
            }
        } else if (QFileInfo::exists(srcFromApp)) {
            if (copyFileSafe(srcFromApp, dst)) {
                copiedAnyDependency = true;
            }
        } else if (QFileInfo::exists(srcFromQtBin)) {
            if (copyFileSafe(srcFromQtBin, dst)) {
                copiedAnyDependency = true;
            }
        }
    }
#endif

    const QStringList pluginDirs = {"platforms", "imageformats", "styles", "iconengines", "multimedia", "tls", "networkinformation", "audio"};
    for (const QString &name : pluginDirs) {
        QString src;
        if (QDir(playerDir.filePath(name)).exists()) {
            src = playerDir.filePath(name);
        } else if (QDir(appDir.filePath(name)).exists()) {
            src = appDir.filePath(name);
        } else if (QDir(qtPluginsDir.filePath(name)).exists()) {
            src = qtPluginsDir.filePath(name);
        }
        if (src.isEmpty()) {
            continue;
        }
        const QString dst = QDir(rootDir).filePath(name);
        if (!copyDirectoryRecursively(src, dst)) {
            if (errorMsg) {
                *errorMsg = QString("复制运行时目录失败：%1").arg(name);
            }
            return false;
        }
#ifdef Q_OS_WIN
        copiedAnyDependency = true;
#endif
    }
#ifdef Q_OS_WIN
    if (!copiedAnyDependency) {
        if (errorMsg) {
            *errorMsg = "未找到可复制的 Qt 运行时依赖。";
        }
        return false;
    }
#endif
    return true;
}

bool copyPlayerRuntimeDependencies(const QString &rootDir, const QString &playerPath, QString *errorMsg)
{
#ifdef Q_OS_WIN
    QString deployError;
    const bool windeployqtSuccess = deployWithWinDeployQt(playerPath, rootDir, &deployError);

    QString fallbackError;
    const bool fallbackSuccess = copyPlayerRuntimeDependenciesFallback(rootDir, playerPath, &fallbackError);

    if (windeployqtSuccess || fallbackSuccess) {
        return true;
    }
    if (errorMsg) {
        *errorMsg = QString("复制运行时依赖失败。windeployqt: %1；fallback: %2")
                        .arg(deployError.isEmpty() ? "未知错误" : deployError,
                             fallbackError.isEmpty() ? "未知错误" : fallbackError);
    }
    return false;
#else
    return copyPlayerRuntimeDependenciesFallback(rootDir, playerPath, errorMsg);
#endif
}

bool writeLauncherScript(const QString &rootDir, QString *errorMsg)
{
#ifdef Q_OS_WIN
    const QString launcherBatPath = QDir(rootDir).filePath("启动游戏.bat");
    QFile launcherBat(launcherBatPath);
    if (!launcherBat.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "无法创建启动脚本。";
        }
        return false;
    }
    const QByteArray batContent =
        "@echo off\r\n"
        "setlocal\r\n"
        "set \"ROOT=%~dp0\"\r\n"
        "set \"ROOT=%ROOT:~0,-1%\"\r\n"
        "powershell -NoProfile -ExecutionPolicy Bypass -Command \"$ErrorActionPreference='SilentlyContinue'; Unblock-File -LiteralPath '%ROOT%\\galplayer.exe'; Get-ChildItem -LiteralPath '%ROOT%' -File -Include *.dll,*.exe | ForEach-Object { Unblock-File -LiteralPath $_.FullName }\" >nul 2>nul\r\n"
        "start \"\" \"%ROOT%\\galplayer.exe\" \"%ROOT%\"\r\n";
    launcherBat.write(batContent);
    launcherBat.close();

    // 额外生成英文文件名启动器，避免部分系统脚本环境下中文路径识别失败。
    const QString launcherAsciiBatPath = QDir(rootDir).filePath("run_game.bat");
    QFile launcherAsciiBat(launcherAsciiBatPath);
    if (!launcherAsciiBat.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "无法创建英文启动脚本。";
        }
        return false;
    }
    launcherAsciiBat.write(batContent);
    launcherAsciiBat.close();

    // 默认无终端启动器：双击只保留游戏界面。
    const QString launcherVbsPath = QDir(rootDir).filePath("启动游戏.vbs");
    QFile launcherVbs(launcherVbsPath);
    if (!launcherVbs.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "无法创建无终端启动脚本。";
        }
        return false;
    }
    const QByteArray vbsContent =
        "Set fso = CreateObject(\"Scripting.FileSystemObject\")\r\n"
        "Set shell = CreateObject(\"WScript.Shell\")\r\n"
        "root = fso.GetParentFolderName(WScript.ScriptFullName)\r\n"
        "bat = root & \"\\\\run_game.bat\"\r\n"
        "If Not fso.FileExists(bat) Then\r\n"
        "  MsgBox \"launcher not found: \" & bat, 16, \"Start Failed\"\r\n"
        "  WScript.Quit 1\r\n"
        "End If\r\n"
        "shell.CurrentDirectory = root\r\n"
        "shell.Run Chr(34) & bat & Chr(34), 0, False\r\n";
    launcherVbs.write(vbsContent);
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

    if (!copyPlayerRuntimeDependencies(rootDir, playerPath, errorMsg)) {
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
        const QString characterId = dialogue->characterId().trimmed();
        const bool isNarrator = characterId == QString::fromLatin1(kNarratorCharacterId);
        if (!isNarrator && !project.getCharacter(characterId)) {
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
    root.mkpath("assets/bgm");

    QJsonObject characterAssets;
    QJsonObject backgroundAssets;
    QJsonObject voiceAssets;
    QJsonObject bgmAssets;

    const QList<Character *> characters = project.characters();
    const QList<Background *> backgrounds = project.backgrounds();
    const QList<BgmTrack *> bgmTracks = project.bgmTracks();
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

    for (BgmTrack *track : bgmTracks) {
        const QString bgm = track->bgmPath().trimmed();
        if (bgm.isEmpty()) {
            continue;
        }
        const QString bgmSrc = resolveExportAudioSourcePath(projectDir, bgm, "bgm");
        if (!QFileInfo::exists(bgmSrc)) {
            if (errorMsg) {
                *errorMsg = QString("背景音乐文件不存在：%1").arg(bgm);
            }
            return false;
        }
        const QString key = track->id().trimmed().isEmpty() ? QString("%1_%2_%3").arg(track->backgroundId()).arg(track->startDialogueId()).arg(track->endDialogueId()) : track->id();
        const QString bgmRel = QDir::cleanPath(QString("assets/bgm/%1_%2")
                                                   .arg(key, QFileInfo(bgmSrc).fileName()));
        const QString bgmDst = root.filePath(bgmRel);
        if (!copyFileSafe(bgmSrc, bgmDst)) {
            if (errorMsg) {
                *errorMsg = QString("复制背景音乐资源失败：%1").arg(bgmSrc);
            }
            return false;
        }
        bgmAssets.insert(key, bgmRel);
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

        const QString src = resolveExportAudioSourcePath(projectDir, voice, "voices");
        if (!QFileInfo::exists(src)) {
            if (errorMsg) {
                *errorMsg = QString("语音文件不存在：%1").arg(voice);
            }
            return false;
        }

        const QString rel = QDir::cleanPath(QString("assets/voices/%1").arg(QFileInfo(src).fileName()));
        const QString dst = root.filePath(rel);
        if (!copyFileSafe(src, dst)) {
            if (errorMsg) {
                *errorMsg = QString("复制语音资源失败：%1").arg(src);
            }
            return false;
        }
        voiceAssets.insert(voice, rel);
        ++done;
        if (progressBar) {
            progressBar->setValue(10 + (done * 70 / total));
        }
    }

    assetsObject->insert("characters", characterAssets);
    assetsObject->insert("backgrounds", backgroundAssets);
    assetsObject->insert("voices", voiceAssets);
    assetsObject->insert("bgm", bgmAssets);

    QJsonObject startMenuAssets;
    const QString startBg = project.startMenuBackgroundPath().trimmed();
    if (!startBg.isEmpty()) {
        const QString src = resolveProjectPath(projectDir, startBg);
        if (QFileInfo::exists(src)) {
            const QString rel = QDir::cleanPath(QString("assets/ui/start_%1").arg(QFileInfo(src).fileName()));
            const QString dst = root.filePath(rel);
            if (!copyFileSafe(src, dst)) {
                if (errorMsg) {
                    *errorMsg = QString("复制开始界面背景失败：%1").arg(src);
                }
                return false;
            }
            startMenuAssets.insert("bgPath", rel);
        }
    }

    const QString startBgm = project.startMenuBgmPath().trimmed();
    if (!startBgm.isEmpty()) {
        const QString src = resolveProjectPath(projectDir, startBgm);
        if (QFileInfo::exists(src)) {
            const QString rel = QDir::cleanPath(QString("assets/bgm/start_%1").arg(QFileInfo(src).fileName()));
            const QString dst = root.filePath(rel);
            if (!copyFileSafe(src, dst)) {
                if (errorMsg) {
                    *errorMsg = QString("复制开始界面音乐失败：%1").arg(src);
                }
                return false;
            }
            startMenuAssets.insert("bgmPath", rel);
        }
    }
    assetsObject->insert("startMenu", startMenuAssets);
    return true;
}

bool Exporter::generateGameJson(const Project &project,
                                const ExportOptions &opts,
                                const QJsonObject &assetsObject,
                                const QString &path,
                                QString *errorMsg)
{
    QJsonArray scriptArray;
    QJsonObject characterNameMap;
    const QJsonObject backgroundAssets = assetsObject.value("backgrounds").toObject();
    const QJsonObject voiceAssets = assetsObject.value("voices").toObject();
    const QJsonObject bgmAssets = assetsObject.value("bgm").toObject();

    for (Dialogue *dialogue : project.dialogues()) {
        const Background *bg = project.getBackgroundForDialogue(dialogue->id());
        const BgmTrack *bgmTrack = bg ? project.getBgmTrackForDialogue(dialogue->id(), bg->id()) : nullptr;
        const Character *character = project.getCharacter(dialogue->characterId());

        QJsonObject scriptLine;
        scriptLine["type"] = "dialogue";
        scriptLine["id"] = dialogue->id();
        scriptLine["char"] = dialogue->characterId();
        const bool isNarrator = dialogue->characterId() == QString::fromLatin1(kNarratorCharacterId);
        scriptLine["charName"] = isNarrator ? QString() : (character ? character->name() : dialogue->characterId());
        scriptLine["text"] = dialogue->text();
        scriptLine["bg"] = bg ? bg->id() : QString();
        scriptLine["bgPath"] = bg ? backgroundAssets.value(bg->id()).toString() : QString();
        QString bgmKey;
        if (bgmTrack) {
            bgmKey = bgmTrack->id().trimmed().isEmpty() ? QString("%1_%2_%3").arg(bgmTrack->backgroundId()).arg(bgmTrack->startDialogueId()).arg(bgmTrack->endDialogueId()) : bgmTrack->id();
        }
        scriptLine["bgmPath"] = bgmTrack ? bgmAssets.value(bgmKey).toString() : QString();
        scriptLine["voice"] = dialogue->voiceFile();
        scriptLine["voicePath"] = voiceAssets.value(dialogue->voiceFile()).toString();
        scriptLine["effect"] = dialogue->toJson().value("specialEffect").toString("none");
        scriptLine["portraitScale"] = (!isNarrator && character) ? character->portraitScale() : 100;
        scriptLine["portraitX"] = (!isNarrator && character) ? character->portraitX() : 320;
        scriptLine["portraitY"] = (!isNarrator && character) ? character->portraitY() : -60;
        scriptLine["nameFontSizeOverride"] = dialogue->nameFontSizeOverride();
        scriptLine["nameFontColorOverride"] = dialogue->nameFontColorOverride();
        scriptLine["textFontSizeOverride"] = dialogue->textFontSizeOverride();
        scriptLine["textFontColorOverride"] = dialogue->textFontColorOverride();
        scriptArray.append(scriptLine);
        if (character) {
            characterNameMap.insert(character->id(), character->name());
        }
    }

    QJsonObject root;
    root["title"] = opts.gameTitle;
    root["width"] = opts.windowSize.width();
    root["height"] = opts.windowSize.height();
    root["assets"] = assetsObject;
    root["script"] = scriptArray;
    root["characterNames"] = characterNameMap;
    root["startMenu"] = assetsObject.value("startMenu").toObject();
    QJsonObject uiStyle;
    QJsonObject startStyle;
    startStyle["fontSize"] = project.startMenuFontSize();
    startStyle["fontColor"] = project.startMenuFontColor();
    startStyle["autoPlayIndicatorColor"] = project.autoPlayIndicatorColor();
    startStyle["settingsButtonColor"] = project.settingsButtonColor();
    uiStyle["startMenu"] = startStyle;

    QJsonObject dialogueStyle;
    dialogueStyle["nameFontSize"] = project.dialogueNameFontSize();
    dialogueStyle["nameFontColor"] = project.dialogueNameFontColor();
    dialogueStyle["textFontSize"] = project.dialogueTextFontSize();
    dialogueStyle["textFontColor"] = project.dialogueTextFontColor();
    dialogueStyle["boxColor"] = project.dialogueBoxColor();
    uiStyle["dialogue"] = dialogueStyle;
    root["uiStyle"] = uiStyle;

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

    m_outputModeCombo = new QComboBox(this);
    m_outputModeCombo->addItem("导出.galgame包");
    m_outputModeCombo->addItem("导出文件夹(调试)");
    m_outputModeCombo->setCurrentIndex(1);

    auto *form = new QFormLayout();
    form->addRow("输出路径", pathRow);
    form->addRow("游戏标题", m_titleEdit);
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
#ifdef Q_OS_WIN
    QMessageBox::information(this, "完成", "导出成功。\n请双击“启动游戏.vbs”启动（无终端窗口）。");
#else
    QMessageBox::information(this, "完成", "导出成功。");
#endif
}

ExportOptions ExportDialog::buildOptions() const
{
    ExportOptions opts;
    opts.outputPath = m_outputPathEdit->text().trimmed();
    opts.gameTitle = m_titleEdit->text().trimmed();
    if (opts.gameTitle.isEmpty() && m_project) {
        opts.gameTitle = m_project->name();
    }
    opts.windowSize = QSize(1920, 1080);
    opts.outputFolderOnly = (m_outputModeCombo->currentIndex() == 1);
    return opts;
}
