#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <QObject>
#include <QJsonObject>
#include <QString>

class Background : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath)
    Q_PROPERTY(int startDialogueId READ startDialogueId WRITE setStartDialogueId)
    Q_PROPERTY(int endDialogueId READ endDialogueId WRITE setEndDialogueId)
    Q_PROPERTY(QString bgmPath READ bgmPath WRITE setBgmPath)
    Q_PROPERTY(int bgmStartDialogueId READ bgmStartDialogueId WRITE setBgmStartDialogueId)
    Q_PROPERTY(int bgmEndDialogueId READ bgmEndDialogueId WRITE setBgmEndDialogueId)

public:
    explicit Background(QObject *parent = nullptr);

    // 获取背景唯一 ID。
    QString id() const;
    // 设置背景唯一 ID。
    void setId(const QString &id);

    // 获取背景图路径。
    QString imagePath() const;
    // 设置背景图路径。
    void setImagePath(const QString &imagePath);

    // 获取背景起始对话 ID。
    int startDialogueId() const;
    // 设置背景起始对话 ID。
    void setStartDialogueId(int startDialogueId);

    // 获取背景结束对话 ID。
    int endDialogueId() const;
    // 设置背景结束对话 ID。
    void setEndDialogueId(int endDialogueId);

    // 获取背景音乐路径。
    QString bgmPath() const;
    // 设置背景音乐路径。
    void setBgmPath(const QString &bgmPath);

    // 获取背景音乐起始对话 ID。
    int bgmStartDialogueId() const;
    // 设置背景音乐起始对话 ID。
    void setBgmStartDialogueId(int id);

    // 获取背景音乐结束对话 ID。
    int bgmEndDialogueId() const;
    // 设置背景音乐结束对话 ID。
    void setBgmEndDialogueId(int id);

    // 序列化背景段信息到 JSON 对象。
    QJsonObject toJson() const;
    // 从 JSON 反序列化背景对象，并绑定可选父对象。
    static Background *fromJson(const QJsonObject &json, QObject *parent = nullptr);
    // 判断指定对话 ID 是否落在该背景覆盖区间。
    bool coversDialogue(int dialogueId) const;
    // 判断指定对话 ID 是否落在该背景音乐覆盖区间。
    bool coversBgmDialogue(int dialogueId) const;
    // 判断两个背景区间是否存在重叠冲突。
    bool conflictsWith(const Background &other) const;

private:
    QString m_id;
    QString m_imagePath;
    int m_startDialogueId = 1;
    int m_endDialogueId = 1;
    QString m_bgmPath;
    int m_bgmStartDialogueId = 1;
    int m_bgmEndDialogueId = 1;
};

#endif // BACKGROUND_H
