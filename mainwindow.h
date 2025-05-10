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
    void compareOutputFile();
    void findShortestPath();
    void onPointsSelected(double startX, double startY, double endX, double endY);
    void runAllQueries();
    void enableSelection();

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
    QLineEdit *maxSpeedEdit;
    
    QString mapFilePath;
    QString queriesFilePath;
    QString outputFilePath;
    
    void setupUi();
    void displayResult(const QString &result);
};
#endif // MAINWINDOW_H
