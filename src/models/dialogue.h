#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <QObject>
#include <QJsonObject>
#include <QString>

class Dialogue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId)
    Q_PROPERTY(QString characterId READ characterId WRITE setCharacterId)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString voiceFile READ voiceFile WRITE setVoiceFile)
    Q_PROPERTY(SpecialEffect specialEffect READ specialEffect WRITE setSpecialEffect)

public:
    enum class SpecialEffect {
        None,
        Shake,
        Flash,
        Fade
    };
    Q_ENUM(SpecialEffect)

    explicit Dialogue(QObject *parent = nullptr);

    // 获取对话 ID（从 1 开始）。
    int id() const;
    // 设置对话 ID。
    void setId(int id);

    // 获取发言角色 ID。
    QString characterId() const;
    // 设置发言角色 ID。
    void setCharacterId(const QString &characterId);

    // 获取对话文本内容。
    QString text() const;
    // 设置对话文本内容。
    void setText(const QString &text);

    // 获取语音文件名。
    QString voiceFile() const;
    // 设置语音文件名。
    void setVoiceFile(const QString &voiceFile);

    // 获取特效枚举值。
    SpecialEffect specialEffect() const;
    // 设置特效枚举值。
    void setSpecialEffect(SpecialEffect specialEffect);

    // 序列化单句对话到 JSON 对象。
    QJsonObject toJson() const;
    // 从 JSON 反序列化对话对象，并绑定可选父对象。
    static Dialogue *fromJson(const QJsonObject &json, QObject *parent = nullptr);

private:
    int m_id = 0;
    QString m_characterId;
    QString m_text;
    QString m_voiceFile;
    SpecialEffect m_specialEffect = SpecialEffect::None;
};

#endif // DIALOGUE_H
