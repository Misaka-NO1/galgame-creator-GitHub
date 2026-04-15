#include "project.h"

#include <QDebug>
#include <QDir>

#if defined(MODEL_TEST) && !defined(CANVAS_TEST)
int main()
{
    Project p;
    p.setName(QString::fromUtf8("测试项目"));

    // 创建 1 个角色并加入项目。
    auto *character = new Character();
    character->setId("char1");
    character->setName(QString::fromUtf8("女主角"));
    character->setPortraitPath("resources/characters/char1_portrait.png");
    character->setAvatarPath("resources/characters/char1_avatar.png");
    character->setVoicePrefix("char1_");
    character->setPosition(Character::Position::Center);
    p.addCharacter(character);

    // 创建 1 个背景，覆盖第 1~5 句对话。
    auto *background = new Background();
    background->setId("bg1");
    background->setImagePath("resources/backgrounds/school.jpg");
    background->setStartDialogueId(1);
    background->setEndDialogueId(5);
    background->setBgmPath("resources/voices/school_bgm.ogg");
    p.addBackground(background);

    // 创建 3 句连续对话。
    for (int i = 1; i <= 3; ++i) {
        auto *dialogue = new Dialogue();
        dialogue->setId(i);
        dialogue->setCharacterId("char1");
        dialogue->setText(QString::fromUtf8("测试台词 %1").arg(i));
        dialogue->setVoiceFile(QString("%1.wav").arg(i, 3, 10, QLatin1Char('0')));
        dialogue->setSpecialEffect(Dialogue::SpecialEffect::None);
        p.addDialogue(dialogue);
    }

    // 保存到临时目录并生成 project.json。
    const QString testDirPath = QDir::cleanPath(QDir::temp().filePath("test.galproj"));
    const bool saveOk = p.saveToFile(testDirPath);
    qDebug() << "[MODEL_TEST] save ok =" << saveOk << ", path =" << testDirPath;

    // 重新加载并验证核心数据完整性。
    Project *loaded = Project::loadFromFile(testDirPath);
    if (!loaded) {
        qDebug() << "[MODEL_TEST] load failed";
        return 1;
    }

    qDebug() << "[MODEL_TEST] loaded name =" << loaded->name();
    qDebug() << "[MODEL_TEST] character count =" << loaded->characters().size();
    qDebug() << "[MODEL_TEST] background count =" << loaded->backgrounds().size();
    qDebug() << "[MODEL_TEST] dialogue count =" << loaded->dialogues().size();

    // 打印每句对话的背景匹配结果。
    for (Dialogue *dialogue : loaded->dialogues()) {
        const Background *matched = loaded->getBackgroundForDialogue(dialogue->id());
        qDebug() << "[MODEL_TEST] dialogue" << dialogue->id()
                 << "-> background =" << (matched ? matched->id() : "NONE");
    }

    delete loaded;
    return 0;
}
#endif
