#include "ProblemDetailsDialog.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

ProblemDetailsDialog::ProblemDetailsDialog(const Problem &problem, bool solved, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Problem %1 - %2").arg(problem.num).arg(problem.title));
    setModal(false);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    const QString judge = problem.status == 2 ? QStringLiteral("special judge")
                         : problem.status == 1 ? QStringLiteral("normal")
                                               : QStringLiteral("unavailable");

    form->addRow(QStringLiteral("Number"), new QLabel(QString::number(problem.num)));
    form->addRow(QStringLiteral("Title"), new QLabel(problem.title));
    form->addRow(QStringLiteral("Judging"), new QLabel(judge));
    form->addRow(QStringLiteral("Solved by you"), new QLabel(solved ? QStringLiteral("yes")
                                                                    : QStringLiteral("no")));
    form->addRow(QStringLiteral("Distinct accepted users"), new QLabel(QString::number(problem.dacu)));
    form->addRow(QStringLiteral("Accepted submissions"), new QLabel(QString::number(problem.ac)));
    form->addRow(QStringLiteral("Wrong answer"), new QLabel(QString::number(problem.wa)));
    form->addRow(QStringLiteral("Time limit exceeded"), new QLabel(QString::number(problem.tle)));
    form->addRow(QStringLiteral("Memory limit exceeded"), new QLabel(QString::number(problem.mle)));
    form->addRow(QStringLiteral("Runtime error"), new QLabel(QString::number(problem.re)));
    form->addRow(QStringLiteral("Compile error"), new QLabel(QString::number(problem.ce)));
    form->addRow(QStringLiteral("Presentation error"), new QLabel(QString::number(problem.pe)));
    form->addRow(QStringLiteral("Output limit exceeded"), new QLabel(QString::number(problem.ole)));
    form->addRow(QStringLiteral("Run time limit"),
                 new QLabel(QStringLiteral("%1 ms").arg(problem.rtl)));
    form->addRow(QStringLiteral("Best accepted runtime"),
                 new QLabel(QStringLiteral("%1 ms").arg(problem.mrun)));
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto *openButton =
        buttons->addButton(QStringLiteral("Open on onlinejudge.org"), QDialogButtonBox::ActionRole);
    connect(openButton, &QPushButton::clicked, this, [problem]() {
        QDesktopServices::openUrl(QUrl(QStringLiteral(
            "https://onlinejudge.org/index.php?option=com_onlinejudge&Itemid=8&page=show_problem&problem=%1")
                                          .arg(problem.pid)));
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    resize(430, 460);
}
