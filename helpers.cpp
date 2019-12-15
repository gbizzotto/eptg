
#include "helpers.hpp"

#include <QDir>

int GetColumn(const QTableWidgetItem *item) { return item->column(); }
int GetColumn(const QModelIndex      &item) { return item .column(); }
QVariant GetData(const QTableWidgetItem *item) { return item->data(Qt::DisplayRole); }
QVariant GetData(const QModelIndex      &item) { return item .data(Qt::DisplayRole); }

template<>
QString & operator<<(QString & out, const std::string & t)
{
    return out.append(t.c_str());
}

QString PathAppend(const QString & path1, const QString & path2)
{
    if (path1.size() == 0)
        return path2;
    return QDir::cleanPath(path1 + QDir::separator() + path2);
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

std::vector<std::vector<std::string>> get_similar(const eptg::Model & model, int allowed_difference, std::function<bool(size_t,size_t)> callback)
{
    std::vector<std::vector<std::string>> result;

    size_t count = 0;
    std::vector<std::tuple<std::string, QImage, double, bool>> thumbs;

    // read all images, resize, calculate avg luminosity
    for (auto it=model.files.collection.begin(),end=model.files.collection.end() ; it!=end ; ++it)
    {
        QString full_path = PathAppend(QString::fromStdString(model.path), QString::fromStdString(it->first));
        QImage thumb = QImage(full_path).scaled(8, 8);
        double grad = 0;
        for (int y=0 ; y<8 ; y++)
            for (int x=0 ; x<8 ; x++)
            {
                QRgb rgb = thumb.pixel(x, y);
                grad += qRed  (rgb);
                grad += qGreen(rgb);
                grad += qBlue (rgb);
            }
        grad /= 3*8*8;
        thumbs.emplace_back(it->first, std::move(thumb), grad, false);

        count++;
        if ( ! callback(count, model.files.size()))
            return result;
    }

    // sort by
    std::sort(thumbs.begin(), thumbs.end(),
        [](const decltype(thumbs)::value_type & left, const decltype(thumbs)::value_type & right)
            {
                return std::get<2>(left) < std::get<2>(right);
            }
        );

    for (auto it=thumbs.begin(),end=thumbs.end() ; it!=end ; ++it)
    {
        if (std::get<3>(*it)) // already in a set
            continue;
        bool found_similar = false;
        auto grad = std::get<2>(*it);
        auto rel_path = std::get<0>(*it);
        for (auto it2=std::make_reverse_iterator(it),end2=thumbs.rend() ; it2!=end2 ; it2++)
        {
            if (std::get<3>(*it2)) // already in a set
                continue;
            if (std::get<2>(*it2) < grad-3)
                break;
            if (images_close(std::get<1>(*it), std::get<1>(*it2), allowed_difference))
            {
                if ( ! found_similar)
                {
                    result.push_back({});
                    result.back().push_back(rel_path);
                    found_similar = true;
                    std::get<3>(*it) = true;
                }
                std::get<3>(*it2) = true;
                result.back().push_back(std::get<0>(*it2));
            }
        }
        for (auto it2=std::next(it),end2=thumbs.end() ; it2!=end2 ; it2++)
        {
            if (std::get<3>(*it2)) // already in a set
                continue;
            if (std::get<2>(*it2) > grad+3)
                break;
            if (images_close(std::get<1>(*it), std::get<1>(*it2), allowed_difference))
            {
                if ( ! found_similar)
                {
                    result.push_back({});
                    result.back().push_back(rel_path);
                    found_similar = true;
                    std::get<3>(*it) = true;
                }
                std::get<3>(*it2) = true;
                result.back().push_back(std::get<0>(*it2));
            }
        }
    }
    return result;
}
