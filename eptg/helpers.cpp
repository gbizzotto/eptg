
#include <string>
#include <cmath>

#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QSize>

#include "eptg/helpers.hpp"
#include "eptg/constants.hpp"


int get_column(const QTableWidgetItem *item) { return item->column(); }
int get_column(const QModelIndex      &item) { return item .column(); }
QVariant get_data(const QTableWidgetItem *item) { return item->data(Qt::DisplayRole); }
QVariant get_data(const QModelIndex      &item) { return item .data(Qt::DisplayRole); }

std::set<QString> names_from_list(const QListWidget * list)
{
    std::set<QString> result;
    for (int i=0 ; i<list->count() ; i++)
        if (list->item(i)->data(Qt::DisplayRole).toString().size() > 0)
            result.insert(list->item(i)->data(Qt::DisplayRole).toString());
    return result;
}

bool images_close(const QImage & left, const QImage & right, int allowed_difference)
{
    int diff = 0;
    for (int x=0 ; x<8 ; x++)
        for (int y=0 ; y<8 ; y++)
        {
            QRgb  left_pixel =  left.pixel(x, y);
            QRgb right_pixel = right.pixel(x, y);
            diff += (qRed  (left_pixel) - qRed  (right_pixel)) * (qRed  (left_pixel) - qRed  (right_pixel))
                  + (qGreen(left_pixel) - qGreen(right_pixel)) * (qGreen(left_pixel) - qGreen(right_pixel))
                  + (qBlue (left_pixel) - qBlue (right_pixel)) * (qBlue (left_pixel) - qBlue (right_pixel))
                  ;
            if (diff > allowed_difference)
                return false;
        }
    return true;
}

QPixmap make_preview(const QString & base_path, const std::set<QString> & selected_items_text, const QSize & size)
{
	QPixmap image_result(size);
	image_result.fill(Qt::transparent);
	int cols = int(std::ceil(std::sqrt(selected_items_text.size())));
	int rows = int(std::ceil(selected_items_text.size() * 1.0 / cols));
	int w = image_result.width() / cols;
	int h = image_result.height() / rows;
	auto it = selected_items_text.begin();
	for (int r = 0 ; r<rows ; ++r)
		for (int c = 0 ; c<cols && it!=selected_items_text.end() ; ++c,++it)
		{
			auto full_path = path::append(base_path, *it);
			QPixmap image(full_path);
			if (image.width() > w || image.height() > h)
				image = image.scaled(w-IMG_BORDER, h-IMG_BORDER, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
			QRectF targetRect(w*c + (w-image.width())/2, h*r + (h-image.height())/2, image.width(), image.height());
			QRectF sourceRect(0, 0, image.width(), image.height());
			QPainter painter(&image_result);
			painter.drawPixmap(targetRect, image, sourceRect);
		}
	return image_result;
}
