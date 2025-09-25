#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSplitter>
#include <QScrollBar>
#include <iostream>
#include <chrono>
#include <fstream>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QApplication>
#include <QMessageBox>
#include <atomic>
#include <QPushButton>
#include <QStyle>

bool MainWindow::isSelectionEnabled = false;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUi();
    updateTheme();
    setWindowTitle("Map Routing Visualizer");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUi()
{
    // Create the main splitter
    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // Create map visualizer widget
    MapVisualizer::instance()->setMapGraph();

    // Create controls panel
    auto *controlPanel = new QWidget(this);
    controlPanel->setMinimumWidth(350);
    auto *controlLayout = new QVBoxLayout(controlPanel);

    // File selection group
    auto *fileGroup = new QGroupBox("Input/Output Files", controlPanel);
    auto *fileLayout = new QVBoxLayout(fileGroup);

    // Map file selection
    auto *mapFileLayout = new QHBoxLayout();
    mapPathLabel = new QLabel("No map file selected", fileGroup);
    mapPathLabel->setWordWrap(true);
    mapPathLabel->setStyleSheet("border: 1px solid #707070;");
    auto *mapFileButton = new QPushButton("Select Map", fileGroup);
    connect(mapFileButton, &QPushButton::clicked, this, &MainWindow::loadMapFile);
    mapFileLayout->addWidget(mapPathLabel,3);
    mapFileLayout->addWidget(mapFileButton,1);
    fileLayout->addLayout(mapFileLayout);

    // Queries file selection
    auto *queriesFileLayout = new QHBoxLayout();
    queriesPathLabel = new QLabel("No queries file selected", fileGroup);
    queriesPathLabel->setWordWrap(true);
    queriesPathLabel->setStyleSheet("border: 1px solid #707070;");
    auto *queriesFileButton = new QPushButton("Select Queries", fileGroup);
    connect(queriesFileButton, &QPushButton::clicked, this, &MainWindow::loadQueriesFile);
    queriesFileLayout->addWidget(queriesPathLabel,3);
    queriesFileLayout->addWidget(queriesFileButton,1);
    fileLayout->addLayout(queriesFileLayout);

    // Run all queries button
    auto *runAllQueriesButton = new QPushButton("Run All Queries", fileGroup);
    connect(runAllQueriesButton, &QPushButton::clicked, this, &MainWindow::runAllQueries);
    fileLayout->addWidget(runAllQueriesButton);

    // Adding select start/end button/////////////////////////////////////////////////////////////////
    auto *enable_SE_Button = new QPushButton("Enable Start/End Selection", fileGroup);
    enable_SE_Button->setStyleSheet("background-color: green; color: white;");
    enable_SE_Button->setCheckable(true);  // make it toggle
    enable_SE_Button->setChecked(true);   // default: off
    enable_SE_Button->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: #00aa00 !important;"
        "  color: white !important;"
        "  border: 1px solid #00aa00 !important;"
        "  padding: 4px 12px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #006600 !important;"
        "  border-color: #006600 !important;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #004d00 !important;"
        "  border-color: #004d00 !important;"
        "}"
    ));
    connect(enable_SE_Button, &QPushButton::toggled, this, [=](const bool checked) {
        isSelectionEnabled = checked;
        isSelectionEnabled = !isSelectionEnabled;
        QString color = isSelectionEnabled ? "red" : "#00aa00";
        QString hoverColor = isSelectionEnabled ? "#cc0000" : "#006600"; // darker shade for hover
        QString pressedColor = isSelectionEnabled ? "#990000" : "#004d00"; // even darker for pressed

        enable_SE_Button->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1 !important;"
            "  color: white !important;"
            "  border: 1px solid %1 !important;"
            "  padding: 4px 12px;"
            "  border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "  background-color: %2 !important;"
            "  border-color: %2 !important;"
            "}"
            "QPushButton:pressed {"
            "  background-color: %3 !important;"
            "  border-color: %3 !important;"
            "}"
        ).arg(color, hoverColor, pressedColor));
        enable_SE_Button->setText(isSelectionEnabled ? "Disable Start/End Selection" : "Enable Start/End Selection");
    });
    fileLayout->addWidget(enable_SE_Button);

    controlLayout->addWidget(fileGroup);

    // ======= New Query Navigation UI with Coordinate Fields =======
    auto *queryNavGroup = new QGroupBox("Query Navigation", controlPanel);
    // Navigation buttons
    auto *navButtonsLayout = new QHBoxLayout(queryNavGroup);
    auto *btnPrevQuery = new QPushButton("<-- Prev Query", queryNavGroup);
    queryIndexEdit = new QLineEdit(queryNavGroup);
    queryIndexEdit->setAlignment(Qt::AlignCenter);
    queryIndexEdit->setPlaceholderText("Query Number");
    queryIndexEdit->setDisabled(queryList.empty());
    auto *btnNextQuery = new QPushButton("Next Query -->", queryNavGroup);

    navButtonsLayout->addWidget(btnPrevQuery);
    navButtonsLayout->addWidget(queryIndexEdit);
    navButtonsLayout->addWidget(btnNextQuery);

    controlLayout->addWidget(queryNavGroup);

    // Connect navigation
    connect(btnPrevQuery, &QPushButton::clicked, this, [=] {
        if (currentQueryIndex > 0 && !queryList.empty()) {
            currentQueryIndex--;
            const Query query = queryList[currentQueryIndex];
            const QString resultText = QString::fromStdString(MapGraph::instance().findShortestPath(query.startX, query.startY,
                                                                                                    query.endX, query.endY, query.R).resultText);
            displayQuery(query, resultText);
        }
    });

    connect(btnNextQuery, &QPushButton::clicked, this, [=] {
        if (currentQueryIndex < queryList.size() - 1 && !queryList.empty()) {
            currentQueryIndex++;
            const Query query = queryList[currentQueryIndex];
            const QString resultText = QString::fromStdString(MapGraph::instance().findShortestPath(query.startX, query.startY,
                                                                                                    query.endX, query.endY, query.R).resultText);
            displayQuery(query, resultText);
        }
    });

    // Connect query index input
    connect(queryIndexEdit, &QLineEdit::returnPressed, this, [=] {
        bool ok;
        if (const int newIndex = queryIndexEdit->text().toInt(&ok) - 1; ok && newIndex >= 0 && newIndex < queryList.size()) {
            currentQueryIndex = newIndex;
            const Query query = queryList[currentQueryIndex];
            const QString resultText = QString::fromStdString(MapGraph::instance().findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R).resultText);
            displayQuery(query, resultText);
        } else {
            queryIndexEdit->setText(QString::number(currentQueryIndex + 1)); // Reset to current index if invalid
        }
    });

    // Path finding group
    auto *pathGroup = new QGroupBox("Path Finding", controlPanel);
    auto *pathLayout = new QVBoxLayout(pathGroup);
    auto *instructionLabel = new QLabel("Click on map to select start and end points\n"
                                        "Or set the coordinates manually", pathGroup);
    pathLayout->addWidget(instructionLabel);

    // Coordinate inputs
    auto *coordsLayout = new QHBoxLayout();
    const bool noMap = MapGraph::instance().empty();
    startXEdit = new QLineEdit();
    startXEdit->setPlaceholderText("Start X");
    startYEdit = new QLineEdit();
    startYEdit->setPlaceholderText("Start Y");
    endXEdit = new QLineEdit();
    endXEdit->setPlaceholderText("End X");
    endYEdit = new QLineEdit();
    endYEdit->setPlaceholderText("End Y");
    auto *RLayout = new QHBoxLayout();
    auto *RLabel = new QLabel("Max Walking Distance (km):", pathGroup);
    REdit = new QLineEdit(pathGroup);
    REdit->setText("10000");

    pathFindingTextEdit(MapGraph::instance().empty());

    coordsLayout->addWidget(startXEdit);
    coordsLayout->addWidget(startYEdit);
    coordsLayout->addWidget(endXEdit);
    coordsLayout->addWidget(endYEdit);
    RLayout->addWidget(RLabel);
    RLayout->addWidget(REdit);

    pathLayout->addLayout(coordsLayout);

    pathLayout->addLayout(RLayout);


    auto *findPathButton = new QPushButton("Find Shortest Path", pathGroup);
    connect(findPathButton, &QPushButton::clicked, this, &MainWindow::findShortestPath);
    pathLayout->addWidget(findPathButton);

    auto *resetButton = new QPushButton("Reset", pathGroup);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::handleResetAll);
    pathLayout->addWidget(resetButton);

    controlLayout->addWidget(pathGroup);

    // Output text area
    auto *outputGroup = new QGroupBox("Results", controlPanel);
    auto *outputLayout = new QVBoxLayout(outputGroup);

    outputTextEdit = new QTextEdit(outputGroup);
    outputTextEdit->setReadOnly(true);
    outputLayout->addWidget(outputTextEdit);

    controlLayout->addWidget(outputGroup);

    // Add the theme toggle button at the end of the control panel
    themeToggleButton = new QPushButton(controlPanel);
    themeToggleButton->setToolTip("Toggle between light and dark mode (Ctrl+T)");
    toggleButtonThemeUpdate(MapVisualizer::instance()->getCurrentTheme());
    connect(themeToggleButton, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    controlLayout->addWidget(themeToggleButton);

    // Add keyboard shortcut for theme toggle (Ctrl+T)
    auto *themeAction = new QAction("Toggle Theme", this);
    themeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);
    addAction(themeAction);

    // Add to splitter
    // Wrap mapVisualizer in a scroll area
    mainSplitter->addWidget(MapVisualizer::instance());
    mainSplitter->addWidget(controlPanel);
    mainSplitter->setStretchFactor(0, 4);
    mainSplitter->setStretchFactor(1, 2);

    // Connect signals
    connect(MapVisualizer::instance(), &MapVisualizer::pointsSelected, this, &MainWindow::onPointsSelected);
}

void MainWindow::loadMapFile()
{
    timeInMap = 0;
    const QString filePath = QFileDialog::getOpenFileName(this, "Open Map File", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    mapFilePath = filePath;
    mapPathLabel->setText(mapFilePath);

    showLoading("Loading map... Please wait");

    const auto startInMap = std::chrono::high_resolution_clock::now();
    if (MapGraph::instance().loadMapFromFile(mapFilePath.toStdString())) {
        MapVisualizer::instance()->setMapGraph();

        // === NEW: Reset map view and UI ===
        MapVisualizer::instance()->reset();
        handleResetAll();  // Also resets UI fields

        // === NEW: Clear queries path info ===
        queriesFilePath.clear();
        queriesPathLabel->setText("No queries file selected");

        displayResult("Map file loaded successfully.");
    } else {
        displayResult("Error loading map file.");
        hideLoading();
        return;
    }
    const auto endInMap = std::chrono::high_resolution_clock::now();
    timeInMap = std::chrono::duration_cast<std::chrono::milliseconds>(endInMap - startInMap).count();

    pathFindingTextEdit(MapGraph::instance().empty());
    hideLoading();
}

void MainWindow::loadQueriesFile()
{
    if (MapGraph::instance().empty()) {
        QMessageBox::warning(
            this,
            "No Map Loaded",
            "You must load a map before loading queries."
        );
        return;
    }

    timeInQuery = 0;
    const QString filePath = QFileDialog::getOpenFileName(this, "Open Queries File", "", "Text Files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    queriesFilePath = filePath;
    queriesPathLabel->setText(queriesFilePath);

    const auto startInQuery = std::chrono::high_resolution_clock::now();
    if (MapGraph::instance().loadQueriesFromFile(queriesFilePath.toStdString())) {
        displayResult("Queries file loaded successfully.");
    } else {
        displayResult("Error loading queries file.");
        return;
    }

    queryList = MapGraph::instance().getQueries();

    if (queryList.empty()) {
        QMessageBox::warning(this, "Invalid File", "The query file is empty or invalid.");
        return;
    }

    queryIndexEdit->setDisabled(false);
    currentQueryIndex = 0;
    const Query query = queryList[currentQueryIndex];
    const QString resultText = QString::fromStdString(MapGraph::instance().findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R).resultText);
    displayQuery(query, resultText);

    const auto endInQuery = std::chrono::high_resolution_clock::now();
    timeInQuery = std::chrono::duration_cast<std::chrono::milliseconds>(endInQuery - startInQuery).count();
}

void MainWindow::findShortestPath() const {
    if (MapGraph::instance().empty()) {
        displayResult("Error: No map loaded.");
        return;
    }

    bool ok1, ok2, ok3, ok4;
    const double startX = startXEdit->text().toDouble(&ok1);
    const double startY = startYEdit->text().toDouble(&ok2);
    const double endX = endXEdit->text().toDouble(&ok3);
    const double endY = endYEdit->text().toDouble(&ok4);

    if (!ok1 || !ok2 || !ok3 || !ok4) {
        displayResult("Invalid coordinate input.");
        return;
    }

    const double R = REdit->text().toDouble();

    const auto start = std::chrono::high_resolution_clock::now();
    const PathResult pathResult = MapGraph::instance().findShortestPath(startX, startY, endX, endY, R);
    const auto end = std::chrono::high_resolution_clock::now();

    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    QString result = QString::fromStdString(pathResult.resultText);
    result += "\nComputation time: " + QString::number(duration) + " ms";

    displayResult(result);

    MapVisualizer::instance()->update();
}

void MainWindow::onPointsSelected(const double startX, const double startY, const double endX, const double endY) const {
    const double R = REdit->text().toDouble();

    const auto start = std::chrono::high_resolution_clock::now();
    const PathResult pathResult = MapGraph::instance().findShortestPath(startX, startY, endX, endY, R);
    const auto end = std::chrono::high_resolution_clock::now();

    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    QString result = QString::fromStdString(pathResult.resultText);
    result += "\nComputation time: " + QString::number(duration) + " ms";
    
    displayResult(result);
    MapVisualizer::instance()->update();
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
    if (MapGraph::instance().empty()) {
        displayResult("Error: No map loaded.");
        return;
    }
    
    if (queriesFilePath.isEmpty()) {
        displayResult("Error: No queries file loaded.");
        return;
    }
    timeBase = 0;

    if (queryList.empty()) {
        displayResult("No queries to run.");
        return;
    }

    QProgressDialog progressDialog("Processing queries...", QString(), 0, static_cast<int>(queryList.size()), this);
    progressDialog.setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid #555; border-radius: 4px;"
        "  background: #2a2a2a; color: #eee;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #00a755;"
        "}"
    );
    progressDialog.setWindowFlag(Qt::WindowCloseButtonHint, false);
    progressDialog.setCancelButton(nullptr);
    progressDialog.setWindowModality(Qt::ApplicationModal);
    progressDialog.setMinimumDuration(0);
    progressDialog.setValue(0);

    std::atomic_bool cancel{false};
    connect(&progressDialog, &QProgressDialog::canceled, [&]{ cancel = true; });

    const auto future = QtConcurrent::run([this, &cancel, &progressDialog] {
        std::vector<PathResult> localResults;
        localResults.reserve(queryList.size());
        const auto startAll = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < queryList.size(); ++i) {
            if (cancel.load()) break;
            const auto &[startX, startY, endX, endY, R] = queryList[i];
            localResults.push_back(MapGraph::instance().findShortestPath(startX, startY, endX, endY, R));
            QMetaObject::invokeMethod(&progressDialog, [this, &progressDialog, i] {
                progressDialog.setRange(0, static_cast<int>(queryList.size()));
                progressDialog.setValue(static_cast<int>(i + 1));
                progressDialog.setLabelText(QString("Processing %1 / %2 ...").arg(static_cast<int>(i + 1)).arg(static_cast<int>(queryList.size())));
            }, Qt::QueuedConnection);
        }
        const auto endAll = std::chrono::high_resolution_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endAll - startAll).count();
        return std::make_pair(elapsed, std::move(localResults));
    });

    QFutureWatcher<std::pair<long long, std::vector<PathResult>>> watcher;
    connect(&watcher, &QFutureWatcherBase::progressValueChanged, &progressDialog, &QProgressDialog::setValue);
    connect(&watcher, &QFutureWatcherBase::progressRangeChanged, &progressDialog, &QProgressDialog::setRange);
    connect(&watcher, &QFutureWatcherBase::finished, this, [this, &progressDialog, &watcher] {
        auto [fst, snd] = watcher.result();
        timeBase = fst;
        const auto results = std::move(snd);

        saveResults("Output/outputs.txt", results);

        QString resultText;
        resultText += "Executed " + QString::number(results.size()) + " queries in " + 
                      QString::number(timeBase) + " ms\nExecution time + I/O: " +
                      QString::number(timeInMap + timeInQuery + timeBase + timeOut) + " ms\n\n";
        resultText += QString::fromStdString(MapGraph::instance().displayOutput(results));
        
        currentQueryIndex = queryList.size() - 1;
        displayQuery(queryList[currentQueryIndex], resultText);

        progressDialog.close();
    });

    watcher.setFuture(future);
    progressDialog.exec();
}

void MainWindow::displayResult(const QString &result) const {
    outputTextEdit->setText(result);
    // Scroll to the top
    outputTextEdit->verticalScrollBar()->setValue(outputTextEdit->verticalScrollBar()->minimum());
}

void MainWindow::displayQuery(const Query &query, const QString& resultText) const {
    startXEdit->setText(QString::number(query.startX));
    startYEdit->setText(QString::number(query.startY));
    endXEdit->setText(QString::number(query.endX));
    endYEdit->setText(QString::number(query.endY));
    REdit->setText(QString::number(query.R));
    queryIndexEdit->setText(QString::number(currentQueryIndex + 1));

    MapVisualizer::instance()->setStartPoint(query.startX, query.startY);
    MapVisualizer::instance()->setEndPoint(query.endX, query.endY);
    displayResult(resultText);
    MapVisualizer::instance()->update();
}

void MainWindow::handleResetAll() {
    MapVisualizer::instance()->reset();
    if (outputTextEdit) outputTextEdit->clear();
    if (queryIndexEdit) queryIndexEdit->clear();
    if (startXEdit) startXEdit->clear();
    if (startYEdit) startYEdit->clear();
    if (endXEdit) endXEdit->clear();
    if (endYEdit) endYEdit->clear();

    currentQueryIndex = 0;
}

void MainWindow::showLoading(const QString &message)
{
    if (!loadingOverlay) {
        loadingOverlay = new QWidget(this);
        loadingOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        loadingLabel = new QLabel(loadingOverlay);
        loadingLabel->setAlignment(Qt::AlignCenter);
        loadingLabel->setWordWrap(true);
    }
    loadingOverlay->setStyleSheet(QString("background-color: rgba(%1, %1, %1, 128);").arg(MapVisualizer::instance()->getCurrentTheme() == AppTheme::Light ? 255 : 0));
    loadingOverlay->setGeometry(rect());
    loadingOverlay->raise();
    loadingLabel->setStyleSheet(QString("color: %1; font-size: 35px; background: rgba(0, 0, 0, 0);").arg(MapVisualizer::instance()->getCurrentTheme() == AppTheme::Dark ? "white" : "black"));
    loadingLabel->setText(message);
    loadingLabel->setGeometry(0, 0, loadingOverlay->width(), loadingOverlay->height());

    loadingOverlay->show();
    QApplication::processEvents();
}

void MainWindow::hideLoading() const {
    if (loadingOverlay) loadingOverlay->hide();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (loadingOverlay && loadingLabel && loadingOverlay->isVisible()) {
        loadingOverlay->setGeometry(rect());
        loadingLabel->setGeometry(0, 0, loadingOverlay->width(), loadingOverlay->height());
    }
}

void MainWindow::pathFindingTextEdit(const bool noMap) {
    startXEdit->setDisabled(noMap);
    startYEdit->setDisabled(noMap);
    endXEdit->setDisabled(noMap);
    endYEdit->setDisabled(noMap);
    REdit->setDisabled(noMap);
}

void MainWindow::toggleTheme() const {
    MapVisualizer::instance()->toggleTheme();
    updateTheme();
}

void MainWindow::toggleButtonThemeUpdate(const AppTheme theme) const {
    if (theme == AppTheme::Light) {
        themeToggleButton->setText("Toggle Dark Mode");
        themeToggleButton->setStyleSheet(
            "QPushButton {"
            "  padding: 8px; font-size: 12px;"
            "  background-color: #404040;"
            "  color: white;"
            "  border: 1px solid #666;"
            "  border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #505050;"
            "  border-color: #888;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #353535;"
            "}"
        );
    }
    else {
        themeToggleButton->setText("Toggle Light Mode");
        themeToggleButton->setStyleSheet(
            "QPushButton {"
            "  padding: 8px; font-size: 12px;"
            "  background-color: #e8e8e8;"
            "  color: black;"
            "  border: 1px solid #666;"
            "  border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #d4d4d4;"
            "  border-color: #888;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #c0c0c0;"
            "}"
        );
    }
}

void MainWindow::updateTheme() const {
    QString stylesheetPath;
    QPalette palette;
    // Update button text based on the current theme
    if (MapVisualizer::instance()->getCurrentTheme() == AppTheme::Light) {
        toggleButtonThemeUpdate(AppTheme::Light);
        stylesheetPath = "styles/light_theme.qss";
        palette.setColor(QPalette::Window, QColor(0xf0f0f0));
        palette.setColor(QPalette::WindowText, Qt::black);
    } else {
        toggleButtonThemeUpdate(AppTheme::Dark);
        stylesheetPath = "styles/dark_theme.qss";
        palette.setColor(QPalette::Window, QColor(0x353535));
        palette.setColor(QPalette::WindowText, Qt::white);
    }

    if (QFile styleFile(stylesheetPath); styleFile.open(QFile::ReadOnly)) {
        const QString styleSheet = QString::fromLatin1(styleFile.readAll());
        qApp->setStyleSheet(styleSheet);
        styleFile.close();
    } else {
        qDebug() << "Failed to load stylesheet:" << stylesheetPath;
        // Fallback to the previous QPalette approach if stylesheets fail
        if (MapVisualizer::instance()->getCurrentTheme() == AppTheme::Dark) {
            palette.setColor(QPalette::Base, QColor(0x191919));
            palette.setColor(QPalette::AlternateBase, QColor(0x353535));
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor(0x353535));
            palette.setColor(QPalette::ButtonText, Qt::white);
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Link, QColor(0x2a82da));
            palette.setColor(QPalette::Highlight, QColor(0x2a82da));
            palette.setColor(QPalette::HighlightedText, Qt::black);
            palette.setColor(QPalette::PlaceholderText, QColor(0xa0a0a0));
            palette.setColor(QPalette::Shadow, QColor(0x1a1a1a));
            palette.setColor(QPalette::Light, QColor(0x505050));
            palette.setColor(QPalette::Midlight, QColor(0x454545));
            palette.setColor(QPalette::Dark, QColor(0x2a2a2a));
            palette.setColor(QPalette::Mid, QColor(0x303030));
        }
        else {
            palette.setColor(QPalette::Base, Qt::white);
            palette.setColor(QPalette::AlternateBase, QColor(0xe9e7e3));
            palette.setColor(QPalette::Text, Qt::black);
            palette.setColor(QPalette::Button, QColor(0xf0f0f0));
            palette.setColor(QPalette::ButtonText, Qt::black);
            palette.setColor(QPalette::BrightText, Qt::white);
            palette.setColor(QPalette::Link, QColor(0x0000ff));
            palette.setColor(QPalette::Highlight, QColor(0x0078d7));
            palette.setColor(QPalette::HighlightedText, Qt::white);
            palette.setColor(QPalette::PlaceholderText, QColor(0x787878));
            palette.setColor(QPalette::Shadow, QColor(0xc0c0c0));
            palette.setColor(QPalette::Light, Qt::white);
            palette.setColor(QPalette::Midlight, QColor(0xf8f8f8));
            palette.setColor(QPalette::Dark, QColor(0xa0a0a0));
            palette.setColor(QPalette::Mid, QColor(0xd0d0d0));
        }
    }
    QApplication::setPalette(palette);
}
