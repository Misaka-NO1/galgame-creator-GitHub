#ifndef PROJECT_H
#define PROJECT_H

#include "background.h"
#include "character.h"
#include "dialogue.h"

#include <QObject>
#include <QFont>
#include <QList>
#include <QString>

class Project : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString version READ version WRITE setVersion)
    Q_PROPERTY(QFont defaultFont READ defaultFont WRITE setDefaultFont)
    Q_PROPERTY(QList<QObject *> characters READ characterObjects)
    Q_PROPERTY(QList<QObject *> dialogues READ dialogueObjects)
    Q_PROPERTY(QList<QObject *> backgrounds READ backgroundObjects)

public:
    explicit Project(QObject *parent = nullptr);
    ~Project() override;

    // 获取项目名称。
    QString name() const;
    // 设置项目名称。
    void setName(const QString &name);

    // 获取项目版本号。
    QString version() const;
    // 设置项目版本号。
    void setVersion(const QString &version);

    // 获取项目默认字体。
    QFont defaultFont() const;
    // 设置项目默认字体。
    void setDefaultFont(const QFont &defaultFont);

    // 获取角色对象列表（强类型）。
    QList<Character *> characters() const;
    // 获取对话对象列表（强类型）。
    QList<Dialogue *> dialogues() const;
    // 获取背景对象列表（强类型）。
    QList<Background *> backgrounds() const;

    // 为 Q_PROPERTY 暴露 QObject 列表，便于后续接入 QML。
    QList<QObject *> characterObjects() const;
    QList<QObject *> dialogueObjects() const;
    QList<QObject *> backgroundObjects() const;

    // 添加并托管角色对象所有权。
    void addCharacter(Character *character);
    // 添加并托管对话对象所有权。
    void addDialogue(Dialogue *dialogue);
    // 在指定索引处插入对话对象，越界时自动夹紧。
    void insertDialogueAt(Dialogue *dialogue, int index);
    // 添加并托管背景对象所有权。
    void addBackground(Background *background);
    // 清空当前项目中的所有角色、对话和背景对象。
    void clear();

    // 保存项目到 project.json，并自动创建 resources 子目录。
    bool saveToFile(const QString &path) const;
    // 从 project.json 加载项目并重建对象关系。
    static Project *loadFromFile(const QString &path, QObject *parent = nullptr, QString *errorMessage = nullptr);

    // 返回当前对话最大 ID + 1。
    int getNextDialogueId() const;
    // 返回当前对话总数。
    int dialogueCount() const;
    // 按索引获取对话对象，越界返回空指针。
    Dialogue *dialogueAt(int index) const;
    // 将对话从 from 索引移动到 to 索引。
    bool moveDialogue(int from, int to);
    // 将当前顺序重排为从 1 开始的连续 ID。
    void normalizeDialogueIds();
    // 删除指定角色及其关联对话。
    bool removeCharacterById(const QString &id);
    // 删除指定背景。
    bool removeBackgroundById(const QString &id);
    // 删除指定对话。
    bool removeDialogueById(int id);
    // 根据对话 ID 获取应显示的背景对象。
    const Background *getBackgroundForDialogue(int id) const;
    // 根据角色 ID 查找角色对象（只读）。
    const Character *getCharacter(const QString &id) const;
    // 根据角色 ID 查找角色对象（可写）。
    Character *getCharacter(const QString &id);

private:
    QString m_name;
    QString m_version = "1.0";
    QFont m_defaultFont;
    QList<Character *> m_characters;
    QList<Dialogue *> m_dialogues;
    QList<Background *> m_backgrounds;
};

#endif // PROJECT_H
