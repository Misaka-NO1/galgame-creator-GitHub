#ifndef CHARACTER_H
#define CHARACTER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QDir>

class Character : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString portraitPath READ portraitPath WRITE setPortraitPath)
    Q_PROPERTY(QString avatarPath READ avatarPath WRITE setAvatarPath)
    Q_PROPERTY(QString voicePrefix READ voicePrefix WRITE setVoicePrefix)
    Q_PROPERTY(int portraitScale READ portraitScale WRITE setPortraitScale)
    Q_PROPERTY(Position position READ position WRITE setPosition)

public:
    enum class Position {
        Left,
        Center,
        Right
    };
    Q_ENUM(Position)

    explicit Character(QObject *parent = nullptr);

    // 获取角色唯一 ID。
    QString id() const;
    // 设置角色唯一 ID。
    void setId(const QString &id);

    // 获取角色显示名称。
    QString name() const;
    // 设置角色显示名称。
    void setName(const QString &name);

    // 获取立绘文件路径。
    QString portraitPath() const;
    // 设置立绘文件路径。
    void setPortraitPath(const QString &portraitPath);

    // 获取头像文件路径。
    QString avatarPath() const;
    // 设置头像文件路径。
    void setAvatarPath(const QString &avatarPath);

    // 获取角色语音文件前缀。
    QString voicePrefix() const;
    // 设置角色语音文件前缀。
    void setVoicePrefix(const QString &voicePrefix);

    // 获取立绘缩放百分比（100 为原始基准）。
    int portraitScale() const;
    // 设置立绘缩放百分比（限制在 10~300）。
    void setPortraitScale(int scalePercent);

    // 获取立绘显示位置。
    Position position() const;
    // 设置立绘显示位置。
    void setPosition(Position position);

    // 序列化角色数据到 JSON 对象。
    QJsonObject toJson() const;
    // 从 JSON 反序列化角色对象，并绑定可选父对象。
    static Character *fromJson(const QJsonObject &json, QObject *parent = nullptr);
    // 校验立绘/头像路径是否有效，支持相对路径基于 baseDir 解析。
    bool validate(const QDir &baseDir = QDir()) const;

private:
    QString m_id;
    QString m_name;
    QString m_portraitPath;
    QString m_avatarPath;
    QString m_voicePrefix;
    int m_portraitScale = 100;
    Position m_position = Position::Center;
};

#endif // CHARACTER_H
