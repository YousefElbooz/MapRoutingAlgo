#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QCompleter>
#include <memory>
#include <QThread>
#include "mapgraph.h"
#include "mapvisualizer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void loadMapFile();
    void loadQueriesFile();
    void findShortestPath() const;
    void onPointsSelected(double startX, double startY, double endX, double endY) const;
    void saveResults(const std::string& filename, const std::vector<PathResult>& results);
    void runAllQueries();
    void handleResetAll();
    void toggleTheme() const;
    void updateTheme() const;

    void toggleButtonThemeUpdate(AppTheme theme) const;

public:
    static bool isSelectionEnabled;

private:
    Ui::MainWindow *ui;
    
    QTextEdit *outputTextEdit{};
    QLabel *mapPathLabel{};
    QLabel *queriesPathLabel{};
    QLabel *outputPathLabel{};
    QLineEdit *REdit{};
    QPushButton *themeToggleButton{};
    
    QString mapFilePath;
    QString queriesFilePath;
    QString outputFilePath;

    //Queries/////////////---1---///////////////////
    QLineEdit *queryLineEdit{};
    QLineEdit *startXEdit{};
    QLineEdit *startYEdit{};
    QLineEdit *endXEdit{};
    QLineEdit *endYEdit{};
    QLineEdit *queryIndexEdit{};

    std::vector<Query> queryList;  // Stores all queries from file
    unsigned long long currentQueryIndex = 0;  // Tracks current query

    long long timeInMap{};
    long long timeInQuery{};
    long long timeOut{};
    long long timeBase{};

    void setupUi();
    void displayResult(const QString &result) const;
    void displayQuery(const Query &query, const QString& resultText) const;
    void pathFindingTextEdit(const bool noMap) const;

    void pathFindingTextUpdate(double startX, double startY, double endX, double endY, double R) const;

    // Loading overlay helpers
    void showLoading(const QString &message);
    void hideLoading() const;
    QWidget *loadingOverlay = nullptr;
    QLabel *loadingLabel = nullptr;

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H