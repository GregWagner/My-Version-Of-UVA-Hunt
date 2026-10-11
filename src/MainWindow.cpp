#include "MainWindow.h"

#include <algorithm>
#include <cmath>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
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
    toolbar->addSeparator();

    // Refresh everything: reload the problem catalogue, the CP4 book and the
    // current account's submissions + ranklist from uhunt.
    auto *refreshAllAction = toolbar->addAction(style()->standardIcon(QStyle::SP_BrowserReload),
                                                QStringLiteral("Refresh"));
    refreshAllAction->setToolTip(QStringLiteral("Reload everything from uhunt (F5)"));
    connect(refreshAllAction, &QAction::triggered, this, &MainWindow::onRefreshAll);

    auto *refreshAction = toolbar->addAction(QStringLiteral("Reload problems"));
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshProblems);

    m_tabs = new QTabWidget;
    m_tabs->addTab(buildDashboardTab(), QStringLiteral("Dashboard"));
    m_tabs->addTab(buildProblemsTab(), QStringLiteral("Problems"));
    m_tabs->addTab(buildLevelsTab(), QStringLiteral("Levels"));
    m_tabs->addTab(buildSubmissionsTab(), QStringLiteral("My Submissions"));
    m_tabs->addTab(buildRanklistTab(), QStringLiteral("Ranklist"));
    m_tabs->addTab(buildLiveTab(), QStringLiteral("Live"));
    setCentralWidget(m_tabs);

    buildMenus();

    connect(m_userEdit, &QLineEdit::returnPressed, this, &MainWindow::onHunt);

    connect(m_client, &UhuntClient::userIdResolved, this, &MainWindow::onUserIdResolved);
    connect(m_client, &UhuntClient::problemsFetched, this, &MainWindow::onProblemsFetched);
    connect(m_client, &UhuntClient::userSubmissionsFetched, this, &MainWindow::onUserSubmissionsFetched);
    connect(m_client, &UhuntClient::ranklistFetched, this, &MainWindow::onRanklistFetched);
    connect(m_client, &UhuntClient::cpBookFetched, this, &MainWindow::onCpBookFetched);
    connect(m_client, &UhuntClient::liveEventsReceived, this, &MainWindow::onLiveEvents);
    connect(m_client, &UhuntClient::errorOccurred, this, &MainWindow::onError);

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

    // "Solved Problems" box: the count, the percentage of the loaded catalogue,
    // and a bar visualising that percentage.
    auto *solvedFrame = new BevelFrame;
    solvedFrame->setFrameShape(QFrame::NoFrame);
    auto *solvedBox = new QVBoxLayout(solvedFrame);
    solvedBox->setContentsMargins(14, 9, 14, 9);
    auto *solvedCap = new QLabel(QStringLiteral("Solved Problems"));
    solvedCap->setAlignment(Qt::AlignHCenter);
    m_solvedValue->setAlignment(Qt::AlignHCenter);
    solvedBox->addWidget(solvedCap);
    solvedBox->addWidget(m_solvedValue);
    m_solvedPercent = new QLabel(QStringLiteral("-"));
    m_solvedPercent->setAlignment(Qt::AlignHCenter);
    solvedBox->addWidget(m_solvedPercent);
    m_solvedBar = new QProgressBar;
    m_solvedBar->setRange(0, 100);
    m_solvedBar->setValue(0);
    m_solvedBar->setTextVisible(false);
    m_solvedBar->setFixedHeight(14);
    m_solvedBar->setStyleSheet(QStringLiteral(
        "QProgressBar { border: 1px solid #bdbdbd; border-radius: 3px; background: #eeeeee; }"
        "QProgressBar::chunk { background-color: #2e7d32; border-radius: 2px; }"));
    solvedBox->addWidget(m_solvedBar);

    auto *subsFrame = makeFieldBox(QStringLiteral("Total Submissions"), m_subsValue);
    // Top-align the shorter box's content so the two boxes look balanced.
    if (auto *subsBox = qobject_cast<QVBoxLayout *>(subsFrame->layout()))
        subsBox->addStretch(1);

    auto *statRow = new QHBoxLayout;
    statRow->addWidget(solvedFrame, 1);
    statRow->addWidget(subsFrame, 1);
    profileLayout->addLayout(statRow);

    layout->addWidget(profileBox);

    auto *recBox = new QGroupBox(QStringLiteral("Competitive Programming Exercises"));
    auto *recLayout = new QVBoxLayout(recBox);

    auto *bookNote = new QLabel(
        QStringLiteral("Competitive Programming 4 (CP4) book exercises. "
                       "\u2605 marks the starred, must-try (harder) problems."));
    bookNote->setWordWrap(true);
    recLayout->addWidget(bookNote);

    // Filter the tree by solved status. The two checkboxes are mutually
    // exclusive (checking one clears the other); with none checked, every
    // problem row is shown.
    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel(QStringLiteral("Show:")));
    m_cpFilterSolved = new QCheckBox(QStringLiteral("Only solved"));
    m_cpFilterUnsolved = new QCheckBox(QStringLiteral("Only unsolved"));
    filterRow->addWidget(m_cpFilterSolved);
    filterRow->addWidget(m_cpFilterUnsolved);
    filterRow->addSpacing(12);
    filterRow->addWidget(new QLabel(QStringLiteral("Level:")));
    m_cpFilterLevel = new QComboBox;
    m_cpFilterLevel->addItem(QStringLiteral("All levels"));
    for (int lvl = 0; lvl <= 10; ++lvl)
        m_cpFilterLevel->addItem(QStringLiteral("Level %1").arg(lvl));
    filterRow->addWidget(m_cpFilterLevel);
    filterRow->addStretch(1);
    recLayout->addLayout(filterRow);
    connect(m_cpFilterSolved, &QCheckBox::toggled, this, [this](bool on) {
        if (on && m_cpFilterUnsolved->isChecked()) {
            QSignalBlocker blocker(m_cpFilterUnsolved);
            m_cpFilterUnsolved->setChecked(false);
        }
        rebuildCpBookTree();
    });
    connect(m_cpFilterUnsolved, &QCheckBox::toggled, this, [this](bool on) {
        if (on && m_cpFilterSolved->isChecked()) {
            QSignalBlocker blocker(m_cpFilterSolved);
            m_cpFilterSolved->setChecked(false);
        }
        rebuildCpBookTree();
    });
    // Index 0 = "All levels"; index N (>0) selects level N-1.
    connect(m_cpFilterLevel, &QComboBox::currentIndexChanged, this,
            [this](int) { rebuildCpBookTree(); });

    m_cpBookTree = new QTreeWidget;
    m_cpBookTree->setColumnCount(6);
    m_cpBookTree->setHeaderLabels(
        { QStringLiteral("Book Chapter / Section / Topic / Problem"),
          QStringLiteral("Level"), QStringLiteral("Solved"), QStringLiteral("Total"),
          QStringLiteral("%"), QString() });
    // Right-align the numeric columns (headers default to centred under some
    // styles).
    for (int col = 1; col <= 4; ++col)
        m_cpBookTree->headerItem()->setTextAlignment(col, Qt::AlignRight | Qt::AlignVCenter);
    m_cpBookTree->setRootIsDecorated(true);
    m_cpBookTree->setUniformRowHeights(false);
    m_cpBookTree->setAlternatingRowColors(false);
    m_cpBookTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int col = 1; col <= 5; ++col)
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

QWidget *MainWindow::buildLevelsTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *note = new QLabel(
        QStringLiteral("All loaded problems grouped by uHunt level "
                       "(10 = hardest, 0 = easiest), derived from dacu. "
                       "The Chapter and Section columns show where each problem "
                       "sits in the CP book; the tree is grouped only by level."));
    note->setWordWrap(true);
    layout->addWidget(note);

    // Filter by solved status. The two checkboxes are mutually exclusive
    // (checking one clears the other); with none checked every problem shows.
    auto *controls = new QHBoxLayout;
    controls->addWidget(new QLabel(QStringLiteral("Show:")));
    m_levelsFilterSolved = new QCheckBox(QStringLiteral("Only solved"));
    m_levelsFilterUnsolved = new QCheckBox(QStringLiteral("Only unsolved"));
    controls->addWidget(m_levelsFilterSolved);
    controls->addWidget(m_levelsFilterUnsolved);
    controls->addStretch(1);
    layout->addLayout(controls);
    connect(m_levelsFilterSolved, &QCheckBox::toggled, this, [this](bool on) {
        if (on && m_levelsFilterUnsolved->isChecked()) {
            QSignalBlocker blocker(m_levelsFilterUnsolved);
            m_levelsFilterUnsolved->setChecked(false);
        }
        rebuildLevelsTree();
    });
    connect(m_levelsFilterUnsolved, &QCheckBox::toggled, this, [this](bool on) {
        if (on && m_levelsFilterSolved->isChecked()) {
            QSignalBlocker blocker(m_levelsFilterSolved);
            m_levelsFilterSolved->setChecked(false);
        }
        rebuildLevelsTree();
    });

    m_levelsTree = new QTreeWidget;
    m_levelsTree->setColumnCount(4);
    m_levelsTree->setHeaderLabels({ QStringLiteral("Problem"),
                                    QStringLiteral("Chapter"),
                                    QStringLiteral("Section"),
                                    QStringLiteral("Solved") });
    m_levelsTree->headerItem()->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
    m_levelsTree->setRootIsDecorated(true);
    m_levelsTree->setUniformRowHeights(true);
    m_levelsTree->setAlternatingRowColors(true);
    m_levelsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 4; ++column)
        m_levelsTree->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    connect(m_levelsTree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int) {
                const int pid = item->data(0, Qt::UserRole).toInt();
                if (pid <= 0)
                    return;
                if (const Problem *p = m_catalog.problem(pid))
                    showProblemDetails(*p, m_solvedPids.contains(p->pid));
            });
    layout->addWidget(m_levelsTree, 1);

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

    connect(m_subsCount, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::reloadSubmissions);

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
    m_rankMode->addItem(QStringLiteral("Around me (1000 above)"), 4);
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

    connect(m_rankMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::reloadRanklist);

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
    auto *refreshAll = fileMenu->addAction(QStringLiteral("&Refresh"));
    refreshAll->setShortcut(QKeySequence(QStringLiteral("F5")));
    connect(refreshAll, &QAction::triggered, this, &MainWindow::onRefreshAll);
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
    setStatus(QStringLiteral("Looking up '%1'...").arg(uname));
    m_client->fetchUserId(uname);
}

void MainWindow::onRefreshProblems()
{
    setStatus(QStringLiteral("Reloading problem list..."));
    m_client->fetchProblems();
}

void MainWindow::onRefreshAll()
{
    setStatus(QStringLiteral("Refreshing everything from uhunt..."));
    // Problem catalogue -- this also rebuilds the CP and Levels trees with the
    // freshly loaded data. (The CP book itself is a static local resource, so
    // it is loaded once at startup and needs no reload.)
    m_client->fetchProblems();

    if (m_currentUid == 0) {
        // Not signed in yet: (re)resolve the username if we have one; that will
        // in turn load the submissions and ranklist.
        const QString uname = m_userEdit->text().trimmed();
        if (!uname.isEmpty())
            m_client->fetchUserId(uname);
        return;
    }
    // Reload the signed-in account's data, honouring the tab selections.
    reloadSubmissions();
    reloadRanklist();
}

void MainWindow::reloadSubmissions()
{
    if (m_currentUid == 0)
        return;
    const int count = m_subsCount->currentData().toInt();
    if (count == 0)
        m_client->fetchUserSubmissions(m_currentUid);
    else
        m_client->fetchUserSubmissionsLast(m_currentUid, count);
}

void MainWindow::reloadRanklist()
{
    const int mode = m_rankMode->currentData().toInt();
    if (mode == 2) {
        m_client->fetchGlobalRank(1, 100);
    } else if (mode == 3) {
        m_client->fetchGlobalRank(1, 500);
    } else if (mode == 4) {
        if (m_currentUid != 0)
            m_client->fetchRanklistWindow(m_currentUid, 1000);
    } else if (m_currentUid != 0) {
        const int n = mode == 1 ? 25 : 5;
        m_client->fetchRanklist(m_currentUid, n, n);
    }
}

void MainWindow::onUserIdResolved(const QString &uname, int uid)
{
    if (uid <= 0) {
        setStatus(QStringLiteral("No such UVa user: '%1'").arg(uname));
        QMessageBox::warning(this, QStringLiteral("User not found"),
                             QStringLiteral("uhunt does not know the user '%1'.").arg(uname));
        return;
    }
    m_currentUid = uid;
    m_currentUname = uname;
    QSettings().setValue(QStringLiteral("account/username"), uname);
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
    rebuildLevelsTree();
    updateProblemCount();
    updateSolvedProgress();
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
    rebuildLevelsTree();

    m_subsValue->setText(QStringLiteral("%1").arg(subs.size()));
    updateSolvedProgress();
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

// uHunt's difficulty level for a problem, derived from the number of distinct
// accepted users (dacu): level = 10 - floor(min(10, ln(dacu))). Higher means
// harder (10 = few/no solvers, 0 = very easy). A dacu of 0 is treated as 1,
// matching uhunt.js. See the "level" cell in uhunt's CP-book view ("Lev N").
int problemLevel(int dacu)
{
    const int d = dacu > 0 ? dacu : 1;
    return 10 - std::min(10, static_cast<int>(std::floor(std::log(static_cast<double>(d)))));
}

// Fills a book exercise node's Solved / Total / % columns (indices 2-4), using
// "-" when it has no catalogue problems.
void setProgressColumns(QTreeWidgetItem *item, int solved, int total)
{
    for (int col = 2; col <= 4; ++col)
        item->setTextAlignment(col, Qt::AlignRight | Qt::AlignVCenter);
    if (total <= 0) {
        item->setText(2, QStringLiteral("-"));
        item->setText(3, QStringLiteral("-"));
        item->setText(4, QStringLiteral("-"));
        return;
    }
    item->setText(2, QString::number(solved));
    item->setText(3, QString::number(total));
    item->setText(4, QStringLiteral("%1%").arg(solved * 100 / total));
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
        delete m_cpBookTree->itemWidget(item, 5);
    };
    for (int i = 0; i < m_cpBookTree->topLevelItemCount(); ++i)
        deleteStartButtons(deleteStartButtons, m_cpBookTree->topLevelItem(i));

    m_cpBookTree->clear();
    if (m_categories.isEmpty() || m_catalog.problems.isEmpty())
        return;

    struct Agg {
        int solvedAll = 0, totalAll = 0;
    };

    // Adds the (currently inert) "Start" button to a problem row's last column.
    // It is wired up to a real action later; the problem id is stashed on the
    // button for that purpose. Only individual problems get a button.
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

        item->setSizeHint(5, button->sizeHint());
        m_cpBookTree->setItemWidget(item, 5, button);
    };

    QHash<QString, QTreeWidgetItem *> chapterItems;
    QHash<QString, QTreeWidgetItem *> sectionItems;
    QHash<QTreeWidgetItem *, Agg> aggs;
    const QColor solvedColor(0, 128, 0);
    const QColor triedColor(196, 120, 0);
    int chapterNumber = 0;

    // "Only solved" / "Only unsolved" filter. The checkboxes are mutually
    // exclusive, so at most one is active; with none active every row shows.
    const bool onlySolved = m_cpFilterSolved && m_cpFilterSolved->isChecked();
    const bool onlyUnsolved = m_cpFilterUnsolved && m_cpFilterUnsolved->isChecked();
    const bool filterActive = onlySolved != onlyUnsolved;
    // Level filter: combo index 0 = all levels, index N (>0) = level N-1.
    const int levelFilter = m_cpFilterLevel ? m_cpFilterLevel->currentIndex() - 1 : -1;
    const bool levelActive = levelFilter >= 0;
    const bool anyFilter = filterActive || levelActive;

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
        }
        const QString sectionKey = chapterName + QLatin1Char('\n') + sectionName;
        QTreeWidgetItem *section = sectionItems.value(sectionKey);
        if (!section) {
            section = new QTreeWidgetItem(chapter);
            section->setText(0, sectionName);
            sectionItems.insert(sectionKey, section);
            aggs.insert(section, Agg{});
        }
        QTreeWidgetItem *topic = new QTreeWidgetItem(section);
        topic->setText(0, topicName);
        Agg topicAgg;
        aggs.insert(topic, topicAgg);

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
            const bool statusOk = !filterActive || (onlySolved ? solved : !solved);
            const bool levelOk = !levelActive || (problemLevel(p->dacu) == levelFilter);
            const bool matches = statusOk && levelOk;

            auto *leaf = new QTreeWidgetItem(topic);
            leaf->setText(0, QStringLiteral("%1. %2%3")
                                 .arg(num)
                                 .arg(starred ? QStringLiteral("\u2605 ") : QString())
                                 .arg(p->title));
            // uHunt's difficulty level (shown as "Lev N" there).
            leaf->setText(1, QString::number(problemLevel(p->dacu)));
            leaf->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            leaf->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
            if (solved) {
                leaf->setText(2, QStringLiteral("\u2714"));
                leaf->setForeground(2, solvedColor);
            } else if (tried) {
                leaf->setText(2, QStringLiteral("tried"));
                leaf->setForeground(2, triedColor);
            }
            leaf->setData(0, Qt::UserRole, p->pid);
            leaf->setToolTip(0, QStringLiteral("%1. %2").arg(num).arg(p->title));
            // Start is only offered for problems that are not finished (solved).
            if (!solved && matches)
                addStartButton(leaf, p->pid);
            leaf->setHidden(!matches);

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

    // When any filter is active, hide chapters/sections/topics that have no
    // matching problems left, so the tree only shows relevant branches.
    if (anyFilter) {
        auto prune = [&](auto &&self, QTreeWidgetItem *item) -> int {
            if (item->data(0, Qt::UserRole).toInt() > 0)  // problem row
                return item->isHidden() ? 0 : 1;
            int visible = 0;
            for (int i = 0; i < item->childCount(); ++i)
                visible += self(self, item->child(i));
            item->setHidden(visible == 0);
            return visible;
        };
        for (int i = 0; i < m_cpBookTree->topLevelItemCount(); ++i)
            prune(prune, m_cpBookTree->topLevelItem(i));
    }
}

void MainWindow::rebuildLevelsTree()
{
    if (!m_levelsTree)
        return;
    m_levelsTree->clear();
    if (m_catalog.problems.isEmpty())
        return;

    // Bucket every loaded problem by its uHunt level (0-10), then emit one
    // "Level N" heading per non-empty bucket holding that level's problems in
    // problem-number order. No chapter/section headings are shown. The
    // solved/unsolved filter, when active, excludes non-matching problems (and
    // the level headings left with none).
    const bool onlySolved = m_levelsFilterSolved && m_levelsFilterSolved->isChecked();
    const bool onlyUnsolved = m_levelsFilterUnsolved && m_levelsFilterUnsolved->isChecked();
    const bool filterActive = onlySolved != onlyUnsolved;

    // Where each problem sits in the CP book, for the Chapter / Section columns.
    // Only the first occurrence (in book order) is kept; a handful of problems
    // are listed under more than one chapter/section.
    QHash<int, QString> chapterOf;
    QHash<int, QString> sectionOf;
    for (const Category &c : m_categories) {
        const QStringList parts = c.path.split(QStringLiteral(" / "));
        const QString chapter = parts.value(0);
        const QString section = parts.value(1);
        for (int num : c.nums) {
            if (chapterOf.contains(num))
                continue;
            chapterOf.insert(num, chapter);
            sectionOf.insert(num, section);
        }
    }

    QVector<QVector<const Problem *>> byLevel(11);
    for (const Problem &p : m_catalog.problems) {
        const bool solved = m_solvedPids.contains(p.pid);
        if (filterActive && !(onlySolved ? solved : !solved))
            continue;
        byLevel[problemLevel(p.dacu)].append(&p);
    }

    const QColor solvedColor(0, 128, 0);
    const QColor triedColor(196, 120, 0);
    for (int lvl = 0; lvl <= 10; ++lvl) {
        if (byLevel[lvl].isEmpty())
            continue;

        QVector<const Problem *> probs = byLevel[lvl];
        std::sort(probs.begin(), probs.end(),
                  [](const Problem *a, const Problem *b) { return a->num < b->num; });

        int solvedCount = 0;
        for (const Problem *p : probs)
            if (m_solvedPids.contains(p->pid))
                ++solvedCount;

        auto *root = new QTreeWidgetItem(m_levelsTree);
        root->setText(0, QStringLiteral("Level %1  (%2 problems, %3 solved)")
                             .arg(lvl)
                             .arg(probs.size())
                             .arg(solvedCount));
        QFont f = root->font(0);
        f.setBold(true);
        root->setFont(0, f);

        for (const Problem *p : probs) {
            auto *item = new QTreeWidgetItem(root);
            item->setText(0, QStringLiteral("%1. %2").arg(p->num).arg(p->title));
            item->setText(1, chapterOf.value(p->num));
            item->setText(2, sectionOf.value(p->num));
            item->setData(0, Qt::UserRole, p->pid);
            item->setToolTip(0, QStringLiteral("%1. %2").arg(p->num).arg(p->title));
            if (m_solvedPids.contains(p->pid)) {
                item->setText(3, QStringLiteral("\u2714"));
                item->setForeground(3, solvedColor);
            } else if (m_triedPids.contains(p->pid)) {
                item->setText(3, QStringLiteral("tried"));
                item->setForeground(3, triedColor);
            }
        }
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

void MainWindow::updateSolvedProgress()
{
    if (!m_solvedValue || !m_solvedPercent || !m_solvedBar)
        return;

    const int solved = m_solvedPids.size();
    const int total = m_catalog.problems.size();
    m_solvedValue->setText(QStringLiteral("%1").arg(solved));

    if (total <= 0) {
        m_solvedPercent->setText(QStringLiteral("-"));
        m_solvedBar->setValue(0);
        return;
    }

    const double pct = 100.0 * solved / total;
    m_solvedPercent->setText(QStringLiteral("%1% of %2").arg(pct, 0, 'f', 1).arg(total));
    m_solvedBar->setValue(qRound(pct));
}

void MainWindow::setStatus(const QString &text)
{
    statusBar()->showMessage(text);
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
    rebuildLevelsTree();
    updateProblemCount();

    for (QLabel *label : { m_rankValue, m_solvedValue, m_solvedPercent, m_subsValue }) {
        label->setText(QStringLiteral("-"));
    }
    if (m_solvedBar)
        m_solvedBar->setValue(0);

    setStatus(QStringLiteral("Signed out."));
}
