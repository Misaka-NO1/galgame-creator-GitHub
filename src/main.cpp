#include "mainwindow.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class StartDialog : public QDialog
{
public:
    explicit StartDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("启动");
        resize(460, 160);

        auto *title = new QLabel("请选择：新建项目 或 打开项目", this);
        m_pathEdit = new QLineEdit(this);
        m_pathEdit->setReadOnly(true);

        auto *newButton = new QPushButton("新建项目", this);
        auto *openButton = new QPushButton("打开项目", this);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

        auto *actionRow = new QHBoxLayout();
        actionRow->addWidget(newButton);
        actionRow->addWidget(openButton);

        auto *layout = new QVBoxLayout(this);
        layout->addWidget(title);
        layout->addLayout(actionRow);
        layout->addWidget(m_pathEdit);
        layout->addWidget(buttons);

        connect(newButton, &QPushButton::clicked, this, [this]() {
            const QString parentDir = QFileDialog::getExistingDirectory(this, "选择项目目录");
            if (parentDir.isEmpty()) {
                return;
            }
            const QString name = QInputDialog::getText(this, "项目名称", "请输入项目名称");
            if (name.trimmed().isEmpty()) {
                return;
            }
            m_selectedPath = QDir(parentDir).filePath(QString("%1.galproj").arg(name.trimmed()));
            m_pathEdit->setText(m_selectedPath);
        });

        connect(openButton, &QPushButton::clicked, this, [this]() {
            const QString projectDir = QFileDialog::getExistingDirectory(this, "选择项目目录");
            if (projectDir.isEmpty()) {
                return;
            }
            m_selectedPath = projectDir;
            m_pathEdit->setText(m_selectedPath);
        });

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (m_selectedPath.isEmpty()) {
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QString selectedPath() const
    {
        return m_selectedPath;
    }

private:
    QString m_selectedPath;
    QLineEdit *m_pathEdit = nullptr;
};

#if !defined(CANVAS_TEST) && !defined(MODEL_TEST)
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    StartDialog dlg;
    if (dlg.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow w(dlg.selectedPath());
    w.show();
    return a.exec();
}
#endif
