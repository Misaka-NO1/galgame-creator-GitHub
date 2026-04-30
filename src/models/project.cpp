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

void Project::clear()
{
    qDeleteAll(m_characters);
    qDeleteAll(m_dialogues);
    qDeleteAll(m_backgrounds);
    m_characters.clear();
    m_dialogues.clear();
    m_backgrounds.clear();
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

    for (Character *character : m_characters) {
        charactersJson.append(character->toJson());
    }
    for (Background *background : m_backgrounds) {
        backgroundsJson.append(background->toJson());
    }
    for (Dialogue *dialogue : m_dialogues) {
        dialoguesJson.append(dialogue->toJson());
    }

    QJsonObject root;
    root["name"] = m_name;
    root["version"] = m_version;
    root["defaultFont"] = fontToJson(m_defaultFont);
    root["startMenuBackgroundPath"] = m_startMenuBackgroundPath;
    root["startMenuBgmPath"] = m_startMenuBgmPath;
    root["characters"] = charactersJson;
    root["backgrounds"] = backgroundsJson;
    root["dialogues"] = dialoguesJson;

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

    const QJsonArray charactersJson = root.value("characters").toArray();
    const QJsonArray backgroundsJson = root.value("backgrounds").toArray();
    const QJsonArray dialoguesJson = root.value("dialogues").toArray();

    for (const QJsonValue &value : charactersJson) {
        project->addCharacter(Character::fromJson(value.toObject(), project));
    }
    for (const QJsonValue &value : backgroundsJson) {
        project->addBackground(Background::fromJson(value.toObject(), project));
    }
    for (const QJsonValue &value : dialoguesJson) {
        project->addDialogue(Dialogue::fromJson(value.toObject(), project));
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
