#ifndef BGMTRACK_H
#define BGMTRACK_H

#include <QObject>
#include <QJsonObject>
#include <QString>

class BgmTrack : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString backgroundId READ backgroundId WRITE setBackgroundId)
    Q_PROPERTY(QString bgmPath READ bgmPath WRITE setBgmPath)
    Q_PROPERTY(int startDialogueId READ startDialogueId WRITE setStartDialogueId)
    Q_PROPERTY(int endDialogueId READ endDialogueId WRITE setEndDialogueId)

public:
    explicit BgmTrack(QObject *parent = nullptr);

    QString id() const;
    void setId(const QString &id);

    QString backgroundId() const;
    void setBackgroundId(const QString &backgroundId);

    QString bgmPath() const;
    void setBgmPath(const QString &bgmPath);

    int startDialogueId() const;
    void setStartDialogueId(int id);

    int endDialogueId() const;
    void setEndDialogueId(int id);

    bool coversDialogue(int dialogueId) const;

    QJsonObject toJson() const;
    static BgmTrack *fromJson(const QJsonObject &json, QObject *parent = nullptr);

private:
    QString m_id;
    QString m_backgroundId;
    QString m_bgmPath;
    int m_startDialogueId = 1;
    int m_endDialogueId = 1;
};

#endif // BGMTRACK_H
