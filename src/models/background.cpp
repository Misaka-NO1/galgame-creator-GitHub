#include "background.h"

Background::Background(QObject *parent)
    : QObject(parent)
{
}

QString Background::id() const
{
    return m_id;
}

void Background::setId(const QString &id)
{
    m_id = id;
}

QString Background::imagePath() const
{
    return m_imagePath;
}

void Background::setImagePath(const QString &imagePath)
{
    m_imagePath = imagePath;
}

int Background::startDialogueId() const
{
    return m_startDialogueId;
}

void Background::setStartDialogueId(int startDialogueId)
{
    m_startDialogueId = startDialogueId;
}

int Background::endDialogueId() const
{
    return m_endDialogueId;
}

void Background::setEndDialogueId(int endDialogueId)
{
    m_endDialogueId = endDialogueId;
}

QString Background::bgmPath() const
{
    return m_bgmPath;
}

void Background::setBgmPath(const QString &bgmPath)
{
    m_bgmPath = bgmPath;
}

QJsonObject Background::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["imagePath"] = m_imagePath;
    json["startDialogueId"] = m_startDialogueId;
    json["endDialogueId"] = m_endDialogueId;
    json["bgmPath"] = m_bgmPath;
    return json;
}

Background *Background::fromJson(const QJsonObject &json, QObject *parent)
{
    auto *background = new Background(parent);
    background->setId(json.value("id").toString());
    background->setImagePath(json.value("imagePath").toString());
    background->setStartDialogueId(json.value("startDialogueId").toInt(1));
    background->setEndDialogueId(json.value("endDialogueId").toInt(background->startDialogueId()));
    background->setBgmPath(json.value("bgmPath").toString());
    return background;
}

bool Background::coversDialogue(int dialogueId) const
{
    return dialogueId >= m_startDialogueId && dialogueId <= m_endDialogueId;
}

bool Background::conflictsWith(const Background &other) const
{
    return !(m_endDialogueId < other.startDialogueId() || other.endDialogueId() < m_startDialogueId);
}
