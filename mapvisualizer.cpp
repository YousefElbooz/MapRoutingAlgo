#include "mapvisualizer.h"
#include <QDebug>

MapVisualizer::MapVisualizer(QWidget *parent)
    : QWidget(parent),
      isStartPointSelected(false),
      nodeDiameter(0.25),
      pathThickness(100),
      nodeColor(Qt::blue),
      edgeColor(Qt::gray),
      pathColor(Qt::red),
      selectedNodeColor(Qt::green),
      startPointColor(Qt::green),
      endPointColor(Qt::red) {
    
    setMinimumSize(400, 400);
    setMouseTracking(true);
}

MapVisualizer::~MapVisualizer() {}

void MapVisualizer::setMapGraph(std::shared_ptr<MapGraph> graph) {
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
    
    // Fill background
    painter.fillRect(rect(), Qt::white);
    
    if (!mapGraph) {
        painter.drawText(rect(), Qt::AlignCenter, "No map loaded");
        return;
    }
    
    // Draw edges
    painter.setPen(QPen(edgeColor, 1));
    for (const auto& edge : mapGraph->getEdges()) {
        const auto& source = mapGraph->getNodes()[edge.source];
        const auto& dest = mapGraph->getNodes()[edge.destination];
        
        QPointF sourcePoint = transformCoordinates(source.x, source.y);
        QPointF destPoint = transformCoordinates(dest.x, dest.y);
        
        painter.drawLine(sourcePoint, destPoint);
    }
    
    // Draw shortest path if available
    const auto& path = mapGraph->getLastPath();
    if (!path.empty()) {
        painter.setPen(QPen(pathColor, pathThickness));
        
        for (size_t i = 0; i < path.size() - 1; ++i) {
            const auto& source = mapGraph->getNodes()[path[i]];
            const auto& dest = mapGraph->getNodes()[path[i+1]];
            
            QPointF sourcePoint = transformCoordinates(source.x, source.y);
            QPointF destPoint = transformCoordinates(dest.x, dest.y);
            
            painter.drawLine(sourcePoint, destPoint);
        }
    }
    
    // Draw nodes
    for (const auto& node : mapGraph->getNodes()) {
        QPointF center = transformCoordinates(node.x, node.y);
        
        // Check if this node is in the path
        bool isInPath = false;
        for (const auto& pathNode : path) {
            if (node.id == pathNode) {
                isInPath = true;
                break;
            }
        }
        
        if (isInPath) {
            painter.setBrush(selectedNodeColor);
        } else {
            painter.setBrush(nodeColor);
        }
        
        painter.setPen(Qt::black);
        painter.drawEllipse(center, nodeDiameter / 2, nodeDiameter / 2);
    }
    
    // Draw selected points
    if (startPoint.x() != 0 || startPoint.y() != 0) {
        painter.setBrush(startPointColor);
        painter.setPen(Qt::black);
        QPointF transformedStart = transformCoordinates(startPoint.x(), startPoint.y());
        painter.drawEllipse(transformedStart, nodeDiameter / 2, nodeDiameter / 2);
        painter.drawText(transformedStart.x() - 4, transformedStart.y() - 10, "S");
    }
    
    if (endPoint.x() != 0 || endPoint.y() != 0) {
        painter.setBrush(endPointColor);
        painter.setPen(Qt::black);
        QPointF transformedEnd = transformCoordinates(endPoint.x(), endPoint.y());
        painter.drawEllipse(transformedEnd, nodeDiameter / 2, nodeDiameter / 2);
        painter.drawText(transformedEnd.x() - 4, transformedEnd.y() - 10, "E");
    }
}

void MapVisualizer::mouseReleaseEvent(QMouseEvent *event) {
    if (!mapGraph) {
        return;
    }
    
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
    double x = (pixelX - offsetX) / scale + graphBounds.left();
    double y = graphBounds.top() + graphBounds.height() - (pixelY - offsetY) / scale;
    
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
    
    for (const auto& node : mapGraph->getNodes()) {
        minX = qMin(minX, node.x);
        minY = qMin(minY, node.y);
        maxX = qMax(maxX, node.x);
        maxY = qMax(maxY, node.y);
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
