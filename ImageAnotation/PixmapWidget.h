/**
* The Image Annotation Tool for image annotations with pixelwise masks
*
* Copyright (C) 2007 Alexander Klaeser
*
* http://lear.inrialpes.fr/people/klaeser/
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef PIXMAPWIDGET_H
#define PIXMAPWIDGET_H

#include <QtOpenGL/qgl.h>
#include <QString>
#include <QPixmap>
#include <QImage>
#include <QList>
#include <QRect>
#include <QMouseEvent>
#include <QMatrix>

#define MARGIN 5


class QAbstractScrollArea;
class QMainWindow;

// defines a bounding box that is to be drawn on the screen
class BoundingBox
{
public:
    QRectF box;
    QList<QPointF> fixPoints;
    double score;
};


// our own pixmap widget .. which displays an image and a annotation mask
class PixmapWidget : public QGLWidget
{
    Q_OBJECT

public:
    PixmapWidget(QAbstractScrollArea*, QWidget *parent=0);
    virtual ~PixmapWidget();
    const QImage& get_draw_mask() const;

    void enable_painting(bool flag);

    void set_pixmap(const QPixmap&);
    void set_mask(QImage&);
    void set_confidence(bool flag);

    void set_pen_width(int width);
    void set_mask_transparency(double transparency);

public slots:
    void slot_zoom_factor_changed(double);
    

signals:
    void zoomFactorChanged(double);
    void pixmapChanged(QPixmap*);
    void maskChanged(QImage*);

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void updateGL();

    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);

private:
    void updateMouseCursor();
    void setup_current_painter_i(QPainter &painter);

private:
    QPixmap *_pixmap;
    QImage _drawMask;

    double _zoom_factor;
    double _mask_transparency;
    int _pen_width;

    QMatrix _current_matrix_inv;
    QMatrix _current_matrix;
    QPoint lastXyMouseOrg;
    QPoint lastXyMouse;
    QPoint lastXyDrawnMouseOrg;
    QPoint lastXyDrawnMouse;
    QPoint xyMouseFollowed;

    bool _is_drawing;

    double _last_v_scroll_value;
    double _last_h_scroll_value;
    QAbstractScrollArea *_scroll_area;
    QMainWindow *_parent_window;

    bool _enable_painting;
    bool _is_confident;
    bool _is_erasing;
};

#endif // PIXMAPWIDGET_H
