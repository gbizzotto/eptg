
#include <string>

#include <QDir>

#include "eptg/helpers.hpp"
#include "eptg/constants.hpp"

bool in(const char * values, const char v)
{
    while(*values)
        if (v == *values++)
            return true;
    return false;
}

int get_column(const QTableWidgetItem *item) { return item->column(); }
int get_column(const QModelIndex      &item) { return item .column(); }
QVariant get_data(const QTableWidgetItem *item) { return item->data(Qt::DisplayRole); }
QVariant get_data(const QModelIndex      &item) { return item .data(Qt::DisplayRole); }

template<>
QString & operator<<(QString & out, const std::string & t)
{
    return out.append(QString::fromStdString(t));
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

std::string substring(const std::string & str, size_t idx)
{
    return str.substr(idx);
}
QString substring(QString str, int idx)
{
    return str.remove(0, idx);
}

std::vector<std::string> split(const std::string & s, const char * separators, const char * ignore)
{
    const char *str = s.c_str();

    std::vector<std::string> result;
    while (*str)
    {
        while(*str && in(ignore,*str))
            str++;
        if ( ! *str)
            break;
        if (in(separators,*str))
        {
            result.push_back(std::string(str, str+1));
            str++;
            continue;
        }
        const char *begin = str;
        while(*str && ! in(separators, *str) && ! in(ignore, *str))
            str++;
        result.push_back(std::string(begin, str));
    }
    return result;
}
std::vector<QString> split(const QString & s, const char * separators, const char * ignore)
{
    std::vector<QString> result;
    for (const std::string & s : split(s.toStdString(), separators, ignore))
        result.push_back(QString::fromStdString(s));
    return result;
}

std::string to_lower(const std::string & str)
{
    std::string istr;
    std::transform(str.begin(), str.end(), std::back_inserter(istr), [](const char c){ return std::tolower(c); });
    return istr;
}

QString to_lower(const QString & str)
{
    return str.toLower();
}

template<>
QString str_to<QString>(const std::string & str)
{
    return QString::fromStdString(str);
}
template<>
QString str_to<QString>(const QString & str)
{
    return str;
}
template<>
std::string str_to<std::string>(const std::string & str)
{
    return str;
}
template<>
std::string str_to<std::string>(const QString & str)
{
    return str.toStdString();
}

std::set<QString> unique_tokens(const QString & str)
{
    std::set<QString> typed_tags;
    for (const QString & tag : str.split(' '))
    {
        if (tag.size() == 0)
            continue;
        typed_tags.insert(tag);
    }
    return typed_tags;
}
