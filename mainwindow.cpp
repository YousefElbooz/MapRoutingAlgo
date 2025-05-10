#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSplitter>
#include <QScrollBar>
#include <iostream>
#include <chrono>
#include <fstream>


bool MainWindow::isSelectionEnabled = false;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mapGraph(std::make_shared<MapGraph>())
{
    ui->setupUi(this);
    setupUi();
    
    setWindowTitle("Map Routing Visualizer");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUi()
{
    // Create main splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    
    // Create map visualizer widget
    mapVisualizer = new MapVisualizer(this);
    mapVisualizer->setMapGraph(mapGraph);

    // Create controls panel
    QWidget *controlPanel = new QWidget(this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    
    // File selection group
    QGroupBox *fileGroup = new QGroupBox("Input/Output Files", controlPanel);
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);
    
    // Map file selection
    QHBoxLayout *mapFileLayout = new QHBoxLayout();
    mapPathLabel = new QLabel("No map file selected", fileGroup);
    mapPathLabel->setWordWrap(true);
    QPushButton *mapFileButton = new QPushButton("Select Map", fileGroup);
    connect(mapFileButton, &QPushButton::clicked, this, &MainWindow::loadMapFile);
    mapFileLayout->addWidget(mapPathLabel);
    mapFileLayout->addWidget(mapFileButton);
    fileLayout->addLayout(mapFileLayout);
    
    // Queries file selection
    QHBoxLayout *queriesFileLayout = new QHBoxLayout();
    queriesPathLabel = new QLabel("No queries file selected", fileGroup);
    queriesPathLabel->setWordWrap(true);
    QPushButton *queriesFileButton = new QPushButton("Select Queries", fileGroup);
    connect(queriesFileButton, &QPushButton::clicked, this, &MainWindow::loadQueriesFile);
    queriesFileLayout->addWidget(queriesPathLabel);
    queriesFileLayout->addWidget(queriesFileButton);
    fileLayout->addLayout(queriesFileLayout);
    
    // Output file selection
    QHBoxLayout *outputFileLayout = new QHBoxLayout();
    outputPathLabel = new QLabel("No output file selected", fileGroup);
    outputPathLabel->setWordWrap(true);
    QPushButton *outputFileButton = new QPushButton("Select Output", fileGroup);
    connect(outputFileButton, &QPushButton::clicked, this, &MainWindow::compareOutputFile);
    outputFileLayout->addWidget(outputPathLabel);
    outputFileLayout->addWidget(outputFileButton);
    fileLayout->addLayout(outputFileLayout);
    
    // Run all queries button
    QPushButton *runAllQueriesButton = new QPushButton("Run All Queries & Compare", fileGroup);
    connect(runAllQueriesButton, &QPushButton::clicked, this, &MainWindow::runAllQueries);
    fileLayout->addWidget(runAllQueriesButton);
    
    controlLayout->addWidget(fileGroup);
    
    // Adding select start/end button/////////////////////////////////////////////////////////////////
    QPushButton *enable_SE_Button = new QPushButton("Enable Start/End Selection", fileGroup);
    enable_SE_Button->setCheckable(true);  // make it toggle
    enable_SE_Button->setChecked(true);   // default: off
    fileLayout->addWidget(enable_SE_Button);
    connect(enable_SE_Button, &QPushButton::toggled, this, [=](bool checked) {
        isSelectionEnabled = checked;
        enableSelection();
        enable_SE_Button->setStyleSheet("background-color: green; color: white;");
    });


    // Path finding group
    QGroupBox *pathGroup = new QGroupBox("Path Finding", controlPanel);
    QVBoxLayout *pathLayout = new QVBoxLayout(pathGroup);
    
    QHBoxLayout *maxSpeedLayout = new QHBoxLayout();
    QLabel *maxSpeedLabel = new QLabel("Max Speed (km/h):", pathGroup);
    maxSpeedEdit = new QLineEdit(pathGroup);
    maxSpeedEdit->setText("100");
    maxSpeedLayout->addWidget(maxSpeedLabel);
    maxSpeedLayout->addWidget(maxSpeedEdit);
    pathLayout->addLayout(maxSpeedLayout);
    
    QLabel *instructionLabel = new QLabel("Click on map to select start and end points", pathGroup);
    pathLayout->addWidget(instructionLabel);
    
    QPushButton *findPathButton = new QPushButton("Find Shortest Path", pathGroup);
    connect(findPathButton, &QPushButton::clicked, this, &MainWindow::findShortestPath);
    pathLayout->addWidget(findPathButton);
    
    QPushButton *resetButton = new QPushButton("Reset", pathGroup);
    connect(resetButton, &QPushButton::clicked, mapVisualizer, &MapVisualizer::reset);
    pathLayout->addWidget(resetButton);
    
    controlLayout->addWidget(pathGroup);
    
    // Output text area
    QGroupBox *outputGroup = new QGroupBox("Results", controlPanel);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);
    
    outputTextEdit = new QTextEdit(outputGroup);
    outputTextEdit->setReadOnly(true);
    outputLayout->addWidget(outputTextEdit);
    
    controlLayout->addWidget(outputGroup);
    
    // Add to splitter
    // Wrap mapVisualizer in a scroll area
    mainSplitter->addWidget(mapVisualizer);
    mainSplitter->addWidget(controlPanel);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);
    
    // Connect signals
    connect(mapVisualizer, &MapVisualizer::pointsSelected, this, &MainWindow::onPointsSelected);
}

void MainWindow::loadMapFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Map File", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    
    mapFilePath = filePath;
    mapPathLabel->setText(mapFilePath);
    
    if (mapGraph->loadMapFromFile(mapFilePath.toStdString())) {
        mapVisualizer->setMapGraph(mapGraph);
        displayResult("Map file loaded successfully.");
    } else {
        displayResult("Error loading map file.");
    }
}

void MainWindow::loadQueriesFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Queries File", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    
    queriesFilePath = filePath;
    queriesPathLabel->setText(queriesFilePath);
    
    if (mapGraph->loadQueriesFromFile(queriesFilePath.toStdString())) {
        displayResult("Queries file loaded successfully.");
    } else {
        displayResult("Error loading queries file.");
    }
}

void MainWindow::compareOutputFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Output File", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    
    outputFilePath = filePath;
    outputPathLabel->setText(outputFilePath);
    
    QString result = QString::fromStdString(mapGraph->compareWithOutput(outputFilePath.toStdString()));
    displayResult("Comparison results:\n" + result);
}

void MainWindow::findShortestPath()
{
    if (mapGraph->getNodes().empty()) {
        displayResult("Error: No map loaded.");
        return;
    }
    
    // Get values directly from the selected points on the visualizer
    if (mapVisualizer->getStartPoint().isNull() || mapVisualizer->getEndPoint().isNull()) {
        displayResult("Please select start and end points on the map first.");
        return;
    }
    
    double startX = mapVisualizer->getStartPoint().x();
    double startY = mapVisualizer->getStartPoint().y();
    double endX = mapVisualizer->getEndPoint().x();
    double endY = mapVisualizer->getEndPoint().y();
    
    double maxSpeed = maxSpeedEdit->text().toDouble();
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string pathResult = mapGraph->findShortestPath(startX, startY, endX, endY, maxSpeed);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    QString result = QString::fromStdString(pathResult);
    result += "\nComputation time: " + QString::number(duration) + " ms";
    
    displayResult(result);
    mapVisualizer->update();
}

void MainWindow::onPointsSelected(double startX, double startY, double endX, double endY)
{
    double maxSpeed = maxSpeedEdit->text().toDouble();
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string pathResult = mapGraph->findShortestPath(startX, startY, endX, endY, maxSpeed);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    QString result = QString::fromStdString(pathResult);
    result += "\nComputation time: " + QString::number(duration) + " ms";
    
    displayResult(result);
    mapVisualizer->update();
}

void MainWindow::runAllQueries()
{
    if (mapGraph->getNodes().empty()) {
        displayResult("Error: No map loaded.");
        return;
    }
    
    if (queriesFilePath.isEmpty()) {
        displayResult("Error: No queries file loaded.");
        return;
    }
    
    if (outputFilePath.isEmpty()) {
        displayResult("Error: No output file selected for comparison.");
        return;
    }
    
    // Get max speed from UI if needed for all queries
    double maxSpeed = maxSpeedEdit->text().toDouble();
    
    // Start timer for all queries
    auto startAll = std::chrono::high_resolution_clock::now();
    
    // Run all queries
    std::vector<PathResult> results = mapGraph->runAllQueries();
    
    // End timer for all queries
    auto endAll = std::chrono::high_resolution_clock::now();
    auto durationAll = std::chrono::duration_cast<std::chrono::milliseconds>(endAll - startAll).count();
    
    // Format the results
    QString resultText;
    resultText += "Executed " + QString::number(results.size()) + " queries in " + 
                  QString::number(durationAll) + " ms\n\n";
    
    // Compare with expected output
    std::ifstream outputFile(outputFilePath.toStdString());
    if (!outputFile.is_open()) {
        resultText += "Error: Could not open output file for comparison.";
        displayResult(resultText);
        return;
    }
    outputFile.close();
    
    // Run comparison
    QString comparisonResult = QString::fromStdString(mapGraph->compareWithOutput(outputFilePath.toStdString()));
    
    // Extract summary metrics if present
    int summaryPos = comparisonResult.indexOf("===== SUMMARY STATISTICS =====");
    QString summary;
    if (summaryPos != -1) {
        summary = comparisonResult.mid(summaryPos);
        comparisonResult = comparisonResult.left(summaryPos);
    }
    
    // Combine results and display
    resultText += "COMPARISON SUMMARY:\n";
    if (!summary.isEmpty()) {
        resultText += summary + "\n\n";
    }
    resultText += "DETAILED RESULTS:\n" + comparisonResult;
    
    // Display results and update visualization with the last query's path
    displayResult(resultText);
    mapVisualizer->update();
}

void MainWindow::displayResult(const QString &result)
{
    outputTextEdit->setText(result);
    // Scroll to the top
    outputTextEdit->verticalScrollBar()->setValue(outputTextEdit->verticalScrollBar()->minimum());
}


void MainWindow::enableSelection()
{
    isSelectionEnabled = !isSelectionEnabled;
    qDebug() << "Selection mode is now " << (isSelectionEnabled ? "ON" : "OFF");
}
