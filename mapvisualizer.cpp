#include "mapvisualizer.h"
#include "mainwindow.h"
#include <QDebug>

MapVisualizer::MapVisualizer(QWidget *parent)
    : QWidget(parent),
      isStartPointSelected(false),
      nodeDiameter(8),
      pathThickness(2),
      nodeColor(Qt::blue),
      edgeColor(Qt::green),
      pathColor(Qt::red),
      selectedNodeColor(Qt::green),
      startPointColor(Qt::blue),
      endPointColor(Qt::magenta) {
    
    setMinimumSize(400, 400);
    setMouseTracking(true);
}

MapVisualizer::~MapVisualizer() {}

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
        calculateGraphBounds();
    }
    update();
}

void MapVisualizer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Apply zoom (this is the new part)
    painter.translate(offset);
    painter.save();  // Save the original painter state
    painter.scale(scaleFactor, scaleFactor);  // Zoom based on user input

    painter.fillRect(rect(), Qt::black);
    
    if (!mapGraph) {
        painter.drawText(rect(), Qt::AlignCenter, "No map loaded");
        return;
    }

    std::vector<std::pair<double, double>> nodes = mapGraph->getNodes();
    
    // Draw edges
    painter.setPen(QPen(edgeColor, 1));
    for (const auto& edge : mapGraph->getEdges()) {
        const auto& source = nodes[edge.first];
        const auto& dest = nodes[edge.second];
        
        QPointF sourcePoint = transformCoordinates(source.first, source.second);
        QPointF destPoint = transformCoordinates(dest.first, dest.second);
        
        painter.drawLine(sourcePoint, destPoint);
    }
    
    //Draw shortest path if available
    const auto& path = mapGraph->getLastPath();
    if (!path.empty()) {
        painter.setPen(QPen(pathColor, pathThickness));
        
        for (size_t i = 0; i < path.size() - 1; ++i) {
            const auto& source = nodes[path[i]];
            const auto& dest = nodes[path[i+1]];
            
            QPointF sourcePoint = transformCoordinates(source.first, source.second);
            QPointF destPoint = transformCoordinates(dest.first, dest.second);
            
            painter.drawLine(sourcePoint, destPoint);
        }
    }

    // Draw selected points
    if (startPoint.x() != 0 || startPoint.y() != 0) {
        painter.setBrush(startPointColor);
        painter.setPen(Qt::black);
        QPointF transformedStart = transformCoordinates(startPoint.x(), startPoint.y());
        painter.drawEllipse(transformedStart, nodeDiameter, nodeDiameter);
        // Draw centered 'S' text
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(nodeDiameter);
        painter.setFont(font);
        painter.drawText(transformedStart.x()-3, transformedStart.y()+4, "S");
    }
    
    if (endPoint.x() != 0 || endPoint.y() != 0) {
        painter.setBrush(endPointColor);
        painter.setPen(Qt::black);
        QPointF transformedEnd = transformCoordinates(endPoint.x(), endPoint.y());
        painter.drawEllipse(transformedEnd, nodeDiameter, nodeDiameter);
        // Draw centered 'E' text
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(nodeDiameter);
        painter.setFont(font);
        painter.drawText(transformedEnd.x()-3, transformedEnd.y()+4, "E");
    }

    painter.restore();  // Restore painter to original state
}

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
        QPoint delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
    }
}

void MapVisualizer::mouseReleaseEvent(QMouseEvent *event) {
    if (!mapGraph) {
        return;
    }
    
    if (MainWindow::isSelectionEnabled) {
        QPointF clickPoint = inverseTransformCoordinates(event->pos().x(), event->pos().y());
        
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

QPointF MapVisualizer::transformCoordinates(double x, double y) const {
    double width = this->width();
    double height = this->height();
    
    // Apply padding
    double padding = 50;
    width -= 2 * padding;
    height -= 2 * padding;
    
    // Scale coordinates to fit in widget
    double scaleX = width / graphBounds.width();
    double scaleY = height / graphBounds.height();
    
    // Use the smaller scale to maintain aspect ratio
    double scale = qMin(scaleX, scaleY);
    
    // Calculate centered position
    double scaledWidth = graphBounds.width() * scale;
    double scaledHeight = graphBounds.height() * scale;
    
    double offsetX = padding + (width - scaledWidth) / 2;
    double offsetY = padding + (height - scaledHeight) / 2;
    
    // Transform coordinates
    double pixelX = offsetX + (x - graphBounds.left()) * scale;
    double pixelY = offsetY + (graphBounds.height() - (y - graphBounds.top())) * scale;
    
    return QPointF(pixelX, pixelY);
}

QPointF MapVisualizer::inverseTransformCoordinates(int pixelX, int pixelY) const {
    // First, remove the offset to get coordinates relative to the original map position
    QPointF adjustedPoint = QPointF(pixelX, pixelY) - offset;
    
    // Then remove the scale factor
    adjustedPoint /= scaleFactor;
    
    double width = this->width();
    double height = this->height();
    
    // Apply padding
    double padding = 50;
    width -= 2 * padding;
    height -= 2 * padding;
    
    // Scale coordinates to fit in widget
    double scaleX = width / graphBounds.width();
    double scaleY = height / graphBounds.height();
    
    // Use the smaller scale to maintain aspect ratio
    double scale = qMin(scaleX, scaleY);
    
    // Calculate centered position
    double scaledWidth = graphBounds.width() * scale;
    double scaledHeight = graphBounds.height() * scale;
    
    double offsetX = padding + (width - scaledWidth) / 2;
    double offsetY = padding + (height - scaledHeight) / 2;
    
    // Inverse transform
    double x = (adjustedPoint.x() - offsetX) / scale + graphBounds.left();
    double y = graphBounds.top() + graphBounds.height() - (adjustedPoint.y() - offsetY) / scale;
    
    return QPointF(x, y);
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
    
    for (const auto& coords : mapGraph->getNodes()) {
        minX = qMin(minX, coords.first);
        minY = qMin(minY, coords.second);
        maxX = qMax(maxX, coords.first);
        maxY = qMax(maxY, coords.second);
    }
    
    // Add some padding
    double paddingX = (maxX - minX) * 0.1;
    double paddingY = (maxY - minY) * 0.1;
    
    graphBounds = QRectF(
        minX - paddingX,
        minY - paddingY,
        maxX - minX + 2 * paddingX,
        maxY - minY + 2 * paddingY
    );
}

//Mouse Wheel Zooming function
void MapVisualizer::wheelEvent(QWheelEvent *event)
{
    double factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;

    // Optional: zoom centered around the cursor
    QPointF cursorPos = event->position(); // Qt 5.14+
    QPointF beforeScale = (cursorPos - offset) / scaleFactor;

    scaleFactor *= factor;

    QPointF afterScale = beforeScale * scaleFactor;
    offset += (cursorPos - offset) - afterScale;

    double newScale = scaleFactor * factor;
    if (newScale >= 1.0) {  // 1.0 = default size
        scaleFactor = newScale;
        update(); // or repaint/draw logic
    }


    update();
}
