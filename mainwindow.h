#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCompleter>
#include <memory>
#include "mapgraph.h"
#include "mapvisualizer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadMapFile();
    void loadQueriesFile();
    void findShortestPath();
    void onPointsSelected(double startX, double startY, double endX, double endY);
    void saveResults(const std::string& filename, const std::vector<PathResult>& results);
    void runAllQueries();
    void handleResetAll();

    static void enableSelection();

public:
    static bool isSelectionEnabled;

private:
    Ui::MainWindow *ui;
    std::shared_ptr<MapGraph> mapGraph;
    MapVisualizer *mapVisualizer;
    
    QTextEdit *outputTextEdit;
    QLabel *mapPathLabel;
    QLabel *queriesPathLabel;
    QLabel *outputPathLabel;
    QLineEdit *REdit;
    
    QString mapFilePath;
    QString queriesFilePath;
    QString outputFilePath;

    //Queries/////////////---1---///////////////////
    QLineEdit *queryLineEdit;
    QLineEdit *startXEdit;
    QLineEdit *startYEdit;
    QLineEdit *endXEdit;
    QLineEdit *endYEdit;
    QLineEdit *queryIndexEdit;

    std::vector<Query> queryList;  // Stores all queries from file
    int currentQueryIndex = 0;  // Tracks current query

    long long timeInMap;
    long long timeInQuery;
    long long timeOut;
    long long timeBase;

    void setupUi();
    void displayResult(const QString &result) const;
    void displayQuery(const Query &query, QString resultText) const;
};
#endif // MAINWINDOW_H
