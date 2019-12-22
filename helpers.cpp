
#include "helpers.hpp"

#include <QDir>

bool in(const char * values, const char v)
{
    while(*values)
        if (v == *values++)
            return true;
    return false;
}

int GetColumn(const QTableWidgetItem *item) { return item->column(); }
int GetColumn(const QModelIndex      &item) { return item .column(); }
QVariant GetData(const QTableWidgetItem *item) { return item->data(Qt::DisplayRole); }
QVariant GetData(const QModelIndex      &item) { return item .data(Qt::DisplayRole); }

template<>
QString & operator<<(QString & out, const std::string & t)
{
    return out.append(QString::fromStdString(t));
}

QString PathAppend(const QString & path1, const QString & path2)
{
    if (path1.size() == 0)
        return path2;
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}
bool PathIsSub(const QString & path, const QString & maybe_sub_path)
{
     return (QDir(maybe_sub_path).absolutePath().startsWith(QDir(path).absolutePath()));
}
QString PathUp(QString path)
{
    if (path.endsWith("/"))
        path.chop(1);
    int idx = path.lastIndexOf("/");
    if (idx == -1)
        return "";
    path.truncate(idx);
    return path;
}
bool PathParentHas(QString sweep_path, const QString & file_name)
{
    for ( ; sweep_path.size() > 0 ; sweep_path=PathUp(sweep_path))
        if (QFileInfo(PathAppend(sweep_path, QString(file_name))).exists())
            return true;
    return false;
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

QString substring(QString str, int idx)
{
    return str.remove(0, idx);
}
std::string substring(const std::string & str, size_t idx)
{
    return str.substr(idx);
}
