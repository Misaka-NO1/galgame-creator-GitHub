#ifndef PROJECT_H
#define PROJECT_H

#include "background.h"
#include "bgmtrack.h"
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
    Q_PROPERTY(QString startMenuBackgroundPath READ startMenuBackgroundPath WRITE setStartMenuBackgroundPath)
    Q_PROPERTY(QString startMenuBgmPath READ startMenuBgmPath WRITE setStartMenuBgmPath)
    Q_PROPERTY(int startMenuFontSize READ startMenuFontSize WRITE setStartMenuFontSize)
    Q_PROPERTY(QString startMenuFontColor READ startMenuFontColor WRITE setStartMenuFontColor)
    Q_PROPERTY(QString startMenuTitle READ startMenuTitle WRITE setStartMenuTitle)
    Q_PROPERTY(int startMenuTitleFontSize READ startMenuTitleFontSize WRITE setStartMenuTitleFontSize)
    Q_PROPERTY(QString startMenuTitleColor READ startMenuTitleColor WRITE setStartMenuTitleColor)
    Q_PROPERTY(int startMenuTitleX READ startMenuTitleX WRITE setStartMenuTitleX)
    Q_PROPERTY(int startMenuTitleY READ startMenuTitleY WRITE setStartMenuTitleY)
    Q_PROPERTY(int startMenuOptionsX READ startMenuOptionsX WRITE setStartMenuOptionsX)
    Q_PROPERTY(int startMenuOptionsY READ startMenuOptionsY WRITE setStartMenuOptionsY)
    Q_PROPERTY(int dialogueNameFontSize READ dialogueNameFontSize WRITE setDialogueNameFontSize)
    Q_PROPERTY(QString dialogueNameFontColor READ dialogueNameFontColor WRITE setDialogueNameFontColor)
    Q_PROPERTY(int dialogueTextFontSize READ dialogueTextFontSize WRITE setDialogueTextFontSize)
    Q_PROPERTY(QString dialogueTextFontColor READ dialogueTextFontColor WRITE setDialogueTextFontColor)
    Q_PROPERTY(QString dialogueBoxColor READ dialogueBoxColor WRITE setDialogueBoxColor)
    Q_PROPERTY(QString autoPlayIndicatorColor READ autoPlayIndicatorColor WRITE setAutoPlayIndicatorColor)
    Q_PROPERTY(QString settingsButtonColor READ settingsButtonColor WRITE setSettingsButtonColor)
    Q_PROPERTY(QList<QObject *> characters READ characterObjects)
    Q_PROPERTY(QList<QObject *> dialogues READ dialogueObjects)
    Q_PROPERTY(QList<QObject *> backgrounds READ backgroundObjects)
    Q_PROPERTY(QList<QObject *> bgmTracks READ bgmTrackObjects)

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

    // 获取开始界面背景图路径。
    QString startMenuBackgroundPath() const;
    // 设置开始界面背景图路径。
    void setStartMenuBackgroundPath(const QString &path);

    // 获取开始界面背景音乐路径。
    QString startMenuBgmPath() const;
    // 设置开始界面背景音乐路径。
    void setStartMenuBgmPath(const QString &path);

    // 获取开始界面按钮字体大小。
    int startMenuFontSize() const;
    // 设置开始界面按钮字体大小。
    void setStartMenuFontSize(int size);
    // 获取开始界面按钮字体颜色。
    QString startMenuFontColor() const;
    // 设置开始界面按钮字体颜色。
    void setStartMenuFontColor(const QString &color);
    // 获取开始界面标题文本。
    QString startMenuTitle() const;
    // 设置开始界面标题文本。
    void setStartMenuTitle(const QString &title);
    // 获取开始界面标题字号。
    int startMenuTitleFontSize() const;
    // 设置开始界面标题字号。
    void setStartMenuTitleFontSize(int size);
    // 获取开始界面标题颜色。
    QString startMenuTitleColor() const;
    // 设置开始界面标题颜色。
    void setStartMenuTitleColor(const QString &color);
    // 获取开始界面标题 X 坐标。
    int startMenuTitleX() const;
    // 设置开始界面标题 X 坐标。
    void setStartMenuTitleX(int x);
    // 获取开始界面标题 Y 坐标。
    int startMenuTitleY() const;
    // 设置开始界面标题 Y 坐标。
    void setStartMenuTitleY(int y);
    // 获取开始界面选项区 X 坐标。
    int startMenuOptionsX() const;
    // 设置开始界面选项区 X 坐标。
    void setStartMenuOptionsX(int x);
    // 获取开始界面选项区 Y 坐标。
    int startMenuOptionsY() const;
    // 设置开始界面选项区 Y 坐标。
    void setStartMenuOptionsY(int y);

    // 获取对话姓名字体大小。
    int dialogueNameFontSize() const;
    // 设置对话姓名字体大小。
    void setDialogueNameFontSize(int size);
    // 获取对话姓名字体颜色。
    QString dialogueNameFontColor() const;
    // 设置对话姓名字体颜色。
    void setDialogueNameFontColor(const QString &color);

    // 获取对话文本字体大小。
    int dialogueTextFontSize() const;
    // 设置对话文本字体大小。
    void setDialogueTextFontSize(int size);
    // 获取对话文本字体颜色。
    QString dialogueTextFontColor() const;
    // 设置对话文本字体颜色。
    void setDialogueTextFontColor(const QString &color);
    // 获取对话背景框颜色。
    QString dialogueBoxColor() const;
    // 设置对话背景框颜色。
    void setDialogueBoxColor(const QString &color);
    // 获取自动播放图标颜色。
    QString autoPlayIndicatorColor() const;
    // 设置自动播放图标颜色。
    void setAutoPlayIndicatorColor(const QString &color);
    // 获取设置按钮颜色。
    QString settingsButtonColor() const;
    // 设置设置按钮颜色。
    void setSettingsButtonColor(const QString &color);

    // 获取角色对象列表（强类型）。
    QList<Character *> characters() const;
    // 获取对话对象列表（强类型）。
    QList<Dialogue *> dialogues() const;
    // 获取背景对象列表（强类型）。
    QList<Background *> backgrounds() const;
    // 获取背景音乐轨道列表（强类型）。
    QList<BgmTrack *> bgmTracks() const;

    // 为 Q_PROPERTY 暴露 QObject 列表，便于后续接入 QML。
    QList<QObject *> characterObjects() const;
    QList<QObject *> dialogueObjects() const;
    QList<QObject *> backgroundObjects() const;
    QList<QObject *> bgmTrackObjects() const;

    // 添加并托管角色对象所有权。
    void addCharacter(Character *character);
    // 添加并托管对话对象所有权。
    void addDialogue(Dialogue *dialogue);
    // 在指定索引处插入对话对象，越界时自动夹紧。
    void insertDialogueAt(Dialogue *dialogue, int index);
    // 添加并托管背景对象所有权。
    void addBackground(Background *background);
    // 添加并托管背景音乐轨道对象所有权。
    void addBgmTrack(BgmTrack *track);
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
    // 根据对话 ID 获取应播放的背景音乐来源背景对象。
    const Background *getBgmBackgroundForDialogue(int id) const;
    // 根据对话 ID 和背景 ID 获取应播放的背景音乐轨道。
    const BgmTrack *getBgmTrackForDialogue(int id, const QString &backgroundId) const;
    // 删除指定背景音乐轨道。
    bool removeBgmTrackById(const QString &id);
    // 根据角色 ID 查找角色对象（只读）。
    const Character *getCharacter(const QString &id) const;
    // 根据角色 ID 查找角色对象（可写）。
    Character *getCharacter(const QString &id);

private:
    QString m_name;
    QString m_version = "1.0";
    QFont m_defaultFont;
    QString m_startMenuBackgroundPath;
    QString m_startMenuBgmPath;
    int m_startMenuFontSize = 22;
    QString m_startMenuFontColor = "#FFFFFF";
    QString m_startMenuTitle = "Galgame";
    int m_startMenuTitleFontSize = 44;
    QString m_startMenuTitleColor = "#FFFFFF";
    int m_startMenuTitleX = 80;
    int m_startMenuTitleY = 80;
    int m_startMenuOptionsX = 120;
    int m_startMenuOptionsY = 520;
    int m_dialogueNameFontSize = 14;
    QString m_dialogueNameFontColor = "#FFFFFF";
    int m_dialogueTextFontSize = 12;
    QString m_dialogueTextFontColor = "#FFFFFF";
    QString m_dialogueBoxColor = "#000000";
    QString m_autoPlayIndicatorColor = "#7CFC00";
    QString m_settingsButtonColor = "#FFFFFF";
    QList<Character *> m_characters;
    QList<Dialogue *> m_dialogues;
    QList<Background *> m_backgrounds;
    QList<BgmTrack *> m_bgmTracks;
};

#endif // PROJECT_H
