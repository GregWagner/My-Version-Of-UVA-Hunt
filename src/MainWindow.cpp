#include "MainWindow.h"

#include <algorithm>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableView>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "LiveEventTableModel.h"
#include "ProblemDetailsDialog.h"
#include "ProblemFilterProxyModel.h"
#include "ProblemTableModel.h"
#include "RanklistTableModel.h"
#include "SubmissionTableModel.h"
#include "UhuntClient.h"

namespace {

// Default UVa username used when no account has been saved in QSettings.
constexpr const char kDefaultUsername[] = "gmwRetired";

QLabel *valueLabel(const QString &text = QStringLiteral("-"))
{
    auto *label = new QLabel(text);
    QFont f = label->font();
    f.setBold(true);
    label->setFont(f);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

// A frame with a classic raised, beveled 3D border. The bevel is painted
// explicitly (using palette roles) so it looks the same under every style --
// some platform styles draw QFrame::StyledPanel/Raised as a flat line.
class BevelFrame : public QFrame
{
public:
    using QFrame::QFrame;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.fillRect(rect(), palette().color(QPalette::Button));

        const int w = width();
        const int h = height();
        if (w < 2 || h < 2)
            return;

        // Outer bevel: light on top/left, dark on bottom/right.
        p.setPen(palette().color(QPalette::Light));
        p.drawLine(0, 0, w - 2, 0);
        p.drawLine(0, 0, 0, h - 2);
        p.setPen(palette().color(QPalette::Dark));
        p.drawLine(0, h - 1, w - 1, h - 1);
        p.drawLine(w - 1, 0, w - 1, h - 1);

        // Inner bevel: mid-light on top/left, shadow on bottom/right.
        p.setPen(palette().color(QPalette::Midlight));
        p.drawLine(1, 1, w - 3, 1);
        p.drawLine(1, 1, 1, h - 3);
        p.setPen(palette().color(QPalette::Shadow));
        p.drawLine(1, h - 2, w - 2, h - 2);
        p.drawLine(w - 2, 1, w - 2, h - 2);
    }
};

}  // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("hunt - UVa Hunting"));
    resize(1150, 720);

    m_client = new UhuntClient(this);

    // Toolbar: username hunt control.
    auto *toolbar = addToolBar(QStringLiteral("Hunt"));
    toolbar->setMovable(false);
    toolbar->addWidget(new QLabel(QStringLiteral(" UVa username: ")));
    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText(QStringLiteral("e.g. felix_halim"));
    m_userEdit->setClearButtonEnabled(true);
    m_userEdit->setMinimumWidth(220);
    toolbar->addWidget(m_userEdit);
    m_huntButton = new QPushButton(QStringLiteral("Sign in"));
    m_huntButton->setToolTip(QStringLiteral("Load this UVa user's profile and remember the account"));
    toolbar->addWidget(m_huntButton);
    toolbar->addSeparator();

    m_accountLabel = new QLabel;
    toolbar->addWidget(m_accountLabel);
    m_signOutAction = toolbar->addAction(QStringLiteral("Sign out"));
    connect(m_signOutAction, &QAction::triggered, this, &MainWindow::onSignOut);
    toolbar->addSeparator();

    auto *refreshAction = toolbar->addAction(QStringLiteral("Reload problems"));
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshProblems);

    m_tabs = new QTabWidget;
    m_tabs->addTab(buildDashboardTab(), QStringLiteral("Dashboard"));
    m_tabs->addTab(buildProblemsTab(), QStringLiteral("Problems"));
    m_tabs->addTab(buildSubmissionsTab(), QStringLiteral("My Submissions"));
    m_tabs->addTab(buildRanklistTab(), QStringLiteral("Ranklist"));
    m_tabs->addTab(buildLiveTab(), QStringLiteral("Live"));
    setCentralWidget(m_tabs);

    buildMenus();

    connect(m_huntButton, &QPushButton::clicked, this, &MainWindow::onHunt);
    connect(m_userEdit, &QLineEdit::returnPressed, this, &MainWindow::onHunt);

    connect(m_client, &UhuntClient::userIdResolved, this, &MainWindow::onUserIdResolved);
    connect(m_client, &UhuntClient::problemsFetched, this, &MainWindow::onProblemsFetched);
    connect(m_client, &UhuntClient::userSubmissionsFetched, this, &MainWindow::onUserSubmissionsFetched);
    connect(m_client, &UhuntClient::ranklistFetched, this, &MainWindow::onRanklistFetched);
    connect(m_client, &UhuntClient::cpBookFetched, this, &MainWindow::onCpBookFetched);
    connect(m_client, &UhuntClient::liveEventsReceived, this, &MainWindow::onLiveEvents);
    connect(m_client, &UhuntClient::errorOccurred, this, &MainWindow::onError);

    updateAccountUi();

    // Default account: the saved username if present, otherwise the built-in
    // default. Only the username is ever stored (never a password).
    QString account = QSettings().value(QStringLiteral("account/username")).toString();
    if (account.isEmpty())
        account = QString::fromLatin1(kDefaultUsername);
    if (!account.isEmpty()) {
        m_userEdit->setText(account);
        setStatus(QStringLiteral("Signing in as %1...").arg(account));
        QTimer::singleShot(0, this, &MainWindow::onHunt);
    }

    setStatus(QStringLiteral("Loading problem list from uhunt..."));
    m_client->fetchProblems();
    // The 4th edition is not served by the API (only 1-3 are), so it ships
    // bundled with the app as a Qt resource.
    m_client->loadCpBook(QStringLiteral(":/resources/cpbook4.json"));
}

MainWindow::~MainWindow() = default;

void MainWindow::configureTable(QTableView *view)
{
    view->setSortingEnabled(true);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->verticalHeader()->setVisible(false);
    view->horizontalHeader()->setStretchLastSection(true);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

QWidget *MainWindow::buildDashboardTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *profileBox = new QGroupBox;
    auto *profileLayout = new QVBoxLayout(profileBox);

    // Wrap each stat in a raised, beveled frame for a boxed 3D look.
    const auto makeFieldBox = [](const QString &caption, QLabel *value) {
        auto *frame = new BevelFrame;
        frame->setFrameShape(QFrame::NoFrame);
        auto *box = new QVBoxLayout(frame);
        box->setContentsMargins(14, 9, 14, 9);
        auto *cap = new QLabel(caption);
        cap->setAlignment(Qt::AlignHCenter);
        value->setAlignment(Qt::AlignHCenter);
        box->addWidget(cap);
        box->addWidget(value);
        return frame;
    };

    // Global ranking is the headline stat: a large, centred number.
    m_rankValue = new QLabel(QStringLiteral("-"));
    {
        QFont f = m_rankValue->font();
        f.setPointSize(48);
        f.setBold(true);
        m_rankValue->setFont(f);
    }
    m_rankValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    profileLayout->addWidget(makeFieldBox(QStringLiteral("Global Ranking"), m_rankValue));

    m_solvedValue = valueLabel();
    m_subsValue = valueLabel();
    for (QLabel *value : { m_solvedValue, m_subsValue }) {
        QFont f = value->font();
        f.setPointSize(f.pointSize() + 5);
        value->setFont(f);
    }
    auto *statRow = new QHBoxLayout;
    statRow->addWidget(makeFieldBox(QStringLiteral("Solved Problems"), m_solvedValue));
    statRow->addWidget(makeFieldBox(QStringLiteral("Total Submissions"), m_subsValue));
    profileLayout->addLayout(statRow);

    layout->addWidget(profileBox);

    auto *recBox = new QGroupBox(QStringLiteral("Competitive Programming Exercises"));
    auto *recLayout = new QVBoxLayout(recBox);

    auto *bookNote = new QLabel(
        QStringLiteral("Competitive Programming 4 (CP4) book exercises. "
                       "\u2605 marks the starred, must-try (harder) problems."));
    bookNote->setWordWrap(true);
    recLayout->addWidget(bookNote);

    m_cpBookTree = new QTreeWidget;
    m_cpBookTree->setColumnCount(5);
    m_cpBookTree->setHeaderLabels(
        { QStringLiteral("Book Chapter / Section / Topic / Problem"),
          QStringLiteral("Solved"), QStringLiteral("Total"), QStringLiteral("%"),
          QString() });
    // Right-align the numeric progress columns (headers default to centred
    // under some styles).
    for (int col = 1; col <= 3; ++col)
        m_cpBookTree->headerItem()->setTextAlignment(col, Qt::AlignRight | Qt::AlignVCenter);
    m_cpBookTree->setRootIsDecorated(true);
    m_cpBookTree->setUniformRowHeights(false);
    m_cpBookTree->setAlternatingRowColors(false);
    m_cpBookTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int col = 1; col <= 4; ++col)
        m_cpBookTree->header()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    connect(m_cpBookTree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int) {
                const int pid = item->data(0, Qt::UserRole).toInt();
                if (pid <= 0)
                    return;
                if (const Problem *p = m_catalog.problem(pid))
                    showProblemDetails(*p, m_solvedPids.contains(p->pid));
            });
    recLayout->addWidget(m_cpBookTree, 1);
    layout->addWidget(recBox, 1);

    return page;
}

QWidget *MainWindow::buildProblemsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *controls = new QHBoxLayout;
    m_problemSearch = new QLineEdit;
    m_problemSearch->setPlaceholderText(QStringLiteral("Search problem number or title..."));
    m_problemSearch->setClearButtonEnabled(true);
    controls->addWidget(m_problemSearch, 1);

    m_statusFilter = new QComboBox;
    m_statusFilter->addItem(QStringLiteral("All problems"), int(ProblemFilterProxyModel::All));
    m_statusFilter->addItem(QStringLiteral("Solved"), int(ProblemFilterProxyModel::Solved));
    m_statusFilter->addItem(QStringLiteral("Unsolved"), int(ProblemFilterProxyModel::Unsolved));
    controls->addWidget(m_statusFilter);

    m_categoryFilter = new QComboBox;
    m_categoryFilter->addItem(QStringLiteral("All categories"));
    m_categoryFilter->setMinimumWidth(320);
    controls->addWidget(m_categoryFilter);

    m_problemCount = new QLabel(QStringLiteral("0 problems"));
    controls->addWidget(m_problemCount);
    layout->addLayout(controls);

    m_problemView = new QTableView;
    m_problemModel = new ProblemTableModel(this);
    m_problemProxy = new ProblemFilterProxyModel(this);
    m_problemProxy->setSourceModel(m_problemModel);
    m_problemView->setModel(m_problemProxy);
    configureTable(m_problemView);
    m_problemView->horizontalHeader()->setSectionResizeMode(ProblemTableModel::ColTitle,
                                                             QHeaderView::Stretch);
    m_problemView->sortByColumn(ProblemTableModel::ColNumber, Qt::AscendingOrder);
    layout->addWidget(m_problemView, 1);

    connect(m_problemSearch, &QLineEdit::textChanged, this, &MainWindow::onProblemSearchChanged);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onStatusFilterChanged);
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onCategoryChanged);
    connect(m_problemView, &QTableView::doubleClicked, this, &MainWindow::onProblemActivated);
    connect(m_problemProxy, &QAbstractItemModel::modelReset, this, &MainWindow::updateProblemCount);
    connect(m_problemProxy, &QAbstractItemModel::layoutChanged, this,
            &MainWindow::updateProblemCount);

    return page;
}

QWidget *MainWindow::buildSubmissionsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *controls = new QHBoxLayout;
    controls->addWidget(new QLabel(QStringLiteral("Show:")));
    m_subsCount = new QComboBox;
    m_subsCount->addItem(QStringLiteral("All submissions"), 0);
    m_subsCount->addItem(QStringLiteral("Last 10"), 10);
    m_subsCount->addItem(QStringLiteral("Last 25"), 25);
    m_subsCount->addItem(QStringLiteral("Last 50"), 50);
    m_subsCount->addItem(QStringLiteral("Last 100"), 100);
    m_subsCount->addItem(QStringLiteral("Last 500"), 500);
    controls->addWidget(m_subsCount);
    controls->addStretch(1);
    layout->addLayout(controls);

    m_subView = new QTableView;
    m_subModel = new SubmissionTableModel(&m_catalog, this);
    m_subView->setModel(m_subModel);
    configureTable(m_subView);
    m_subView->horizontalHeader()->setSectionResizeMode(SubmissionTableModel::ColProblem,
                                                        QHeaderView::Stretch);
    layout->addWidget(m_subView, 1);

    connect(m_subsCount, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (m_currentUid == 0)
            return;
        const int count = m_subsCount->currentData().toInt();
        if (count == 0)
            m_client->fetchUserSubmissions(m_currentUid);
        else
            m_client->fetchUserSubmissionsLast(m_currentUid, count);
    });

    return page;
}

QWidget *MainWindow::buildRanklistTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *controls = new QHBoxLayout;
    controls->addWidget(new QLabel(QStringLiteral("View:")));
    m_rankMode = new QComboBox;
    m_rankMode->addItem(QStringLiteral("Around me (5 above / 5 below)"), 0);
    m_rankMode->addItem(QStringLiteral("Around me (25 above / 25 below)"), 1);
    m_rankMode->addItem(QStringLiteral("Global top 100"), 2);
    m_rankMode->addItem(QStringLiteral("Global top 500"), 3);
    controls->addWidget(m_rankMode);
    controls->addStretch(1);
    layout->addLayout(controls);

    m_rankView = new QTableView;
    m_rankModel = new RanklistTableModel(this);
    m_rankView->setModel(m_rankModel);
    configureTable(m_rankView);
    m_rankView->horizontalHeader()->setSectionResizeMode(RanklistTableModel::ColName,
                                                        QHeaderView::Stretch);
    layout->addWidget(m_rankView, 1);

    connect(m_rankMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        const int mode = m_rankMode->currentData().toInt();
        if (mode == 2) {
            m_client->fetchGlobalRank(1, 100);
        } else if (mode == 3) {
            m_client->fetchGlobalRank(1, 500);
        } else if (m_currentUid != 0) {
            const int n = mode == 1 ? 25 : 5;
            m_client->fetchRanklist(m_currentUid, n, n);
        }
    });

    return page;
}

QWidget *MainWindow::buildLiveTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *controls = new QHBoxLayout;
    m_liveToggle = new QPushButton(QStringLiteral("Start live feed"));
    m_liveToggle->setCheckable(true);
    controls->addWidget(m_liveToggle);
    m_liveStatus = new QLabel(QStringLiteral("Stopped"));
    controls->addWidget(m_liveStatus);
    controls->addStretch(1);
    layout->addLayout(controls);

    m_liveView = new QTableView;
    m_liveModel = new LiveEventTableModel(&m_catalog, this);
    m_liveView->setModel(m_liveModel);
    configureTable(m_liveView);
    m_liveView->horizontalHeader()->setSectionResizeMode(LiveEventTableModel::ColProblem,
                                                        QHeaderView::Stretch);
    layout->addWidget(m_liveView, 1);

    connect(m_liveToggle, &QPushButton::toggled, this, &MainWindow::onLiveToggled);

    return page;
}

void MainWindow::buildMenus()
{
    auto *accountMenu = menuBar()->addMenu(QStringLiteral("&Account"));
    auto *signIn = accountMenu->addAction(QStringLiteral("&Sign in..."));
    signIn->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    connect(signIn, &QAction::triggered, this, [this]() {
        m_userEdit->setFocus();
        m_userEdit->selectAll();
    });
    auto *signOut = accountMenu->addAction(QStringLiteral("Sign &out"));
    connect(signOut, &QAction::triggered, this, &MainWindow::onSignOut);

    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    auto *refresh = fileMenu->addAction(QStringLiteral("&Reload problems"));
    connect(refresh, &QAction::triggered, this, &MainWindow::onRefreshProblems);
    fileMenu->addSeparator();
    auto *quit = fileMenu->addAction(QStringLiteral("&Quit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);

    auto *helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    auto *about = helpMenu->addAction(QStringLiteral("&About"));
    connect(about, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, QStringLiteral("About hunt"),
                           QStringLiteral(
                               "<b>hunt</b> - a Qt6 desktop client for the uhunt API.<br><br>"
                               "Data comes from <a href=\"https://uhunt.onlinejudge.org/api\">"
                               "uhunt.onlinejudge.org/api</a>, the statistics service for the "
                               "UVa Online Judge.<br><br>"
                               "Enter a UVa username to load a profile, browse problems, inspect "
                               "submissions, view ranklists and follow the live submission feed."));
    });
}

void MainWindow::onHunt()
{
    const QString uname = m_userEdit->text().trimmed();
    if (uname.isEmpty()) {
        setStatus(QStringLiteral("Enter a UVa username first."));
        return;
    }
    m_huntButton->setEnabled(false);
    setStatus(QStringLiteral("Looking up '%1'...").arg(uname));
    m_client->fetchUserId(uname);
}

void MainWindow::onRefreshProblems()
{
    setStatus(QStringLiteral("Reloading problem list..."));
    m_client->fetchProblems();
}

void MainWindow::onUserIdResolved(const QString &uname, int uid)
{
    m_huntButton->setEnabled(true);
    if (uid <= 0) {
        setStatus(QStringLiteral("No such UVa user: '%1'").arg(uname));
        QMessageBox::warning(this, QStringLiteral("User not found"),
                             QStringLiteral("uhunt does not know the user '%1'.").arg(uname));
        return;
    }
    m_currentUid = uid;
    m_currentUname = uname;
    QSettings().setValue(QStringLiteral("account/username"), uname);
    updateAccountUi();
    m_rankValue->setText(QStringLiteral("..."));
    setStatus(QStringLiteral("Loading submissions and ranklist for %1 (uid %2)...").arg(uname).arg(uid));
    m_client->fetchUserSubmissions(uid);
    m_client->fetchRanklist(uid, 5, 5);
    m_tabs->setCurrentIndex(0);
}

void MainWindow::onProblemsFetched(const QVector<Problem> &problems)
{
    m_catalog.problems = problems;
    m_catalog.rebuild();
    m_problemModel->setProblems(problems);
    m_problemModel->setSolvedPids(m_solvedPids);
    rebuildCpBookTree();
    updateProblemCount();
    setStatus(QStringLiteral("Loaded %1 problems.").arg(problems.size()));
}

void MainWindow::onUserSubmissionsFetched(int uid, const QVector<Submission> &subs)
{
    if (uid != m_currentUid)
        return;

    m_subModel->setSubmissions(subs);

    m_solvedPids.clear();
    m_triedPids.clear();
    for (const Submission &s : subs) {
        m_triedPids.insert(s.pid);
        if (s.ver == 90)
            m_solvedPids.insert(s.pid);
    }
    m_problemModel->setSolvedPids(m_solvedPids);
    rebuildCpBookTree();

    m_solvedValue->setText(QStringLiteral("%1").arg(m_solvedPids.size()));
    m_subsValue->setText(QStringLiteral("%1").arg(subs.size()));
    setStatus(QStringLiteral("Loaded %1 submissions for %2.").arg(subs.size()).arg(m_currentUname));
}

void MainWindow::onRanklistFetched(const QVector<UserRank> &list)
{
    m_rankModel->setRanks(list);
    for (const UserRank &r : list) {
        if (r.userid == m_currentUid) {
            m_rankValue->setText(QStringLiteral("%1").arg(r.rank));
            break;
        }
    }
}

void MainWindow::onCpBookFetched(const QVector<Category> &categories)
{
    m_categories = categories;
    m_categoryFilter->blockSignals(true);
    m_categoryFilter->clear();
    m_categoryFilter->addItem(QStringLiteral("All categories"));
    for (const Category &c : categories)
        m_categoryFilter->addItem(c.path);
    m_categoryFilter->blockSignals(false);
    rebuildCpBookTree();
}

void MainWindow::onLiveEvents(const QVector<LiveEvent> &events, qint64 lastId)
{
    m_liveModel->addEvents(events);
    m_liveStatus->setText(QStringLiteral("Live - %1 events, last id %2")
                              .arg(m_liveModel->rowCount())
                              .arg(lastId));
}

void MainWindow::onLiveToggled(bool on)
{
    if (on) {
        m_liveModel->clear();
        m_liveToggle->setText(QStringLiteral("Stop live feed"));
        m_liveStatus->setText(QStringLiteral("Starting..."));
        m_client->startLivePolling(0);
    } else {
        m_client->stopLivePolling();
        m_liveToggle->setText(QStringLiteral("Start live feed"));
        m_liveStatus->setText(QStringLiteral("Stopped"));
    }
}

void MainWindow::onProblemSearchChanged()
{
    m_problemProxy->setSearchText(m_problemSearch->text());
    updateProblemCount();
}

void MainWindow::onStatusFilterChanged(int index)
{
    m_problemProxy->setStatusFilter(
        static_cast<ProblemFilterProxyModel::StatusFilter>(m_statusFilter->itemData(index).toInt()));
    updateProblemCount();
}

void MainWindow::onCategoryChanged(int index)
{
    QSet<int> nums;
    if (index > 0 && index - 1 < m_categories.size()) {
        for (int n : m_categories.at(index - 1).nums)
            nums.insert(n);
    }
    m_problemProxy->setCategoryNumbers(nums);
    updateProblemCount();
}

void MainWindow::onProblemActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    const QModelIndex sourceIndex = m_problemProxy->mapToSource(index);
    const Problem *p = m_problemModel->problemAt(sourceIndex.row());
    if (!p)
        return;
    showProblemDetails(*p, m_solvedPids.contains(p->pid));
}

void MainWindow::showProblemDetails(const Problem &problem, bool solved)
{
    auto *dialog = new ProblemDetailsDialog(problem, solved, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::onError(const QString &context, const QString &message)
{
    setStatus(QStringLiteral("%1: %2").arg(context, message));
}

namespace {

// Fills a book exercise node's Solved / Total / % columns (indices 1-3), using
// "-" when it has no catalogue problems.
void setProgressColumns(QTreeWidgetItem *item, int solved, int total)
{
    for (int col = 1; col <= 3; ++col)
        item->setTextAlignment(col, Qt::AlignRight | Qt::AlignVCenter);
    if (total <= 0) {
        item->setText(1, QStringLiteral("-"));
        item->setText(2, QStringLiteral("-"));
        item->setText(3, QStringLiteral("-"));
        return;
    }
    item->setText(1, QString::number(solved));
    item->setText(2, QString::number(total));
    item->setText(3, QStringLiteral("%1%").arg(solved * 100 / total));
}

}  // namespace

void MainWindow::rebuildCpBookTree()
{
    if (!m_cpBookTree)
        return;

    // QTreeWidget::clear() deletes the items but *not* the widgets installed
    // with setItemWidget(), so delete the per-row "Start" buttons ourselves
    // first — otherwise they leak and accumulate on every rebuild.
    auto deleteStartButtons = [this](auto &&self, QTreeWidgetItem *item) -> void {
        for (int i = 0; i < item->childCount(); ++i)
            self(self, item->child(i));
        delete m_cpBookTree->itemWidget(item, 4);
    };
    for (int i = 0; i < m_cpBookTree->topLevelItemCount(); ++i)
        deleteStartButtons(deleteStartButtons, m_cpBookTree->topLevelItem(i));

    m_cpBookTree->clear();
    if (m_categories.isEmpty() || m_catalog.problems.isEmpty())
        return;

    struct Agg {
        int solvedAll = 0, totalAll = 0;
    };

    // Adds the (currently inert) "Start" button to a row's last column. It is
    // wired up to a real action later; the problem id is stashed on the button
    // for that purpose (0 for chapter/section/topic rows).
    auto addStartButton = [this](QTreeWidgetItem *item, int pid) {
        auto *button = new QPushButton(QStringLiteral("Start"));
        button->setCursor(Qt::PointingHandCursor);
        button->setProperty("pid", pid);

        // Compact font + tight padding so the button is smaller than the
        // default, and a distinct green fill so it stands out from the rows.
        QFont f = button->font();
        if (f.pointSizeF() > 0)
            f.setPointSizeF(f.pointSizeF() - 2.0);
        button->setFont(f);
        button->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #2e7d32;"
            "  color: #ffffff;"
            "  border: 1px solid #1b5e20;"
            "  border-radius: 3px;"
            "  padding: 0px 6px;"
            "}"
            "QPushButton:hover { background-color: #388e3c; }"
            "QPushButton:pressed { background-color: #1b5e20; }"));
        button->setFixedHeight(button->fontMetrics().height() + 4);

        item->setSizeHint(4, button->sizeHint());
        m_cpBookTree->setItemWidget(item, 4, button);
    };

    QHash<QString, QTreeWidgetItem *> chapterItems;
    QHash<QString, QTreeWidgetItem *> sectionItems;
    QHash<QTreeWidgetItem *, Agg> aggs;
    const QColor solvedColor(0, 128, 0);
    const QColor triedColor(196, 120, 0);
    int chapterNumber = 0;

    for (const Category &c : m_categories) {
        const QStringList parts = c.path.split(QStringLiteral(" / "));
        const QString chapterName = parts.value(0);
        const QString sectionName = parts.value(1);
        const QString topicName = parts.value(2);

        QTreeWidgetItem *chapter = chapterItems.value(chapterName);
        if (!chapter) {
            chapter = new QTreeWidgetItem(m_cpBookTree);
            chapter->setText(0, QStringLiteral("%1. %2").arg(++chapterNumber).arg(chapterName));
            QFont f = chapter->font(0);
            f.setBold(true);
            chapter->setFont(0, f);
            chapterItems.insert(chapterName, chapter);
            aggs.insert(chapter, Agg{});
            addStartButton(chapter, 0);
        }
        const QString sectionKey = chapterName + QLatin1Char('\n') + sectionName;
        QTreeWidgetItem *section = sectionItems.value(sectionKey);
        if (!section) {
            section = new QTreeWidgetItem(chapter);
            section->setText(0, sectionName);
            sectionItems.insert(sectionKey, section);
            aggs.insert(section, Agg{});
            addStartButton(section, 0);
        }
        QTreeWidgetItem *topic = new QTreeWidgetItem(section);
        topic->setText(0, topicName);
        Agg topicAgg;
        aggs.insert(topic, topicAgg);
        addStartButton(topic, 0);

        // Determine which node aggregates this category contributes to.
        auto addTo = [&](QTreeWidgetItem *item, bool solved) {
            Agg &a = aggs[item];
            ++a.totalAll;
            if (solved)
                ++a.solvedAll;
        };

        for (int num : c.nums) {
            const Problem *p = m_catalog.problemByNum(num);
            if (!p)
                continue;
            const bool starred = c.starred.contains(num);
            const bool solved = m_solvedPids.contains(p->pid);
            const bool tried = m_triedPids.contains(p->pid);

            auto *leaf = new QTreeWidgetItem(topic);
            leaf->setText(0, QStringLiteral("%1. %2%3")
                                 .arg(num)
                                 .arg(starred ? QStringLiteral("\u2605 ") : QString())
                                 .arg(p->title));
            leaf->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            if (solved) {
                leaf->setText(1, QStringLiteral("\u2714"));
                leaf->setForeground(1, solvedColor);
            } else if (tried) {
                leaf->setText(1, QStringLiteral("tried"));
                leaf->setForeground(1, triedColor);
            }
            leaf->setData(0, Qt::UserRole, p->pid);
            leaf->setToolTip(0, QStringLiteral("%1. %2").arg(num).arg(p->title));
            addStartButton(leaf, p->pid);

            addTo(topic, solved);
            addTo(section, solved);
            addTo(chapter, solved);
        }

        const Agg &ta = aggs.value(topic);
        setProgressColumns(topic, ta.solvedAll, ta.totalAll);
    }

    for (auto it = chapterItems.constBegin(); it != chapterItems.constEnd(); ++it) {
        const Agg &a = aggs.value(it.value());
        setProgressColumns(it.value(), a.solvedAll, a.totalAll);
    }
    for (auto it = sectionItems.constBegin(); it != sectionItems.constEnd(); ++it) {
        const Agg &a = aggs.value(it.value());
        setProgressColumns(it.value(), a.solvedAll, a.totalAll);
    }
}

void MainWindow::updateProblemCount()
{
    if (!m_problemCount || !m_problemModel || !m_problemProxy)
        return;
    m_problemCount->setText(QStringLiteral("%1 / %2 problems")
                                .arg(m_problemProxy->rowCount())
                                .arg(m_problemModel->rowCount()));
}

void MainWindow::setStatus(const QString &text)
{
    statusBar()->showMessage(text);
}

void MainWindow::updateAccountUi()
{
    const bool signedIn = m_currentUid > 0;
    if (signedIn)
        m_accountLabel->setText(QStringLiteral("  Signed in as %1  ").arg(m_currentUname));
    else
        m_accountLabel->setText(QStringLiteral("  Not signed in  "));
    if (m_signOutAction)
        m_signOutAction->setEnabled(signedIn);
}

void MainWindow::onSignOut()
{
    QSettings settings;
    settings.remove(QStringLiteral("account/username"));

    m_currentUid = 0;
    m_currentUname.clear();
    m_solvedPids.clear();
    m_triedPids.clear();
    m_userEdit->setText(QString::fromLatin1(kDefaultUsername));

    m_subModel->clear();
    m_rankModel->setRanks({});
    m_problemModel->setSolvedPids(m_solvedPids);
    rebuildCpBookTree();
    updateProblemCount();

    for (QLabel *label : { m_rankValue, m_solvedValue, m_subsValue }) {
        label->setText(QStringLiteral("-"));
    }

    updateAccountUi();
    setStatus(QStringLiteral("Signed out."));
}
