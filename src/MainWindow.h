#pragma once

#include <QMainWindow>
#include <QSet>

#include "Types.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QTabWidget;
class QTableView;
class QAction;

class UhuntClient;
class ProblemTableModel;
class ProblemFilterProxyModel;
class SubmissionTableModel;
class RanklistTableModel;
class LiveEventTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onHunt();
    void onRefreshProblems();
    void onUserIdResolved(const QString &uname, int uid);
    void onProblemsFetched(const QVector<Problem> &problems);
    void onUserSubmissionsFetched(int uid, const QVector<Submission> &subs);
    void onRanklistFetched(const QVector<UserRank> &list);
    void onCpBookFetched(const QVector<Category> &categories);
    void onLiveEvents(const QVector<LiveEvent> &events, qint64 lastId);
    void onLiveToggled(bool on);
    void onProblemSearchChanged();
    void onStatusFilterChanged(int index);
    void onCategoryChanged(int index);
    void onProblemActivated(const QModelIndex &index);
    void onError(const QString &context, const QString &message);
    void onSignOut();

private:
    QWidget *buildDashboardTab();
    QWidget *buildProblemsTab();
    QWidget *buildSubmissionsTab();
    QWidget *buildRanklistTab();
    QWidget *buildLiveTab();
    void buildMenus();
    void showProblemDetails(const Problem &problem, bool solved);
    void refreshRecommendations();
    void updateProblemCount();
    void updateAccountUi();
    void setStatus(const QString &text);
    void configureTable(QTableView *view);

    UhuntClient *m_client = nullptr;

    ProblemCatalog m_catalog;
    QSet<int> m_solvedPids;
    QVector<Category> m_categories;
    int m_currentUid = 0;
    QString m_currentUname;

    // Dashboard
    QLabel *m_rankValue = nullptr;
    QLabel *m_solvedValue = nullptr;
    QLabel *m_subsValue = nullptr;
    QTableView *m_recommendView = nullptr;
    ProblemTableModel *m_recommendModel = nullptr;

    // Problems
    QLineEdit *m_problemSearch = nullptr;
    QComboBox *m_statusFilter = nullptr;
    QComboBox *m_categoryFilter = nullptr;
    QLabel *m_problemCount = nullptr;
    QTableView *m_problemView = nullptr;
    ProblemTableModel *m_problemModel = nullptr;
    ProblemFilterProxyModel *m_problemProxy = nullptr;

    // Submissions
    QComboBox *m_subsCount = nullptr;
    QTableView *m_subView = nullptr;
    SubmissionTableModel *m_subModel = nullptr;

    // Ranklist
    QComboBox *m_rankMode = nullptr;
    QTableView *m_rankView = nullptr;
    RanklistTableModel *m_rankModel = nullptr;

    // Live
    QPushButton *m_liveToggle = nullptr;
    QLabel *m_liveStatus = nullptr;
    QTableView *m_liveView = nullptr;
    LiveEventTableModel *m_liveModel = nullptr;

    // Toolbar
    QLineEdit *m_userEdit = nullptr;
    QPushButton *m_huntButton = nullptr;
    QLabel *m_accountLabel = nullptr;
    QAction *m_signOutAction = nullptr;
    QTabWidget *m_tabs = nullptr;
};
