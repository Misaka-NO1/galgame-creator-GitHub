#include "dialogue.h"

namespace {

QString specialEffectToString(Dialogue::SpecialEffect effect)
{
    switch (effect) {
    case Dialogue::SpecialEffect::None:
        return "none";
    case Dialogue::SpecialEffect::Shake:
        return "shake";
    case Dialogue::SpecialEffect::Flash:
        return "flash";
    case Dialogue::SpecialEffect::Fade:
        return "fade";
    }
    return "none";
}

Dialogue::SpecialEffect specialEffectFromString(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == "shake") {
        return Dialogue::SpecialEffect::Shake;
    }
    if (normalized == "flash") {
        return Dialogue::SpecialEffect::Flash;
    }
    if (normalized == "fade") {
        return Dialogue::SpecialEffect::Fade;
    }
    return Dialogue::SpecialEffect::None;
}

} // namespace

Dialogue::Dialogue(QObject *parent)
    : QObject(parent)
{
}

int Dialogue::id() const
{
    return m_id;
}

void Dialogue::setId(int id)
{
    m_id = id;
}

QString Dialogue::characterId() const
{
    return m_characterId;
}

void Dialogue::setCharacterId(const QString &characterId)
{
    m_characterId = characterId;
}

QString Dialogue::text() const
{
    return m_text;
}

void Dialogue::setText(const QString &text)
{
    m_text = text;
}

QString Dialogue::voiceFile() const
{
    return m_voiceFile;
}

void Dialogue::setVoiceFile(const QString &voiceFile)
{
    m_voiceFile = voiceFile;
}

Dialogue::SpecialEffect Dialogue::specialEffect() const
{
    return m_specialEffect;
}

void Dialogue::setSpecialEffect(SpecialEffect specialEffect)
{
    m_specialEffect = specialEffect;
}

int Dialogue::nameFontSizeOverride() const
{
    return m_nameFontSizeOverride;
}

void Dialogue::setNameFontSizeOverride(int size)
{
    m_nameFontSizeOverride = size;
}

QString Dialogue::nameFontColorOverride() const
{
    return m_nameFontColorOverride;
}

void Dialogue::setNameFontColorOverride(const QString &color)
{
    m_nameFontColorOverride = color.trimmed();
}

int Dialogue::textFontSizeOverride() const
{
    return m_textFontSizeOverride;
}

void Dialogue::setTextFontSizeOverride(int size)
{
    m_textFontSizeOverride = size;
}

QString Dialogue::textFontColorOverride() const
{
    return m_textFontColorOverride;
}

void Dialogue::setTextFontColorOverride(const QString &color)
{
    m_textFontColorOverride = color.trimmed();
}

QJsonObject Dialogue::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["characterId"] = m_characterId;
    json["text"] = m_text;
    json["voiceFile"] = m_voiceFile;
    json["specialEffect"] = specialEffectToString(m_specialEffect);
    json["nameFontSizeOverride"] = m_nameFontSizeOverride;
    json["nameFontColorOverride"] = m_nameFontColorOverride;
    json["textFontSizeOverride"] = m_textFontSizeOverride;
    json["textFontColorOverride"] = m_textFontColorOverride;
    return json;
}

Dialogue *Dialogue::fromJson(const QJsonObject &json, QObject *parent)
{
    auto *dialogue = new Dialogue(parent);
    const QJsonValue idValue = json.value("id");
    int parsedId = 0;
    if (idValue.isDouble()) {
        parsedId = idValue.toInt(0);
    } else if (idValue.isString()) {
        parsedId = idValue.toString().toInt();
    }
    dialogue->setId(parsedId);
    dialogue->setCharacterId(json.value("characterId").toString());
    dialogue->setText(json.value("text").toString());
    dialogue->setVoiceFile(json.value("voiceFile").toString());
    dialogue->setSpecialEffect(specialEffectFromString(json.value("specialEffect").toString()));
    dialogue->setNameFontSizeOverride(json.value("nameFontSizeOverride").toInt(0));
    dialogue->setNameFontColorOverride(json.value("nameFontColorOverride").toString());
    dialogue->setTextFontSizeOverride(json.value("textFontSizeOverride").toInt(0));
    dialogue->setTextFontColorOverride(json.value("textFontColorOverride").toString());
    return dialogue;
}
