#include "character.h"

#include <QFile>

namespace {

QString positionToString(Character::Position position)
{
    switch (position) {
    case Character::Position::Left:
        return "Left";
    case Character::Position::Center:
        return "Center";
    case Character::Position::Right:
        return "Right";
    }
    return "Center";
}

Character::Position positionFromString(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == "left") {
        return Character::Position::Left;
    }
    if (normalized == "right") {
        return Character::Position::Right;
    }
    return Character::Position::Center;
}

QString resolvePath(const QDir &baseDir, const QString &path)
{
    if (path.trimmed().isEmpty()) {
        return {};
    }

    if (QDir::isAbsolutePath(path)) {
        return QDir::cleanPath(path);
    }

    const QDir activeDir = baseDir.path().isEmpty() ? QDir::current() : baseDir;
    return QDir::cleanPath(activeDir.absoluteFilePath(path));
}

} // namespace

Character::Character(QObject *parent)
    : QObject(parent)
{
}

QString Character::id() const
{
    return m_id;
}

void Character::setId(const QString &id)
{
    m_id = id;
}

QString Character::name() const
{
    return m_name;
}

void Character::setName(const QString &name)
{
    m_name = name;
}

QString Character::portraitPath() const
{
    return m_portraitPath;
}

void Character::setPortraitPath(const QString &portraitPath)
{
    m_portraitPath = portraitPath;
}

QString Character::avatarPath() const
{
    return m_avatarPath;
}

void Character::setAvatarPath(const QString &avatarPath)
{
    m_avatarPath = avatarPath;
}

QString Character::voicePrefix() const
{
    return m_voicePrefix;
}

void Character::setVoicePrefix(const QString &voicePrefix)
{
    m_voicePrefix = voicePrefix;
}

int Character::portraitScale() const
{
    return m_portraitScale;
}

void Character::setPortraitScale(int scalePercent)
{
    m_portraitScale = qBound(10, scalePercent, 300);
}

Character::Position Character::position() const
{
    return m_position;
}

void Character::setPosition(Position position)
{
    m_position = position;
}

QJsonObject Character::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["portraitPath"] = m_portraitPath;
    json["avatarPath"] = m_avatarPath;
    json["voicePrefix"] = m_voicePrefix;
    json["portraitScale"] = m_portraitScale;
    json["position"] = positionToString(m_position);
    return json;
}

Character *Character::fromJson(const QJsonObject &json, QObject *parent)
{
    auto *character = new Character(parent);
    character->setId(json.value("id").toString());
    character->setName(json.value("name").toString());
    character->setPortraitPath(json.value("portraitPath").toString());
    character->setAvatarPath(json.value("avatarPath").toString());
    character->setVoicePrefix(json.value("voicePrefix").toString());
    character->setPortraitScale(json.value("portraitScale").toInt(100));
    character->setPosition(positionFromString(json.value("position").toString()));
    return character;
}

bool Character::validate(const QDir &baseDir) const
{
    const QString portraitAbsolutePath = resolvePath(baseDir, m_portraitPath);
    if (portraitAbsolutePath.isEmpty() || !QFile::exists(portraitAbsolutePath)) {
        return false;
    }

    if (!m_avatarPath.trimmed().isEmpty()) {
        const QString avatarAbsolutePath = resolvePath(baseDir, m_avatarPath);
        if (avatarAbsolutePath.isEmpty() || !QFile::exists(avatarAbsolutePath)) {
            return false;
        }
    }

    if (m_id.trimmed().isEmpty() || m_name.trimmed().isEmpty()) {
        return false;
    }

    return true;
}
