#include "mapvisualizer.h"
#include "mainwindow.h"

MapVisualizer::MapVisualizer(QWidget *parent)
    : QWidget(parent),
      isStartPointSelected(false),
      nodeDiameter(10),
      pathThickness(3),
      startPointColor(Qt::blue),
      endPointColor(Qt::red),
      currentTheme(AppTheme::Dark) {

    setMinimumSize(400, 400);
    setMouseTracking(true);
    updateThemeColors();
}

MapVisualizer::~MapVisualizer() = default;

void MapVisualizer::setMapGraph(const std::shared_ptr<MapGraph> &graph) {
    mapGraph = graph;
    
    if (mapGraph) {
        calculateGraphBounds();
    }
    
    update();
}

void MapVisualizer::clearSelectionPoints() {
    isStartPointSelected = false;
    startPoint = QPointF();
    endPoint = QPointF();
    update();
}

void MapVisualizer::reset() {
    clearSelectionPoints();
    if (mapGraph) {
        mapGraph->clearLastPath();
        calculateGraphBounds();
    }
    // Reset zoom to default level
    resetZoom();
}

void MapVisualizer::resetZoom() {
    scaleFactor = 1.0;
    offset = QPointF(0, 0);
    clampView();
    update();
}

void MapGraph::clearLastPath() {
    lastPath.clear();
}

void MapVisualizer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Apply zoom (this is the new part)
    painter.translate(offset);
    painter.save();  // Save the original painter state
    painter.scale(scaleFactor, scaleFactor);  // Zoom based on user input

    painter.fillRect(rect(), backgroundColor);
    
    if (!mapGraph) {
        // Set text color based on theme for better contrast
        painter.setPen(currentTheme == AppTheme::Light ? Qt::black : Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "No map loaded");
        return;
    }

    const std::vector<std::pair<double, double>> nodes = mapGraph->getNodes();
    
    // Draw edges with theme-appropriate thickness
    float edgeThickness = (currentTheme == AppTheme::Light) ? 1.5/scaleFactor : 1/scaleFactor;
    painter.setPen(QPen(edgeColor, edgeThickness));
    
    for (const auto&[edgeStart, edgeEnd] : mapGraph->getEdges()) {
        const auto&[sourceX, sourceY] = nodes[edgeStart];
        const auto&[destX, destY] = nodes[edgeEnd];
        
        QPointF sourcePoint = transformCoordinates(sourceX, sourceY);
        QPointF destPoint = transformCoordinates(destX, destY);
        
        painter.drawLine(sourcePoint, destPoint);
    }
    
    // Draw the shortest path if available
    if (const auto& path = mapGraph->getLastPath(); !path.empty()) {
        float currentPathThickness = (currentTheme == AppTheme::Light) ? 
            (pathThickness + 1)/scaleFactor : pathThickness/scaleFactor;
            
        painter.setPen(QPen(pathColor, currentPathThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        for (size_t i = 0; i < path.size() - 1; ++i) {
            const auto&[sourceX, sourceY] = nodes[path[i]];
            const auto&[destX, destY] = nodes[path[i+1]];

            QPointF sourcePoint = transformCoordinates(sourceX, sourceY);
            QPointF destPoint = transformCoordinates(destX, destY);

            painter.drawLine(sourcePoint, destPoint);
        }
    }

    // Draw selected points
    if (startPoint.x() != 0 || startPoint.y() != 0) {
        painter.setBrush(startPointColor);
        painter.setPen(QPen(Qt::black, 2/scaleFactor));
        const QPointF transformedStart = transformCoordinates(startPoint.x(), startPoint.y());
        painter.drawEllipse(transformedStart, nodeDiameter/scaleFactor, nodeDiameter/scaleFactor);
    }

    if (endPoint.x() != 0 || endPoint.y() != 0) {
        painter.setBrush(endPointColor);
        painter.setPen(QPen(Qt::black, 2/scaleFactor));
        const QPointF transformedEnd = transformCoordinates(endPoint.x(), endPoint.y());
        const double radius = (nodeDiameter-4)/scaleFactor;
        auto *trianglePoints = new QPointF[3];
        trianglePoints[0] = QPointF(transformedEnd.x()-radius*sqrt3, transformedEnd.y()-radius);
        trianglePoints[1] = QPointF(transformedEnd.x()+radius*sqrt3, transformedEnd.y()-radius);
        trianglePoints[2] = QPointF(transformedEnd.x(), transformedEnd.y()+radius*3);
        painter.drawPolygon(trianglePoints, 3);
        painter.drawEllipse(QPointF((trianglePoints[0].x()+trianglePoints[1].x())/2, trianglePoints[0].y()), abs(trianglePoints[0].x()-trianglePoints[1].x())/2, abs(trianglePoints[0].x()-trianglePoints[1].x())/4);
        delete[] trianglePoints;
    }

    painter.restore();
}

// Theme management methods
void MapVisualizer::setTheme(AppTheme theme) {
    currentTheme = theme;
    updateThemeColors();
    update();
}

void MapVisualizer::toggleTheme() {
    currentTheme = (currentTheme == AppTheme::Light) ? AppTheme::Dark : AppTheme::Light;
    updateThemeColors();
    update();
}

void MapVisualizer::updateThemeColors() {
    if (currentTheme == AppTheme::Light) {
        backgroundColor = Qt::white;
        edgeColor = QColor(0x003F00);
        pathColor = Qt::red;
    } else { // Dark theme
        backgroundColor = Qt::black;
        edgeColor = QColor(0x00EF00);
        pathColor = Qt::cyan;
    }
}

// Rest of your existing methods remain the same...
void MapVisualizer::mousePressEvent(QMouseEvent *event)
{
    if (!MainWindow::isSelectionEnabled && event->button() == Qt::LeftButton) {
        isPanning = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    else {
        isPanning = false;
    }
}

void MapVisualizer::mouseMoveEvent(QMouseEvent *event)
{
    if (!MainWindow::isSelectionEnabled && isPanning) {
        const QPoint delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        clampView();
        update();
    }
}

void MapVisualizer::mouseReleaseEvent(QMouseEvent *event) {
    if (!mapGraph) {
        return;
    }
    
    if (MainWindow::isSelectionEnabled) {
        const QPointF clickPoint = inverseTransformCoordinates(event->pos().x(), event->pos().y());
        
        if (!isStartPointSelected) {
            startPoint = clickPoint;
            isStartPointSelected = true;
            emit startPointSelected(clickPoint.x(), clickPoint.y());
        } else {
            endPoint = clickPoint;
            isStartPointSelected = false;
            emit endPointSelected(clickPoint.x(), clickPoint.y());
            emit pointsSelected(startPoint.x(), startPoint.y(), endPoint.x(), endPoint.y());
        }
    }
    
    if (event->button() == Qt::LeftButton) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
    }

    update();
}

QPointF MapVisualizer::transformCoordinates(const double x, const double y) const {
    double width = this->width();
    double height = this->height();
    
    // Apply padding
    constexpr double padding = 50;
    width -= 2 * padding;
    height -= 2 * padding;
    
    // Scale coordinates to fit in the widget
    const double scaleX = width / graphBounds.width();
    const double scaleY = height / graphBounds.height();

    // Use the smaller scale to maintain the aspect ratio
    const double scale = qMin(scaleX, scaleY);
    
    // Calculate centered position
    const double scaledWidth = graphBounds.width() * scale;
    const double scaledHeight = graphBounds.height() * scale;

    const double offsetX = padding + (width - scaledWidth) / 2;
    const double offsetY = padding + (height - scaledHeight) / 2;
    
    // Transform coordinates
    double pixelX = offsetX + (x - graphBounds.left()) * scale;
    double pixelY = offsetY + (graphBounds.height() - (y - graphBounds.top())) * scale;
    
    return {pixelX, pixelY};
}

QPointF MapVisualizer::inverseTransformCoordinates(const int pixelX, const int pixelY) const {
    // First, remove the offset to get coordinates relative to the original map position
    QPointF adjustedPoint = QPointF(pixelX, pixelY) - offset;

    // Then remove the scale factor
    adjustedPoint /= scaleFactor;
    
    double width = this->width();
    double height = this->height();
    
    // Apply padding
    constexpr double padding = 50;
    width -= 2 * padding;
    height -= 2 * padding;
    
    // Scale coordinates to fit in the widget
    const double scaleX = width / graphBounds.width();
    const double scaleY = height / graphBounds.height();

    // Use the smaller scale to maintain the aspect ratio
    const double scale = qMin(scaleX, scaleY);
    
    // Calculate centered position
    const double scaledWidth = graphBounds.width() * scale;
    const double scaledHeight = graphBounds.height() * scale;

    const double offsetX = padding + (width - scaledWidth) / 2;
    const double offsetY = padding + (height - scaledHeight) / 2;
    
    // Inverse transform
    double x = (adjustedPoint.x() - offsetX) / scale + graphBounds.left();
    double y = graphBounds.top() + graphBounds.height() - (adjustedPoint.y() - offsetY) / scale;
    
    return {x, y};
}

void MapVisualizer::calculateGraphBounds() {
    if (!mapGraph || mapGraph->getNodes().empty()) {
        graphBounds = QRectF(0, 0, 1, 1);
        return;
    }
    
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    
    for (const auto&[x, y] : mapGraph->getNodes()) {
        minX = qMin(minX, x);
        minY = qMin(minY, y);
        maxX = qMax(maxX, x);
        maxY = qMax(maxY, y);
    }
    
    // Add some padding
    const double paddingX = (maxX - minX) * 0.1;
    const double paddingY = (maxY - minY) * 0.1;
    
    graphBounds = QRectF(
        minX - paddingX,
        minY - paddingY,
        maxX - minX + 2 * paddingX,
        maxY - minY + 2 * paddingY
    );
}

//Mouse Wheel Zooming function
void MapVisualizer::wheelEvent(QWheelEvent *event) {
    const double factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;

    // Optional: zoom centered around the cursor
    const QPointF cursorPos = event->position();
    const QPointF beforeScale = (cursorPos - offset) / scaleFactor;

    double newScale = scaleFactor * factor;
    if (newScale < 1.0) newScale = 1.0;
    scaleFactor = newScale;

    const QPointF afterScale = beforeScale * scaleFactor;
    offset += (cursorPos - offset) - afterScale;

    clampView();
    update();
}

void MapVisualizer::clampView() {
    const double minX = static_cast<double>(width()) * (1.0 - scaleFactor);
    const double minY = static_cast<double>(height()) * (1.0 - scaleFactor);

    if (offset.x() < minX) offset.setX(minX);
    if (offset.x() > 0.0) offset.setX(0.0);
    if (offset.y() < minY) offset.setY(minY);
    if (offset.y() > 0.0) offset.setY(0.0);
}