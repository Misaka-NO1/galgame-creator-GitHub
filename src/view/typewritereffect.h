#ifndef TYPEWRITEREFFECT_H
#define TYPEWRITEREFFECT_H

#include <QObject>
#include <QTimer>
#include <QString>

class TypewriterEffect : public QObject
{
    Q_OBJECT

public:
    explicit TypewriterEffect(QObject *parent = nullptr);

    // 设置每个字符的显示间隔（毫秒），0 表示立即显示全文。
    void setSpeed(int msPerChar);
    // 开始按打字机效果显示文本。
    void start(const QString &fullText);
    // 静默停止当前打字机，不发任何文本/完成信号。
    void cancel();
    // 立即跳过到全文显示。
    void skipToEnd();
    // 返回当前是否仍在逐字播放。
    bool isRunning() const;

signals:
    // 每次文本更新时发出当前应显示内容。
    void textUpdated(const QString &text);
    // 文本完整显示后发出。
    void finished();

private slots:
    void onTick();

private:
    QTimer m_timer;
    QString m_fullText;
    int m_msPerChar = 20;
    int m_currentLength = 0;
    bool m_running = false;
};

#endif // TYPEWRITEREFFECT_H
