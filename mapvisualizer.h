#ifndef MAPVISUALIZER_H
#define MAPVISUALIZER_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPoint>
#include <memory>
#include "mapgraph.h"

class MapVisualizer : public QWidget {
    Q_OBJECT

public:
    explicit MapVisualizer(QWidget *parent = nullptr);
    ~MapVisualizer();

    void setMapGraph(std::shared_ptr<MapGraph> graph);
    void clearSelectionPoints();
    void reset();
    
    // Getters for selected points
    QPointF getStartPoint() const { return startPoint; }
    QPointF getEndPoint() const { return endPoint; }
    
    // Coordinate transformation
    QPointF transformCoordinates(double x, double y) const;
    QPointF inverseTransformCoordinates(int pixelX, int pixelY) const;

signals:
    void startPointSelected(double x, double y);
    void endPointSelected(double x, double y);
    void pointsSelected(double startX, double startY, double endX, double endY);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    //Mouse Wheel Zooming In/Out////////////////////////////////////////////////////
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private:
    double scaleFactor = 1.0;  // Zoom level, default = 100%
    QPoint lastMousePos;
    QPointF offset;        // Offset for panning
    bool isPanning = false;

private:
    std::shared_ptr<MapGraph> mapGraph;
    bool isStartPointSelected;
    QPointF startPoint;
    QPointF endPoint;

    // Drawing parameters
    int nodeDiameter;
    int pathThickness;
    QColor nodeColor;
    QColor edgeColor;
    QColor pathColor;
    QColor selectedNodeColor;
    QColor startPointColor;
    QColor endPointColor;

    // Coordinate transformation
    QRectF graphBounds;
    void calculateGraphBounds();
};

#endif // MAPVISUALIZER_H 
