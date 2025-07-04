#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSplitter>
#include <QScrollBar>
#include <QRegularExpression>
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
    
    // Run all queries button
    QPushButton *runAllQueriesButton = new QPushButton("Run All Queries", fileGroup);
    connect(runAllQueriesButton, &QPushButton::clicked, this, &MainWindow::runAllQueries);
    fileLayout->addWidget(runAllQueriesButton);
    
    controlLayout->addWidget(fileGroup);

    // ======= New Query Navigation UI with Coordinate Fields =======
    QGroupBox *queryNavGroup = new QGroupBox("Query Navigation", controlPanel);
    QVBoxLayout *queryNavLayout = new QVBoxLayout(queryNavGroup);

    // Coordinate inputs
    QHBoxLayout *coordsLayout = new QHBoxLayout();
    startXEdit = new QLineEdit();
    startXEdit->setPlaceholderText("Start X");
    startYEdit = new QLineEdit();
    startYEdit->setPlaceholderText("Start Y");
    endXEdit = new QLineEdit();
    endXEdit->setPlaceholderText("End X");
    endYEdit = new QLineEdit();
    endYEdit->setPlaceholderText("End Y");

    coordsLayout->addWidget(startXEdit);
    coordsLayout->addWidget(startYEdit);
    coordsLayout->addWidget(endXEdit);
    coordsLayout->addWidget(endYEdit);

    queryNavLayout->addLayout(coordsLayout);

    // Navigation buttons
    QHBoxLayout *navButtonsLayout = new QHBoxLayout();
    QPushButton *btnPrevQuery = new QPushButton("<-- Prev Query", queryNavGroup);
    queryIndexEdit = new QLineEdit(queryNavGroup);
    queryIndexEdit->setAlignment(Qt::AlignCenter);
    queryIndexEdit->setPlaceholderText("Query Number");
    QPushButton *btnNextQuery = new QPushButton("Next Query -->", queryNavGroup);


    navButtonsLayout->addWidget(btnPrevQuery);
    navButtonsLayout->addWidget(queryIndexEdit);
    navButtonsLayout->addWidget(btnNextQuery);

    queryNavLayout->addLayout(navButtonsLayout);
    controlLayout->addWidget(queryNavGroup);

    // Connect navigation
    connect(btnPrevQuery, &QPushButton::clicked, this, [=]() {
        if (currentQueryIndex > 0) {
            currentQueryIndex--;
            Query query = queryList[currentQueryIndex];
            QString resultText = QString::fromStdString(mapGraph->findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R).resultText);
            displayQuery(query, resultText);
        }
    });

    connect(btnNextQuery, &QPushButton::clicked, this, [=]() {
        if (currentQueryIndex < queryList.size() - 1) {
            currentQueryIndex++;
            Query query = queryList[currentQueryIndex];
            QString resultText = QString::fromStdString(mapGraph->findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R).resultText);
            displayQuery(query, resultText);
        }
    });

    // Connect query index input
    connect(queryIndexEdit, &QLineEdit::returnPressed, this, [=]() {
        bool ok;
        int newIndex = queryIndexEdit->text().toInt(&ok) - 1; // Convert to 0-based index
        if (ok && newIndex >= 0 && newIndex < queryList.size()) {
            currentQueryIndex = newIndex;
            Query query = queryList[currentQueryIndex];
            QString resultText = QString::fromStdString(mapGraph->findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R).resultText);
            displayQuery(query, resultText);
        } else {
            queryIndexEdit->setText(QString::number(currentQueryIndex + 1)); // Reset to current index if invalid
        }
    });

    // Adding select start/end button/////////////////////////////////////////////////////////////////
    QPushButton *enable_SE_Button = new QPushButton("Enable Start/End Selection", fileGroup);
    enable_SE_Button->setStyleSheet("background-color: green; color: white;");
    enable_SE_Button->setCheckable(true);  // make it toggle
    enable_SE_Button->setChecked(true);   // default: off
    fileLayout->addWidget(enable_SE_Button);
    connect(enable_SE_Button, &QPushButton::toggled, this, [=](bool checked) {
        isSelectionEnabled = checked;
        enableSelection();
        enable_SE_Button->setStyleSheet(QString("background-color: %1; color: white;").arg(isSelectionEnabled ? "red" : "green"));
        enable_SE_Button->setText((isSelectionEnabled ? "Disable Start/End Selection" : "Enable Start/End Selection"));
    });


    // Path finding group
    QGroupBox *pathGroup = new QGroupBox("Path Finding", controlPanel);
    QVBoxLayout *pathLayout = new QVBoxLayout(pathGroup);
    
    QHBoxLayout *RLayout = new QHBoxLayout();
    QLabel *RLabel = new QLabel("Max Walking Distance (km):", pathGroup);
    REdit = new QLineEdit(pathGroup);
    REdit->setText("10000");
    RLayout->addWidget(RLabel);
    RLayout->addWidget(REdit);
    pathLayout->addLayout(RLayout);
    
    QLabel *instructionLabel = new QLabel("Click on map to select start and end points", pathGroup);
    pathLayout->addWidget(instructionLabel);
    
    QPushButton *findPathButton = new QPushButton("Find Shortest Path", pathGroup);
    connect(findPathButton, &QPushButton::clicked, this, &MainWindow::findShortestPath);
    pathLayout->addWidget(findPathButton);
    
    QPushButton *resetButton = new QPushButton("Reset", pathGroup);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::handleResetAll);
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
    timeInMap = 0;
    QString filePath = QFileDialog::getOpenFileName(this, "Open Map File", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    mapFilePath = filePath;
    mapPathLabel->setText(mapFilePath);

    const auto startInMap = std::chrono::high_resolution_clock::now();
    if (mapGraph->loadMapFromFile(mapFilePath.toStdString())) {
        mapVisualizer->setMapGraph(mapGraph);

        // === NEW: Reset map view and UI ===
        if (mapVisualizer) mapVisualizer->reset();
        handleResetAll();  // Also resets UI fields

        // === NEW: Clear queries path info ===
        queriesFilePath.clear();
        queriesPathLabel->setText("No queries file selected");

        displayResult("Map file loaded successfully.");
    } else {
        displayResult("Error loading map file.");
        return;
    }
    const auto endInMap = std::chrono::high_resolution_clock::now();
    timeInMap = std::chrono::duration_cast<std::chrono::milliseconds>(endInMap - startInMap).count();
}

void MainWindow::loadQueriesFile()
{
    timeInQuery = 0;
    QString filePath = QFileDialog::getOpenFileName(this, "Open Queries File", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    queriesFilePath = filePath;
    queriesPathLabel->setText(queriesFilePath);

    const auto startInQuery = std::chrono::high_resolution_clock::now();
    if (mapGraph->loadQueriesFromFile(queriesFilePath.toStdString())) {
        displayResult("Queries file loaded successfully.");
    } else {
        displayResult("Error loading queries file.");
        return;
    }

    queryList = mapGraph->getQueries();

    if (queryList.empty()) {
        QMessageBox::warning(this, "Invalid File", "The query file is empty or invalid.");
        return;
    }

    currentQueryIndex = 0;
    Query query = queryList[currentQueryIndex];
    QString resultText = QString::fromStdString(mapGraph->findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R).resultText);
    displayQuery(query, resultText);

    const auto endInQuery = std::chrono::high_resolution_clock::now();
    timeInQuery = std::chrono::duration_cast<std::chrono::milliseconds>(endInQuery - startInQuery).count();
}

void MainWindow::findShortestPath()
{
    if (mapGraph->empty()) {
        displayResult("Error: No map loaded.");
        return;
    }

    bool ok1, ok2, ok3, ok4;
    double startX = startXEdit->text().toDouble(&ok1);
    double startY = startYEdit->text().toDouble(&ok2);
    double endX = endXEdit->text().toDouble(&ok3);
    double endY = endYEdit->text().toDouble(&ok4);

    if (!ok1 || !ok2 || !ok3 || !ok4) {
        displayResult("Invalid coordinate input.");
        return;
    }

    double R = REdit->text().toDouble();

    auto start = std::chrono::high_resolution_clock::now();
    PathResult pathResult = mapGraph->findShortestPath(startX, startY, endX, endY, R);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    QString result = QString::fromStdString(pathResult.resultText);
    result += "\nComputation time: " + QString::number(duration) + " ms";

    displayResult(result);

    mapVisualizer->startPointSelected(startX, startY);
    mapVisualizer->endPointSelected(endX, endY);
    mapVisualizer->update();
}

void MainWindow::onPointsSelected(double startX, double startY, double endX, double endY)
{
    double R = REdit->text().toDouble();
    
    auto start = std::chrono::high_resolution_clock::now();
    PathResult pathResult = mapGraph->findShortestPath(startX, startY, endX, endY, R);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    QString result = QString::fromStdString(pathResult.resultText);
    result += "\nComputation time: " + QString::number(duration) + " ms";
    
    displayResult(result);
    mapVisualizer->update();
}

void MainWindow::saveResults(const std::string& filename, const std::vector<PathResult>& results) {
    timeOut = 0;
    const auto startOut = std::chrono::high_resolution_clock::now();
    std::ofstream out(filename);

    for (const auto& res : results) {
        out << res.resultText << "\n";
    }
    const auto endOut = std::chrono::high_resolution_clock::now();
    timeOut = std::chrono::duration_cast<std::chrono::milliseconds>(endOut - startOut).count();

    // Execution times would normally go here (for lab measurement)
    out << timeBase <<" ms\n\n"; // placeholder
    out << timeInMap + timeInQuery + timeBase + timeOut <<" ms\n"; // placeholder

    out.close();
}

void MainWindow::runAllQueries()
{
    if (mapGraph->empty()) {
        displayResult("Error: No map loaded.");
        return;
    }
    
    if (queriesFilePath.isEmpty()) {
        displayResult("Error: No queries file loaded.");
        return;
    }
    timeBase = 0;
    // Start timer for all queries
    auto startAll = std::chrono::high_resolution_clock::now();
    
    // Run all queries
    std::vector<PathResult> results = mapGraph->runAllQueries();
    
    // End timer for all queries
    auto endAll = std::chrono::high_resolution_clock::now();
    timeBase = std::chrono::duration_cast<std::chrono::milliseconds>(endAll - startAll).count();

    saveResults("Output/outputs.txt", results);

    // Format the results
    QString resultText;
    resultText += "Executed " + QString::number(results.size()) + " queries in " + 
                  QString::number(timeBase) + " ms\nExecution time + I/O: " +
                  QString::number(timeInMap + timeInQuery + timeBase + timeOut) + " ms\n\n";
    resultText += QString::fromStdString(mapGraph->displayOutput(results));
    
    // Display results and update visualization
    currentQueryIndex = queryList.size() - 1;
    displayQuery(queryList[currentQueryIndex], resultText);
}

void MainWindow::displayResult(const QString &result) const {
    outputTextEdit->setText(result);
    // Scroll to the top
    outputTextEdit->verticalScrollBar()->setValue(outputTextEdit->verticalScrollBar()->minimum());
}


void MainWindow::enableSelection()
{
    isSelectionEnabled = !isSelectionEnabled;
    qDebug() << "Selection mode is now " << (isSelectionEnabled ? "ON" : "OFF");
}

void MainWindow::displayQuery(const Query &query, const QString& resultText) const {
    startXEdit->setText(QString::number(query.startX));
    startYEdit->setText(QString::number(query.startY));
    endXEdit->setText(QString::number(query.endX));
    endYEdit->setText(QString::number(query.endY));
    REdit->setText(QString::number(query.R));
    queryIndexEdit->setText(QString::number(currentQueryIndex + 1));

    mapVisualizer->setStartPoint(query.startX, query.startY);
    mapVisualizer->setEndPoint(query.endX, query.endY);
    displayResult(resultText);
    mapVisualizer->update();
}

void MainWindow::handleResetAll() {
    if (mapVisualizer) {
        mapVisualizer->reset();
    }
    if (outputTextEdit) outputTextEdit->clear();
    if (queryIndexEdit) queryIndexEdit->clear();
    if (startXEdit) startXEdit->clear();
    if (startYEdit) startYEdit->clear();
    if (endXEdit) endXEdit->clear();
    if (endYEdit) endYEdit->clear();

    currentQueryIndex = 0;
}
