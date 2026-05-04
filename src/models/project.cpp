#include "project.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace {

QString resolveProjectJsonPath(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        return {};
    }

    if (path.endsWith(".json", Qt::CaseInsensitive)) {
        return path;
    }

    return QDir::cleanPath(QDir(path).filePath("project.json"));
}

QString extractDirectoryFromFilePath(const QString &filePath)
{
    const QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(filePath));
    const int slashIndex = normalized.lastIndexOf('/');
    if (slashIndex < 0) {
        return QDir::currentPath();
    }
    return normalized.left(slashIndex);
}

bool ensureResourceDirectories(const QString &projectFilePath)
{
    const QString basePath = extractDirectoryFromFilePath(projectFilePath);
    QDir baseDir(basePath);

    if (!baseDir.exists() && !baseDir.mkpath(".")) {
        return false;
    }

    return baseDir.mkpath("resources/characters")
        && baseDir.mkpath("resources/backgrounds")
        && baseDir.mkpath("resources/voices")
        && baseDir.mkpath("resources/bgm");
}

bool hasContinuousDialogueIds(const QList<Dialogue *> &dialogues)
{
    if (dialogues.isEmpty()) {
        return true;
    }

    QList<int> ids;
    ids.reserve(dialogues.size());
    for (const Dialogue *dialogue : dialogues) {
        ids.append(dialogue->id());
    }

    std::sort(ids.begin(), ids.end());
    if (ids.first() != 1) {
        return false;
    }

    for (int i = 1; i < ids.size(); ++i) {
        if (ids[i] != ids[i - 1] + 1) {
            return false;
        }
    }
    return true;
}

QJsonObject fontToJson(const QFont &font)
{
    QJsonObject json;
    json["family"] = font.family();
    json["pointSize"] = font.pointSizeF();
    json["bold"] = font.bold();
    json["italic"] = font.italic();
    return json;
}

QFont fontFromJson(const QJsonValue &value)
{
    if (!value.isObject()) {
        return {};
    }

    const QJsonObject json = value.toObject();
    QFont font(json.value("family").toString());
    font.setPointSizeF(json.value("pointSize").toDouble(font.pointSizeF()));
    font.setBold(json.value("bold").toBool(false));
    font.setItalic(json.value("italic").toBool(false));
    return font;
}

} // namespace

Project::Project(QObject *parent)
    : QObject(parent)
{
}

Project::~Project()
{
    clear();
}

QString Project::name() const
{
    return m_name;
}

void Project::setName(const QString &name)
{
    m_name = name;
}

QString Project::version() const
{
    return m_version;
}

void Project::setVersion(const QString &version)
{
    m_version = version;
}

QFont Project::defaultFont() const
{
    return m_defaultFont;
}

void Project::setDefaultFont(const QFont &defaultFont)
{
    m_defaultFont = defaultFont;
}

QString Project::startMenuBackgroundPath() const
{
    return m_startMenuBackgroundPath;
}

void Project::setStartMenuBackgroundPath(const QString &path)
{
    m_startMenuBackgroundPath = path;
}

QString Project::startMenuBgmPath() const
{
    return m_startMenuBgmPath;
}

void Project::setStartMenuBgmPath(const QString &path)
{
    m_startMenuBgmPath = path;
}

int Project::startMenuFontSize() const
{
    return m_startMenuFontSize;
}

void Project::setStartMenuFontSize(int size)
{
    m_startMenuFontSize = qBound(8, size, 96);
}

QString Project::startMenuFontColor() const
{
    return m_startMenuFontColor;
}

void Project::setStartMenuFontColor(const QString &color)
{
    m_startMenuFontColor = color.trimmed().isEmpty() ? QString("#FFFFFF") : color.trimmed();
}

QString Project::startMenuTitle() const
{
    return m_startMenuTitle;
}

void Project::setStartMenuTitle(const QString &title)
{
    m_startMenuTitle = title;
}

int Project::startMenuTitleFontSize() const
{
    return m_startMenuTitleFontSize;
}

void Project::setStartMenuTitleFontSize(int size)
{
    m_startMenuTitleFontSize = qBound(8, size, 160);
}

QString Project::startMenuTitleColor() const
{
    return m_startMenuTitleColor;
}

void Project::setStartMenuTitleColor(const QString &color)
{
    m_startMenuTitleColor = color.trimmed().isEmpty() ? QString("#FFFFFF") : color.trimmed();
}

int Project::startMenuTitleX() const
{
    return m_startMenuTitleX;
}

void Project::setStartMenuTitleX(int x)
{
    m_startMenuTitleX = qMax(0, x);
}

int Project::startMenuTitleY() const
{
    return m_startMenuTitleY;
}

void Project::setStartMenuTitleY(int y)
{
    m_startMenuTitleY = qMax(0, y);
}

int Project::startMenuOptionsX() const
{
    return m_startMenuOptionsX;
}

void Project::setStartMenuOptionsX(int x)
{
    m_startMenuOptionsX = qMax(0, x);
}

int Project::startMenuOptionsY() const
{
    return m_startMenuOptionsY;
}

void Project::setStartMenuOptionsY(int y)
{
    m_startMenuOptionsY = qMax(0, y);
}

int Project::dialogueNameFontSize() const
{
    return m_dialogueNameFontSize;
}

void Project::setDialogueNameFontSize(int size)
{
    m_dialogueNameFontSize = qBound(8, size, 96);
}

QString Project::dialogueNameFontColor() const
{
    return m_dialogueNameFontColor;
}

void Project::setDialogueNameFontColor(const QString &color)
{
    m_dialogueNameFontColor = color.trimmed().isEmpty() ? QString("#FFFFFF") : color.trimmed();
}

int Project::dialogueTextFontSize() const
{
    return m_dialogueTextFontSize;
}

void Project::setDialogueTextFontSize(int size)
{
    m_dialogueTextFontSize = qBound(8, size, 96);
}

QString Project::dialogueTextFontColor() const
{
    return m_dialogueTextFontColor;
}

void Project::setDialogueTextFontColor(const QString &color)
{
    m_dialogueTextFontColor = color.trimmed().isEmpty() ? QString("#FFFFFF") : color.trimmed();
}

QString Project::dialogueBoxColor() const
{
    return m_dialogueBoxColor;
}

void Project::setDialogueBoxColor(const QString &color)
{
    m_dialogueBoxColor = color.trimmed().isEmpty() ? QString("#000000") : color.trimmed();
}

QString Project::autoPlayIndicatorColor() const
{
    return m_autoPlayIndicatorColor;
}

void Project::setAutoPlayIndicatorColor(const QString &color)
{
    m_autoPlayIndicatorColor = color.trimmed().isEmpty() ? QString("#7CFC00") : color.trimmed();
}

QString Project::settingsButtonColor() const
{
    return m_settingsButtonColor;
}

void Project::setSettingsButtonColor(const QString &color)
{
    m_settingsButtonColor = color.trimmed().isEmpty() ? QString("#FFFFFF") : color.trimmed();
}

QList<Character *> Project::characters() const
{
    return m_characters;
}

QList<Dialogue *> Project::dialogues() const
{
    return m_dialogues;
}

QList<Background *> Project::backgrounds() const
{
    return m_backgrounds;
}

QList<BgmTrack *> Project::bgmTracks() const
{
    return m_bgmTracks;
}

QList<QObject *> Project::characterObjects() const
{
    QList<QObject *> result;
    result.reserve(m_characters.size());
    for (Character *character : m_characters) {
        result.append(character);
    }
    return result;
}

QList<QObject *> Project::dialogueObjects() const
{
    QList<QObject *> result;
    result.reserve(m_dialogues.size());
    for (Dialogue *dialogue : m_dialogues) {
        result.append(dialogue);
    }
    return result;
}

QList<QObject *> Project::backgroundObjects() const
{
    QList<QObject *> result;
    result.reserve(m_backgrounds.size());
    for (Background *background : m_backgrounds) {
        result.append(background);
    }
    return result;
}

QList<QObject *> Project::bgmTrackObjects() const
{
    QList<QObject *> result;
    result.reserve(m_bgmTracks.size());
    for (BgmTrack *track : m_bgmTracks) {
        result.append(track);
    }
    return result;
}

void Project::addCharacter(Character *character)
{
    if (!character) {
        return;
    }
    character->setParent(this);
    m_characters.append(character);
}

void Project::addDialogue(Dialogue *dialogue)
{
    if (!dialogue) {
        return;
    }
    dialogue->setParent(this);
    m_dialogues.append(dialogue);
}

void Project::insertDialogueAt(Dialogue *dialogue, int index)
{
    if (!dialogue) {
        return;
    }
    dialogue->setParent(this);
    const int clampedIndex = qBound(0, index, m_dialogues.size());
    m_dialogues.insert(clampedIndex, dialogue);
}

void Project::addBackground(Background *background)
{
    if (!background) {
        return;
    }
    background->setParent(this);
    m_backgrounds.append(background);
}

void Project::addBgmTrack(BgmTrack *track)
{
    if (!track) {
        return;
    }
    track->setParent(this);
    m_bgmTracks.append(track);
}

void Project::clear()
{
    qDeleteAll(m_characters);
    qDeleteAll(m_dialogues);
    qDeleteAll(m_backgrounds);
    qDeleteAll(m_bgmTracks);
    m_characters.clear();
    m_dialogues.clear();
    m_backgrounds.clear();
    m_bgmTracks.clear();
}

bool Project::saveToFile(const QString &path) const
{
    const QString filePath = resolveProjectJsonPath(path);
    if (filePath.isEmpty()) {
        return false;
    }

    if (!hasContinuousDialogueIds(m_dialogues)) {
        return false;
    }

    if (!ensureResourceDirectories(filePath)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonArray charactersJson;
    QJsonArray backgroundsJson;
    QJsonArray dialoguesJson;
    QJsonArray bgmTracksJson;

    for (Character *character : m_characters) {
        charactersJson.append(character->toJson());
    }
    for (Background *background : m_backgrounds) {
        backgroundsJson.append(background->toJson());
    }
    for (Dialogue *dialogue : m_dialogues) {
        dialoguesJson.append(dialogue->toJson());
    }
    for (BgmTrack *track : m_bgmTracks) {
        bgmTracksJson.append(track->toJson());
    }

    QJsonObject root;
    root["name"] = m_name;
    root["version"] = m_version;
    root["defaultFont"] = fontToJson(m_defaultFont);
    root["startMenuBackgroundPath"] = m_startMenuBackgroundPath;
    root["startMenuBgmPath"] = m_startMenuBgmPath;
    root["startMenuFontSize"] = m_startMenuFontSize;
    root["startMenuFontColor"] = m_startMenuFontColor;
    root["startMenuTitle"] = m_startMenuTitle;
    root["startMenuTitleFontSize"] = m_startMenuTitleFontSize;
    root["startMenuTitleColor"] = m_startMenuTitleColor;
    root["startMenuTitleX"] = m_startMenuTitleX;
    root["startMenuTitleY"] = m_startMenuTitleY;
    root["startMenuOptionsX"] = m_startMenuOptionsX;
    root["startMenuOptionsY"] = m_startMenuOptionsY;
    root["dialogueNameFontSize"] = m_dialogueNameFontSize;
    root["dialogueNameFontColor"] = m_dialogueNameFontColor;
    root["dialogueTextFontSize"] = m_dialogueTextFontSize;
    root["dialogueTextFontColor"] = m_dialogueTextFontColor;
    root["dialogueBoxColor"] = m_dialogueBoxColor;
    root["autoPlayIndicatorColor"] = m_autoPlayIndicatorColor;
    root["settingsButtonColor"] = m_settingsButtonColor;
    root["characters"] = charactersJson;
    root["backgrounds"] = backgroundsJson;
    root["dialogues"] = dialoguesJson;
    root["bgmTracks"] = bgmTracksJson;

    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

Project *Project::loadFromFile(const QString &path, QObject *parent, QString *errorMessage)
{
    const QString filePath = resolveProjectJsonPath(path);
    if (filePath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "项目路径为空。";
        }
        return nullptr;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QString("无法打开文件：%1").arg(filePath);
        }
        return nullptr;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QString("JSON 解析失败：%1").arg(parseError.errorString());
        }
        return nullptr;
    }

    const QJsonObject root = doc.object();
    auto *project = new Project(parent);
    project->setName(root.value("name").toString());
    project->setVersion(root.value("version").toString("1.0"));
    project->setDefaultFont(fontFromJson(root.value("defaultFont")));
    project->setStartMenuBackgroundPath(root.value("startMenuBackgroundPath").toString());
    project->setStartMenuBgmPath(root.value("startMenuBgmPath").toString());
    project->setStartMenuFontSize(root.value("startMenuFontSize").toInt(22));
    project->setStartMenuFontColor(root.value("startMenuFontColor").toString("#FFFFFF"));
    project->setStartMenuTitle(root.value("startMenuTitle").toString("Galgame"));
    project->setStartMenuTitleFontSize(root.value("startMenuTitleFontSize").toInt(44));
    project->setStartMenuTitleColor(root.value("startMenuTitleColor").toString("#FFFFFF"));
    project->setStartMenuTitleX(root.value("startMenuTitleX").toInt(80));
    project->setStartMenuTitleY(root.value("startMenuTitleY").toInt(80));
    project->setStartMenuOptionsX(root.value("startMenuOptionsX").toInt(120));
    project->setStartMenuOptionsY(root.value("startMenuOptionsY").toInt(520));
    project->setDialogueNameFontSize(root.value("dialogueNameFontSize").toInt(14));
    project->setDialogueNameFontColor(root.value("dialogueNameFontColor").toString("#FFFFFF"));
    project->setDialogueTextFontSize(root.value("dialogueTextFontSize").toInt(12));
    project->setDialogueTextFontColor(root.value("dialogueTextFontColor").toString("#FFFFFF"));
    project->setDialogueBoxColor(root.value("dialogueBoxColor").toString("#000000"));
    project->setAutoPlayIndicatorColor(root.value("autoPlayIndicatorColor").toString("#7CFC00"));
    project->setSettingsButtonColor(root.value("settingsButtonColor").toString("#FFFFFF"));

    const QJsonArray charactersJson = root.value("characters").toArray();
    const QJsonArray backgroundsJson = root.value("backgrounds").toArray();
    const QJsonArray dialoguesJson = root.value("dialogues").toArray();
    const QJsonArray bgmTracksJson = root.value("bgmTracks").toArray();

    for (const QJsonValue &value : charactersJson) {
        project->addCharacter(Character::fromJson(value.toObject(), project));
    }
    for (const QJsonValue &value : backgroundsJson) {
        project->addBackground(Background::fromJson(value.toObject(), project));
    }
    for (const QJsonValue &value : dialoguesJson) {
        project->addDialogue(Dialogue::fromJson(value.toObject(), project));
    }
    for (const QJsonValue &value : bgmTracksJson) {
        project->addBgmTrack(BgmTrack::fromJson(value.toObject(), project));
    }

    if (project->m_bgmTracks.isEmpty()) {
        int autoId = 1;
        for (Background *background : project->m_backgrounds) {
            if (background->bgmPath().trimmed().isEmpty()) {
                continue;
            }
            auto *track = new BgmTrack(project);
            track->setId(QString("bgm%1").arg(autoId++));
            track->setBackgroundId(background->id());
            track->setBgmPath(background->bgmPath());
            track->setStartDialogueId(background->bgmStartDialogueId());
            track->setEndDialogueId(background->bgmEndDialogueId());
            project->addBgmTrack(track);
        }
    }

    // 兼容旧版本/外部数据：若 ID 不连续则自动重排，不直接判加载失败。
    if (!hasContinuousDialogueIds(project->m_dialogues)) {
        project->normalizeDialogueIds();
    }

    return project;
}

int Project::getNextDialogueId() const
{
    int maxId = 0;
    for (const Dialogue *dialogue : m_dialogues) {
        if (dialogue->id() > maxId) {
            maxId = dialogue->id();
        }
    }
    return maxId + 1;
}

int Project::dialogueCount() const
{
    return m_dialogues.size();
}

Dialogue *Project::dialogueAt(int index) const
{
    if (index < 0 || index >= m_dialogues.size()) {
        return nullptr;
    }
    return m_dialogues.at(index);
}

bool Project::moveDialogue(int from, int to)
{
    if (from < 0 || from >= m_dialogues.size() || to < 0 || to >= m_dialogues.size()) {
        return false;
    }

    if (from == to) {
        return true;
    }

    m_dialogues.move(from, to);
    normalizeDialogueIds();
    return true;
}

void Project::normalizeDialogueIds()
{
    for (int i = 0; i < m_dialogues.size(); ++i) {
        m_dialogues.at(i)->setId(i + 1);
    }
}

bool Project::removeCharacterById(const QString &id)
{
    if (id.trimmed().isEmpty()) {
        return false;
    }

    bool removed = false;
    for (int i = m_characters.size() - 1; i >= 0; --i) {
        Character *character = m_characters.at(i);
        if (character->id() != id) {
            continue;
        }
        m_characters.removeAt(i);
        delete character;
        removed = true;
    }

    if (!removed) {
        return false;
    }

    for (int i = m_dialogues.size() - 1; i >= 0; --i) {
        Dialogue *dialogue = m_dialogues.at(i);
        if (dialogue->characterId() != id) {
            continue;
        }
        m_dialogues.removeAt(i);
        delete dialogue;
    }
    normalizeDialogueIds();
    return true;
}

bool Project::removeBackgroundById(const QString &id)
{
    if (id.trimmed().isEmpty()) {
        return false;
    }

    for (int i = m_backgrounds.size() - 1; i >= 0; --i) {
        Background *background = m_backgrounds.at(i);
        if (background->id() != id) {
            continue;
        }
        m_backgrounds.removeAt(i);
        delete background;
        for (int j = m_bgmTracks.size() - 1; j >= 0; --j) {
            BgmTrack *track = m_bgmTracks.at(j);
            if (track->backgroundId() != id) {
                continue;
            }
            m_bgmTracks.removeAt(j);
            delete track;
        }
        return true;
    }
    return false;
}

bool Project::removeDialogueById(int id)
{
    if (id <= 0) {
        return false;
    }

    for (int i = m_dialogues.size() - 1; i >= 0; --i) {
        Dialogue *dialogue = m_dialogues.at(i);
        if (dialogue->id() != id) {
            continue;
        }
        m_dialogues.removeAt(i);
        delete dialogue;
        normalizeDialogueIds();
        return true;
    }
    return false;
}

const Background *Project::getBackgroundForDialogue(int id) const
{
    const Background *selected = nullptr;
    for (const Background *background : m_backgrounds) {
        if (!background->coversDialogue(id)) {
            continue;
        }
        if (!selected || background->startDialogueId() >= selected->startDialogueId()) {
            selected = background;
        }
    }
    return selected;
}

const Background *Project::getBgmBackgroundForDialogue(int id) const
{
    const Background *bg = getBackgroundForDialogue(id);
    if (!bg) {
        return nullptr;
    }
    return getBgmTrackForDialogue(id, bg->id()) ? bg : nullptr;
}

const BgmTrack *Project::getBgmTrackForDialogue(int id, const QString &backgroundId) const
{
    const BgmTrack *selected = nullptr;
    for (const BgmTrack *track : m_bgmTracks) {
        if (track->bgmPath().trimmed().isEmpty()) {
            continue;
        }
        if (!backgroundId.trimmed().isEmpty() && track->backgroundId() != backgroundId) {
            continue;
        }
        if (!track->coversDialogue(id)) {
            continue;
        }
        if (!selected || track->startDialogueId() >= selected->startDialogueId()) {
            selected = track;
        }
    }
    return selected;
}

bool Project::removeBgmTrackById(const QString &id)
{
    if (id.trimmed().isEmpty()) {
        return false;
    }
    for (int i = m_bgmTracks.size() - 1; i >= 0; --i) {
        BgmTrack *track = m_bgmTracks.at(i);
        if (track->id() != id) {
            continue;
        }
        m_bgmTracks.removeAt(i);
        delete track;
        return true;
    }
    return false;
}

const Character *Project::getCharacter(const QString &id) const
{
    for (Character *character : m_characters) {
        if (character->id() == id) {
            return character;
        }
    }
    return nullptr;
}

Character *Project::getCharacter(const QString &id)
{
    for (Character *character : m_characters) {
        if (character->id() == id) {
            return character;
        }
    }
    return nullptr;
}
