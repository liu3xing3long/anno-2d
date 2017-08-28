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
#include "MainWindow.h"
#include <iostream>
#include <string>

#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QMessageBox>
#include <stdio.h>
#include "ScrollAreaNoWheel.h"
#include <QtDebug>
#include <QMessageBox>
#include <QImageReader>
#include <QTextCodec>

#include "defines.h"

#define MASK_TPYE_NUM 10
static const std::string S_mask_types[MASK_TPYE_NUM] = 
{
    "microaneurysms" , 
    "exudates",
    "hemorrhages",
    "cotton wool spots",
    "venous beading",
    "neovascularization",
    "IMRA",
    "hemorrhages spot",
    "artery",
    "vein"
};


bool maskFileLessThan(const QString &f1, const QString &f2)
{
    // mask file name looks like: <imageFileName>.mask.<Number>.<extension>
    // compare front part
    QString front1 = f1.section(".", 0, -4);
    QString front2 = f2.section(".", 0, -4);
    if (front1 != front2)
    {
        return front1 < front2;
    }

    // compare numbers
    QString strNum1 = f1.section(".", -2, -2);
    QString strNum2 = f2.section(".", -2, -2);
    bool ok1;
    int num1 = strNum1.toInt(&ok1);
    bool ok2;
    int num2 = strNum2.toInt(&ok2);

    if (!ok1 || num1 < 0)
        return false;
    if (!ok2 || num2 < 0)
        return true;
    return num1 < num2;
}


MainWindow::MainWindow(QWidget *parent, QFlag flags)
    : QMainWindow(parent, flags)
{
    // set up the UI
    setupUi(this);

    _scroll_area = new ScrollAreaNoWheel(this);
    _pixmap_widget = new PixmapWidget(_scroll_area, this);
    
    _scroll_area->setWidgetResizable(true);
    _scroll_area->setWidget(_pixmap_widget);
    setCentralWidget(_scroll_area);
    _is_key_shift_pressed = false;
    _is_key_ctrl_pressed = false;

    // some hard coded data
    _color_table << qRgb(0, 0, 0); // background
    _color_table << qRgb(255, 0, 0); // confident object
    _color_table << qRgb(0, 255, 0); // unconfident object
    

    brushSizes << 1 << 3 << 5 << 7 << 9 << 11 << 13 << 15 << 18 << 20 << 25 << 30 << 50 << 100;
    _max_history_step = 10;
    _current_history_img = 0;

    for (int i = 0; i < brushSizes.size(); i++)
        brushSizeComboBox->addItem("Circle (" + QString::number(brushSizes[i]) + "x" + QString::number(brushSizes[i]) + ")");


    // we want to receive key events, therefore we have to set the focus policy
    setFocusPolicy(Qt::WheelFocus);

    // make some connections
    connect(_pixmap_widget, SIGNAL(maskChanged(QImage *)), this, SLOT(slot_mask_draw_i(QImage *)));
    connect(zoomSpinBox, SIGNAL(valueChanged(double)), _pixmap_widget, SLOT(slot_zoom_factor_changed(double)));
    connect(_pixmap_widget, SIGNAL(zoomFactorChanged(double)), zoomSpinBox, SLOT(setValue(double)));
    connect(_scroll_area, SIGNAL(wheelTurned(QWheelEvent*)), this, SLOT(slot_wheel_turned_in_scroll_area_i(QWheelEvent *)));

    // set some default values
    brushSizeComboBox->setCurrentIndex(1);

    //set objTypeComboBox item
    QTextCodec::setCodecForTr(QTextCodec::codecForLocale());
    QString mask_type_tool_tips[MASK_TPYE_NUM] = 
    {
        tr("Î¢Ð¡Ñª¹ÜÁö") , 
        tr("Ó²ÐÔÉø³öÎï"),
        tr("³öÑª°ß"),
        tr("ÃÞÈÞ°ß"),
        tr("¾²Âö´®Öé"),
        tr("ÐÂÉúÑª¹Ü"),
        tr("Î¢Ñª¹ÜÒì³£"),
        tr("³öÑª°ß"),
        tr("¶¯Âö"),
        tr("¾²Âö")
    };

    objTypeComboBox->setToolTip(tr("²¡ÔîÃû³Æ"));
    objTypeComboBox->clear();
    for (int i = 0 ; i< MASK_TPYE_NUM ; ++i)
    {
        objTypeComboBox->addItem(QString(S_mask_types[i].c_str()));
        objTypeComboBox->setItemData(i , mask_type_tool_tips[i] ,Qt::ToolTipRole);
    }
}

QString MainWindow::get_mask_file(int obj_id, QString img_file) const
{
    return img_file.replace(".image.", ".").section(".", 0, -2) + ".mask." + QString(S_mask_types[obj_id].c_str()) + ".png";
}

QString MainWindow::get_current_direction() const
{
    QTreeWidgetItem *current = imgTreeWidget->currentItem();
    if (!current || !current->parent())
    {
        return "";
    }
    else
    {
        QString dir = current->parent()->text(0);
        return dir;
    }
}

QString MainWindow::get_current_file() const
{
    QTreeWidgetItem *current = imgTreeWidget->currentItem();
    if (!current || !current->parent())
    {
        return "";
    }
    else
    {
        return current->text(0);
    }
}

QString MainWindow::get_current_obj_file()
{
    const int iObj = get_current_obj_id();
    if (iObj < 0)
    {
        return QString("");
    }
    else
    {
        return _current_obj_file_collection[iObj];
    }
}

int MainWindow::get_current_obj_id() const
{
    QString file = get_current_file();
    QString dir = get_current_direction();
    if (file.isEmpty() || dir.isEmpty())
    {
        return -1;
    }
    else
    {
        return objTypeComboBox->currentIndex();
    }
}


void MainWindow::slot_wheel_turned_in_scroll_area_i(QWheelEvent *event)
{
    wheelEvent(event);
}

void MainWindow::on_actionOpenDir_triggered()
{
    // clear the status bar and set the normal mode for the pixmapWidget
    statusBar()->clearMessage();

    // ask the user to add files
    QString opened_dir = QFileDialog::getExistingDirectory(this, "Choose a directory to be read in", _current_opened_direction);

    if (opened_dir.isEmpty())
    {
        return;
    }

    // save the opened path
    _current_opened_direction = opened_dir;

    // read in the directory structure
    refresh_img_tree_i();

    // update the window title
    setWindowTitle("ImageAnnotation - " + opened_dir);

    // the ctrl key rests
    _is_key_ctrl_pressed = false;
    _is_key_shift_pressed = false;

    // update the statusbar
    statusBar()->showMessage("Opened directory structure " + opened_dir, 5 * 1000);
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionShortcutHelp_triggered()
{
    // clear the status bar and set the normal mode for the pixmapWidget
    statusBar()->clearMessage();

    // we display an overview on shortcuts
    QMessageBox::about(this, "Shortcut Help",
        "<table border=0 cellpadding=0 cellspacing=2>\n"
        "<tr>\n"
        "<td><b>Left Mouse Button</b></td>\n"
        "<td width=10></td>\n"
        "<td>draw in the chosen color</td>\n"
        "</tr><tr>\n"
        "<td><b>Right Mouse Button</b></td>\n"
        "<td width=10></td>\n"
        "<td>erase current point</td>\n"
        "</tr><tr>\n"
        "<td><b>1, ..., 9</b></td>\n"
        "<td></td>\n"
        "<td>choose brush size from drop down box</td>\n"
        "</tr><tr>\n"
        "<td><b>MouseWheel Up/Down</b></td>\n"
        "<td></td>\n"
        "<td>zoom out/in</td>\n"
        "</tr><tr>\n"
        "<td><b>Ctrl+MouseWheel Up/Down</b></td>\n"
        "<td></td>\n"
        "<td>increase/decrease brush size</td>\n"
        "</tr><tr>\n"
        "<td><b>Shift+MouseWheel Up/Down</b></td>\n"
        "<td></td>\n"
        "<td>go to the previous/next file in the file list</td>\n"
        "</tr>\n"
        "</table>\n");
}

void MainWindow::on_actionUndo_triggered()
{
    if (_current_history_img < _img_undo_history.size() - 1 && _img_undo_history.size() > 1) {
        // get the name of the mask image file
        QString iFile = get_current_file();
        QString iDir = get_current_direction();
        QString iMask = get_current_obj_file();
        if (iFile.isEmpty() || iDir.isEmpty() || iMask.isEmpty())
            return;
        QString objMaskFilename = get_mask_file(get_current_obj_id(), iFile);

        // save the image from the history
        _current_history_img++;
        if (!_img_undo_history[_current_history_img].save(_current_opened_direction + iDir + "/" + objMaskFilename, "PNG")) {
            show_mask_error_message_i();
            return;
        }

        refresh_obj_mask_i();
        update_undo_redo_menu();
    }
}

void MainWindow::on_actionRedo_triggered()
{
    if (_current_history_img > 0 && _current_history_img < _img_undo_history.size() && _img_undo_history.size() > 1) {
        // get the name of the mask image file
        QString iFile = get_current_file();
        QString iDir = get_current_direction();
        QString iMask = get_current_obj_file();
        if (iFile.isEmpty() || iDir.isEmpty() || iMask.isEmpty())
            return;
        QString objMaskFilename = get_mask_file(get_current_obj_id(), iFile);

        // save the image from the history
        _current_history_img--;
        if (!_img_undo_history[_current_history_img].save(_current_opened_direction + iDir + "/" + objMaskFilename, "PNG")) {
            show_mask_error_message_i();
            return;
        }

        refresh_obj_mask_i();
        update_undo_redo_menu();
    }
}

void MainWindow::on_objTypeComboBox_currentIndexChanged(int obj_id)
{
    if (obj_id<0)
    {
        _pixmap_widget->enable_painting(false);
    }
    else
    {
        _pixmap_widget->enable_painting(true);

        QString iFile = get_current_file();
        QString iDir = get_current_direction();
        if (iFile.isEmpty() || iDir.isEmpty() || obj_id < 0)
        {
            return;
        }

        //Check empty mask file
        const bool empty_obj_file = _current_obj_file_collection.find(obj_id) == _current_obj_file_collection.end();
        // create a new segmentation mask
        if (empty_obj_file)
        {
            QImage orgImg(_current_opened_direction + iDir + "/" + iFile);
            QImage mask(orgImg.size(), QImage::Format_Indexed8);
            mask.setColorTable(_color_table);
            mask.fill(BACKGROUND);
            //mask.setText("annotationObjType", objTypes[0]);
            QString objMaskFilename = get_mask_file(obj_id, iFile);
            if (!mask.save(_current_opened_direction + iDir + "/" + objMaskFilename, "PNG")) 
            {
                show_mask_error_message_i();
                return;
            }
            _current_obj_file_collection[obj_id] = objMaskFilename;
        }


        // refresh
        refresh_obj_mask_i();

        // clear the history and append the current mask
        _img_undo_history.clear();
        QImage maskImg = _pixmap_widget->get_draw_mask().copy();
        _img_undo_history.push_front(maskImg);
        _current_history_img = 0;
        update_undo_redo_menu();

    }
}

void MainWindow::on_imgTreeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    // check weather dir/file/object have been selected
    QString iFile = get_current_file();
    QString iDir = get_current_direction();
    if (iFile.isEmpty() || iDir.isEmpty())
        return;

    // check weather we have a relative or absolute path
    QString absoluteDir;
    if (iDir[0] != '/')
        absoluteDir = _current_opened_direction;

    // load new file
    QString filepath(absoluteDir + iDir + "/" + iFile);
    _pixmap_widget->enable_painting(false);
    _pixmap_widget->set_pixmap(QPixmap(filepath  ));

     //get mask file
    get_mask_files();
    if (_current_obj_file_collection.empty())
    {
        objTypeComboBox->blockSignals(true);
        on_objTypeComboBox_currentIndexChanged(objTypeComboBox->currentIndex());
        _pixmap_widget->enable_painting(true);
        objTypeComboBox->blockSignals(false);
    }
    else
    {
        const int pre_obj_id = objTypeComboBox->currentIndex();
        if (_current_obj_file_collection.find(pre_obj_id) == _current_obj_file_collection.end())
        {
            objTypeComboBox->blockSignals(true);
            on_objTypeComboBox_currentIndexChanged(pre_obj_id);
            objTypeComboBox->blockSignals(false);
        }
        _pixmap_widget->enable_painting(true);
    }

    _current_history_img = 0;

    refresh_obj_mask_i();
}


void MainWindow::on_brushSizeComboBox_currentIndexChanged(int i)
{
    if (i < 0 || i >= brushSizes.size())
        return;
    _pixmap_widget->set_pen_width(brushSizes[i]);
}

void MainWindow::on_transparencySlider_valueChanged(int i)
{
    _pixmap_widget->set_mask_transparency(((double)i) / transparencySlider->maximum());
}

void MainWindow::slot_mask_draw_i(QImage *mask)
{
    // check weather dir/file/object have been selected
    QString iFile = get_current_file();
    QString iDir = get_current_direction();
    int iObj = get_current_obj_id();
    if (iFile.isEmpty() || iDir.isEmpty() || iObj < 0)
        return;

    // save the mask
    // should only save mask once when finish mask draw
    save_mask_i();
}

void MainWindow::refresh_img_tree_i()
{
    QList<QByteArray> sup_imgs = QImageReader::supportedImageFormats();
    std::cout << "Support format : ";
    std::list<QByteArray> std_sup_imgs = sup_imgs.toStdList();
    for (auto it = std_sup_imgs.begin() ; it != std_sup_imgs.end() ; ++it)
    {
        std::cout << (*it).data() << " , ";
    }
    std::cout << std::endl;

    // clear all items
    imgTreeWidget->clear();

    // read in the currently opened directory structure recursively
    QStringList dirs;
    QStringList relativeDirStack;
    relativeDirStack << "."; // start with the main dir
    while (!relativeDirStack.empty()) {
        QString nextDirStr = relativeDirStack.first();
        relativeDirStack.pop_front();
        dirs << nextDirStr;

        // get all directories in the current directory
        QDir currentDir(_current_opened_direction + nextDirStr);
        QStringList dirList = currentDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        for (int i = 0; i < dirList.size(); i++) {
            relativeDirStack << nextDirStr + "/" + dirList[i];
        }
    }

    // read in all images in all collected directories
    // add all files and directories to the QTreeWidget
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.png" << "*.bmp" << "*.jpeg" << "*.tif" << "*.gif" << "*.tiff" << "*.pbm" << "*.pgm" << "*.ppm" << "*.xbm" << "*.xpm";
    for (int i = 0; i < dirs.size(); i++) {
        // get all images in the current directory
        QDir currentDir(_current_opened_direction + dirs[i]);
        currentDir.setFilter(QDir::Files);
        currentDir.setNameFilters(nameFilters);
        QStringList files = currentDir.entryList();

        // ignore the directory if we couldn't find any image file in it
        if (files.size() <= 0)
            continue;

        // construct a new directory entry
        QTreeWidgetItem *currentTreeDir = new QTreeWidgetItem(imgTreeWidget);
        imgTreeWidget->setItemExpanded(currentTreeDir, true);
        currentTreeDir->setText(0, dirs[i]);

        // construct new entries for the image files
        for (int j = 0; j < files.size(); j++) {
            // make sure that the image file is not a mask
            if (files[j].contains(".mask."))
                continue;

            // construct a new entry
            QTreeWidgetItem *currentFile = new QTreeWidgetItem(currentTreeDir);
            currentFile->setText(0, files[j]);
        }
    }

    // sort the files + directories
    imgTreeWidget->sortItems(0, Qt::AscendingOrder);
}

void MainWindow::refresh_obj_mask_i()
{
    // check weather dir/file/object have been selected
    QString iFile = get_current_file();
    QString iDir = get_current_direction();
    int iObj = get_current_obj_id();
    if (iFile.isEmpty() || iDir.isEmpty() || iObj < 0)
    {
        return;
    }

    //if (iObj < 0) {
    //    // set an empty mask if no mask exists
    //    QImage tmp_org_img(currentlyOpenedDir + iDir + "/" + iFile);
    //    QImage emptyMask;
    //    pixmapWidget->setMask(emptyMask);
    //}
    //else 
    {
        // load the mask
        QImage mask(_current_opened_direction + iDir + "/" + get_current_obj_file());
        // convert binary masks
        //if (mask.colorCount() == 2) 
        //{
        //    QImage newMask(mask.size(), QImage::Format_Indexed8);
        //    newMask.setColorTable(_color_table);
        //    for (int y = 0; y < newMask.height(); y++)
        //    {
        //        for (int x = 0; x < newMask.width(); x++)
        //        {
        //            newMask.setPixel(x, y, mask.pixelIndex(x, y));
        //        }
        //        mask = newMask;
        //    }
        //}

        _pixmap_widget->set_mask(mask);
    }
}

void MainWindow::switch_img_file(MainWindow::Direction direction)
{
    // choose the current items from the imgTreeWidget
    QTreeWidgetItem *current = imgTreeWidget->currentItem();
    if (!current)
        return;
    QTreeWidgetItem *currentParent = current->parent();

    if (!currentParent) {
        // we have a directory selected .. take the first file as current item
        current = current->child(0);
        currentParent = current->parent();
    }

    if (!current || !currentParent)
        return;

    // get the indeces
    int iParent = imgTreeWidget->indexOfTopLevelItem(currentParent);
    int iCurrent = currentParent->indexOfChild(current);

    // select the next file index
    if (direction == Up)
        iCurrent--;
    else
        iCurrent++;

    // the index may be negative .. in that case we switch the parent as well
    if (iCurrent < 0) {
        if (iParent > 0) {
            // get the directory before
            iParent--;
            currentParent = imgTreeWidget->topLevelItem(iParent);

            if (!currentParent)
                return;

            // get the last item from the directory before
            iCurrent = currentParent->childCount() - 1;
        }
        else
            // we are at the beginning ..
            iCurrent = 0;
    }
    // the index might be too large .. in that case we switch the parent as well
    else if (iCurrent >= currentParent->childCount()) {
        if (iParent < imgTreeWidget->topLevelItemCount() - 1) {
            // get the next directory
            iParent++;
            currentParent = imgTreeWidget->topLevelItem(iParent);

            if (!currentParent)
                return;

            // get the first item from the next directory
            iCurrent = 0;
        }
        else
            // we are at the end ..
            iCurrent = currentParent->childCount() - 1;
    }

    if (!currentParent)
        return;

    // we handled all special cases thus we may try to set the next current item
    current = currentParent->child(iCurrent);
    if (current)
        imgTreeWidget->setCurrentItem(current);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Control) {
        _is_key_ctrl_pressed = true;
        //pixmapWidget->setFloodFill(true);
        statusBar()->showMessage("Use mouse wheel to increase/decrease brush size");
        event->accept();
    }
    else if (event->key() == Qt::Key_Shift) {
        _is_key_shift_pressed = true;
        statusBar()->showMessage("Use the mouse wheel to change files");
        event->accept();
    }
    // keys to change the brush size
    else if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        int index = event->key() - Qt::Key_1;
        if (index >= 0 && index < brushSizeComboBox->count())
            brushSizeComboBox->setCurrentIndex(index);
        event->accept();
    }
    else
        event->ignore();
}

void MainWindow::keyReleaseEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Control) {
        event->accept();
        _is_key_ctrl_pressed = false;
        statusBar()->clearMessage();
    }
    else if (event->key() == Qt::Key_Shift) {
        event->accept();
        _is_key_shift_pressed = false;
        statusBar()->clearMessage();
    }
    else
        event->ignore();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (!event->isAccepted()) 
    {
        // see what to do with the event
        if (_is_key_shift_pressed) 
        {
            // select a different file
            if (event->delta() < 0)
                switch_img_file(Down);
            else if (event->delta() > 0)
                switch_img_file(Up);
        }
        else if (_is_key_ctrl_pressed) 
        {
            // select a different object
            if (event->delta() > 0) 
            {
                int idx = brushSizeComboBox->currentIndex() + 1;
                idx = idx > brushSizeComboBox->count() ? brushSizeComboBox->count() : idx;
                brushSizeComboBox->setCurrentIndex(idx);
            }
            else if (event->delta() < 0) 
            {
                int idx = brushSizeComboBox->currentIndex() - 1;
                idx = idx > 0 ? idx : 0;
                brushSizeComboBox->setCurrentIndex(idx);
            }
        }
        else 
        {
            // forward the wheelEvent to the zoomSpinBox
            if (event->delta() > 0)
                zoomSpinBox->stepDown();
            else if (event->delta() < 0)
                zoomSpinBox->stepUp();
        }
        event->accept();
    }
}

void MainWindow::show_mask_error_message_i()
{
    QMessageBox::critical(this, "Writing Error", "Object mask files could not be changed/created.\nPlease check your user rights for directory and files.");
}

void MainWindow::save_mask_i()
{
    // check whether dir/file/object have been selected
    QString iFile = get_current_file();
    QString iDir = get_current_direction();
    int iObj = get_current_obj_id();
    if (iFile.isEmpty() || iDir.isEmpty() || iObj < 0)
    {
        return;
    }

    // get the current mask
    const QImage& draw_mask = _pixmap_widget->get_draw_mask();

    //convert draw mask(RGBA32) to saved mask(Indexed8)
    QImage mask(draw_mask.size() , QImage::Format_Indexed8);
    mask.setColorTable(_color_table);
    mask.fill(BACKGROUND);

    //const QRgb color_background =qRgb(0, 0, 0);
    const QRgb color_confident= qRgb(255, 0, 0);
    const QRgb color_unconfident =qRgb(0, 255, 0);

    const int img_height = draw_mask.height();
    const int img_width = draw_mask.width();
    for (int y = 0; y < img_height; ++y)
    {
        for (int x = 0; x < img_width; ++x) 
        {
            QRgb rgb = draw_mask.pixel(x, y);

            if (qRed(rgb) >0)
            {
                mask.setPixel(x, y, CONFIDENCE_OBJECT);
            }
            else if(qGreen(rgb) > 0) 
            {
                mask.setPixel(x, y, UN_CONFIDENCE_OBJECT);
            }
            else
            {
                mask.setPixel(x , y , BACKGROUND);
            }
        }
    }
    // save the mask
    mask.save(_current_opened_direction + iDir + "/" + _current_obj_file_collection[iObj], "PNG");

    // save the image in the history and delete items in case the history
    // is too big
    while (0 < _current_history_img) 
    {
        _img_undo_history.pop_front();
        _current_history_img--;
    }
    QImage maskCopy = mask.copy();
    _img_undo_history.push_front(maskCopy);
    while (_img_undo_history.size() > _max_history_step)
        _img_undo_history.pop_back();

    update_undo_redo_menu();
}

void MainWindow::update_undo_redo_menu()
{
    // enable/disable the undo/redo menu items
    if (_current_history_img < _img_undo_history.size()  && _img_undo_history.size() > 1)
        actionUndo->setEnabled(true);
    else
        actionUndo->setEnabled(false);
    if (_current_history_img > 0 && _img_undo_history.size() > 1)
        actionRedo->setEnabled(true);
    else
        actionRedo->setEnabled(false);
}

std::map<int, QString> MainWindow::get_mask_files()
{
    // check weather dir/file/object have been selected
    QString iFile = get_current_file();
    QString iDir = get_current_direction();
    if (iFile.isEmpty() || iDir.isEmpty())
        return std::map<int ,QString>();


    // find all mask images for the current image .. they determine the
    // number of objects for one image
    QStringList nameFilters;
    nameFilters << iFile.replace(".image.", ".").section(".", 0, -2) + ".mask.*.png";
    QDir currentDir(_current_opened_direction + iDir);
    currentDir.setFilter(QDir::Files);
    currentDir.setNameFilters(nameFilters);
    currentDir.setSorting(QDir::Name);
    QStringList files = currentDir.entryList();
    qSort(files.begin(), files.end(), maskFileLessThan);

    _current_obj_file_collection.clear();
    for (int i = 0 ; i<files.size()  ;++i)
    {
        std::string s(files[i].toLocal8Bit());
        for (int j = 0 ; j< MASK_TPYE_NUM ; ++j)
        {
            std::string target = S_mask_types[j];
            if (s.find(target) != std::string::npos)
            {
                _current_obj_file_collection[j] = files[i];
                break;
            }
        }
    }

    return _current_obj_file_collection;
}

void MainWindow::on_confidenceCheckBox_stateChanged(int state)
{
    if (state)
    {
        _pixmap_widget->set_confidence(true);
    }
    else
    {
        _pixmap_widget->set_confidence(false);
    }
}

