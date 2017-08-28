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
#ifndef MainWindow_H
#define MainWindow_H

#include <QMainWindow>
#include <QRectF>
#include <QCloseEvent>
#include <QStringList>
#include <QVector>
#include <QList>
#include <QColor>
#include "ui_MainWindow.h"
#include "PixmapWidget.h"
#include "ImgAnnotation.h"
#include "ScrollAreaNoWheel.h"


class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    enum Direction { Up, Down };

public:
    MainWindow(QWidget *parent = 0, QFlag flags = 0);
    QString get_mask_file(int obj_id, QString img_file) const;
    QString get_current_direction() const;
    QString get_current_file() const;
    QString get_current_obj_file();
    int get_current_obj_id() const;

protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);
    void wheelEvent(QWheelEvent *event);

private:
    void show_mask_error_message_i();
    void save_mask_i();
    void update_undo_redo_menu();
    std::map<int, QString> get_mask_files();
    void refresh_img_tree_i();
    void refresh_obj_mask_i();
    void switch_img_file(Direction);

private slots:
    void on_actionOpenDir_triggered();
    void on_actionQuit_triggered();
    void on_actionShortcutHelp_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();

    void on_imgTreeWidget_currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *);

    void on_transparencySlider_valueChanged(int i);
    void on_objTypeComboBox_currentIndexChanged(int);
    void on_brushSizeComboBox_currentIndexChanged(int);

    void on_confidenceCheckBox_stateChanged(int);

    void slot_wheel_turned_in_scroll_area_i(QWheelEvent *);
    void slot_mask_draw_i(QImage *mask);

private:
    PixmapWidget *_pixmap_widget;
    ScrollAreaNoWheel *_scroll_area;

    QString _current_opened_direction;
    std::map<int , QString> _current_obj_file_collection;

    QVector<QRgb> _color_table;

    QVector<int> brushSizes;

    QList<QImage> _img_undo_history;
    int _current_history_img;
    int _max_history_step;

    bool _is_key_shift_pressed;
    bool _is_key_ctrl_pressed;
};

#endif
