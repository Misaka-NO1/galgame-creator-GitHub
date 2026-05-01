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
    Q_PROPERTY(int nameFontSizeOverride READ nameFontSizeOverride WRITE setNameFontSizeOverride)
    Q_PROPERTY(QString nameFontColorOverride READ nameFontColorOverride WRITE setNameFontColorOverride)
    Q_PROPERTY(int textFontSizeOverride READ textFontSizeOverride WRITE setTextFontSizeOverride)
    Q_PROPERTY(QString textFontColorOverride READ textFontColorOverride WRITE setTextFontColorOverride)

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

    // 获取该句姓名字号覆盖值（<=0 表示使用全局）。
    int nameFontSizeOverride() const;
    // 设置该句姓名字号覆盖值（<=0 表示使用全局）。
    void setNameFontSizeOverride(int size);
    // 获取该句姓名颜色覆盖值（空表示使用全局）。
    QString nameFontColorOverride() const;
    // 设置该句姓名颜色覆盖值（空表示使用全局）。
    void setNameFontColorOverride(const QString &color);

    // 获取该句对白字号覆盖值（<=0 表示使用全局）。
    int textFontSizeOverride() const;
    // 设置该句对白字号覆盖值（<=0 表示使用全局）。
    void setTextFontSizeOverride(int size);
    // 获取该句对白颜色覆盖值（空表示使用全局）。
    QString textFontColorOverride() const;
    // 设置该句对白颜色覆盖值（空表示使用全局）。
    void setTextFontColorOverride(const QString &color);

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
    int m_nameFontSizeOverride = 0;
    QString m_nameFontColorOverride;
    int m_textFontSizeOverride = 0;
    QString m_textFontColorOverride;
};

#endif // DIALOGUE_H
