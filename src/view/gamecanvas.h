#ifndef GAMECANVAS_H
#define GAMECANVAS_H

#include "../models/background.h"
#include "../models/character.h"
#include "../models/dialogue.h"
#include "typewritereffect.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QTimer>

class GameCanvas : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GameCanvas(QWidget *parent = nullptr);

    // 根据当前对话、说话角色、背景进行完整渲染。
    void setDialogue(const Dialogue &d, const Character &speaker, const Background &bg);
    // 清空当前场景并恢复初始显示状态。
    void clearScene();

    // 显示当前说话角色立绘，其他角色不显示（单立绘模式）。
    void showCharacterPortrait(const QString &imagePath, Character::Position pos, int scalePercent = 100);
    // 隐藏当前立绘。
    void hideCharacterPortrait();

    // 设置并展示说话人和文本内容。
    void showText(const QString &speakerName, const QString &text);
    // 设置打字机速度，0 表示瞬间显示。
    void setTypingSpeed(int msPerChar);

    // 设置是否允许点击推进下一句。
    void setClickToAdvance(bool enabled);
    // 设置自动播放开关和推进延迟（毫秒）。
    void setAutoPlay(bool enabled, int msDelay);
    // 设置资源路径基准目录（用于解析相对图片路径）。
    void setResourceBaseDir(const QString &baseDir);
    // 设置对话背景框颜色（支持 #RRGGBB）。
    void setDialogueBoxColor(const QString &color);
    // 设置全局对话文字样式（姓名/对白）。
    void setGlobalDialogueTextStyle(int nameFontSize,
                                    const QString &nameFontColor,
                                    int textFontSize,
                                    const QString &textFontColor);

signals:
    // 用户点击或按键请求推进到下一句。
    void advanceRequested();
    // 当前句文本显示完成（打字机结束或跳过）。
    void textDisplayFinished();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onTypewriterTextUpdated(const QString &text);
    void onTypewriterFinished();
    void onAutoPlayTimeout();

private:
    void setupUiLayer();
    void setBackgroundImage(const QString &imagePath);
    void updateTextLayout();
    void updateViewTransform();
    void handleAdvanceInput();

    static constexpr int kVirtualWidth = 1280;
    static constexpr int kVirtualHeight = 720;

    QGraphicsScene m_scene;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
    QGraphicsPixmapItem *m_portraitItem = nullptr;
    QGraphicsRectItem *m_dialogueBoxItem = nullptr;
    QGraphicsTextItem *m_speakerNameItem = nullptr;
    QGraphicsTextItem *m_dialogueTextItem = nullptr;

    TypewriterEffect m_typewriter;
    QTimer m_autoPlayTimer;

    QString m_fullText;
    QString m_resourceBaseDir;
    int m_globalNameFontSize = 14;
    QString m_globalNameFontColor = "#FFFFFF";
    int m_globalTextFontSize = 12;
    QString m_globalTextFontColor = "#FFFFFF";
    int m_typingSpeedMs = 20;
    bool m_clickToAdvance = true;
    bool m_autoPlayEnabled = false;
    int m_autoPlayDelayMs = 1500;
};

#endif // GAMECANVAS_H
