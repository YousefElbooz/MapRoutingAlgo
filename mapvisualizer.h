#ifndef MAPVISUALIZER_H
#define MAPVISUALIZER_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPoint>
#include <memory>
#include <QApplication>
#include <QPalette>
#include "mapgraph.h"

const double sqrt3 = sqrt(3);

// Theme enum for light/dark mode
enum class AppTheme {
    Light,
    Dark
};

class MapVisualizer : public QWidget {
    Q_OBJECT

public:
    // Singleton
    static MapVisualizer* instance(){static auto* instance = new MapVisualizer(nullptr);; return instance; }

    explicit MapVisualizer(QWidget *parent = nullptr);
    ~MapVisualizer() override;

    void setMapGraph();
    void clearSelectionPoints();
    void reset();
    void resetZoom();
    
    // Theme management
    void setTheme(AppTheme theme);
    void toggleTheme();
    [[nodiscard]] AppTheme getCurrentTheme() const { return currentTheme; }

    // Getters for selected points
    [[nodiscard]] QPointF getStartPoint() const { return startPoint; }
    [[nodiscard]] QPointF getEndPoint() const { return endPoint; }
    
    // Setters for points
    void setStartPoint(const double x, const double y) { startPoint = QPointF(x, y); hasStartPoint = true; update(); }
    void setEndPoint(const double x, const double y) { endPoint = QPointF(x, y); hasEndPoint = true; update(); }
    
    // Coordinate transformation
    [[nodiscard]] QPointF transformCoordinates(double x, double y) const;
    [[nodiscard]] QPointF inverseTransformCoordinates(int pixelX, int pixelY) const;

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

    bool hasStartPoint, hasEndPoint;
    QPointF startPoint;
    QPointF endPoint;

    // Drawing parameters
    int nodeDiameter;
    int pathThickness;
    QColor edgeColor;
    QColor pathColor;
    QColor startPointColor;
    QColor endPointColor;
    QColor backgroundColor;
    
    // Theme management
    AppTheme currentTheme;
    void updateThemeColors();

    // Coordinate transformation
    QRectF graphBounds;
    void calculateGraphBounds();

    // Keep scaled content within view
    void clampView();

    Q_DISABLE_COPY(MapVisualizer);
};

#endif // MAPVISUALIZER_H