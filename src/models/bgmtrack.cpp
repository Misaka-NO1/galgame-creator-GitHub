#include "bgmtrack.h"

BgmTrack::BgmTrack(QObject *parent)
    : QObject(parent)
{
}

QString BgmTrack::id() const
{
    return m_id;
}

void BgmTrack::setId(const QString &id)
{
    m_id = id;
}

QString BgmTrack::backgroundId() const
{
    return m_backgroundId;
}

void BgmTrack::setBackgroundId(const QString &backgroundId)
{
    m_backgroundId = backgroundId;
}

QString BgmTrack::bgmPath() const
{
    return m_bgmPath;
}

void BgmTrack::setBgmPath(const QString &bgmPath)
{
    m_bgmPath = bgmPath;
}

int BgmTrack::startDialogueId() const
{
    return m_startDialogueId;
}

void BgmTrack::setStartDialogueId(int id)
{
    m_startDialogueId = id;
}

int BgmTrack::endDialogueId() const
{
    return m_endDialogueId;
}

void BgmTrack::setEndDialogueId(int id)
{
    m_endDialogueId = id;
}

bool BgmTrack::coversDialogue(int dialogueId) const
{
    return dialogueId >= m_startDialogueId && dialogueId <= m_endDialogueId;
}

QJsonObject BgmTrack::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["backgroundId"] = m_backgroundId;
    json["bgmPath"] = m_bgmPath;
    json["startDialogueId"] = m_startDialogueId;
    json["endDialogueId"] = m_endDialogueId;
    return json;
}

BgmTrack *BgmTrack::fromJson(const QJsonObject &json, QObject *parent)
{
    auto *track = new BgmTrack(parent);
    track->setId(json.value("id").toString());
    track->setBackgroundId(json.value("backgroundId").toString());
    track->setBgmPath(json.value("bgmPath").toString());
    track->setStartDialogueId(json.value("startDialogueId").toInt(1));
    track->setEndDialogueId(json.value("endDialogueId").toInt(track->startDialogueId()));
    return track;
}
