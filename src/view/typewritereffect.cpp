#include "typewritereffect.h"

TypewriterEffect::TypewriterEffect(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, &TypewriterEffect::onTick);
}

void TypewriterEffect::setSpeed(int msPerChar)
{
    m_msPerChar = qMax(0, msPerChar);
    if (m_running && m_msPerChar > 0) {
        m_timer.setInterval(m_msPerChar);
    }
}

void TypewriterEffect::start(const QString &fullText)
{
    m_timer.stop();
    m_running = false;

    m_fullText = fullText;
    m_currentLength = 0;

    if (m_msPerChar <= 0 || m_fullText.isEmpty()) {
        m_running = false;
        m_timer.stop();
        emit textUpdated(m_fullText);
        emit finished();
        return;
    }

    m_running = true;
    m_timer.start(m_msPerChar);
    emit textUpdated(QString());
}

void TypewriterEffect::cancel()
{
    m_timer.stop();
    m_running = false;
    m_currentLength = 0;
    m_fullText.clear();
}

void TypewriterEffect::skipToEnd()
{
    if (!m_running) {
        return;
    }

    m_running = false;
    m_timer.stop();
    emit textUpdated(m_fullText);
    emit finished();
}

bool TypewriterEffect::isRunning() const
{
    return m_running;
}

void TypewriterEffect::onTick()
{
    if (!m_running) {
        return;
    }

    ++m_currentLength;
    if (m_currentLength >= m_fullText.size()) {
        m_running = false;
        m_timer.stop();
        emit textUpdated(m_fullText);
        emit finished();
        return;
    }

    emit textUpdated(m_fullText.left(m_currentLength));
}
