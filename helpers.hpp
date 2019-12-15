#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <vector>
#include <string>
#include <functional>

#include <QImage>
#include <QTableWidgetItem>

#include "model.hpp"

int GetColumn(const QTableWidgetItem *item);
int GetColumn(const QModelIndex      &item);
QVariant GetData(const QTableWidgetItem *item);
QVariant GetData(const QModelIndex      &item);

template<typename ITEM_TYPE>
std::set<std::string> GetSelectedRowsTitles(const QList<ITEM_TYPE> & selected_items)
{
    std::set<std::string> result;
    for (int i=0 ; i<selected_items.size() ; i++)
        if (GetColumn(selected_items[i]) == 0)
            result.insert(GetData(selected_items[i]).toString().toStdString());
    return result;
}

template<typename C>
QStringList QStringListFromStd(const C & container)
{
    QStringList result;
    for (const std::string & s : container)
        result.append(QString::fromStdString(s));
    return result;
}

template<typename C>
QStringList QStringListFromStd(C && string_collection)
{
    QStringList result;
    for (const std::string & str : string_collection)
        result += QString::fromStdString(str);
    return result;
}

template<typename T>
QString & operator<<(QString & out, const T & t)
{
    return out.append(t);
}
template<>
QString & operator<<(QString & out, const std::string & t);

QString PathAppend(const QString & path1, const QString & path2);

bool images_close(const QImage & left, const QImage & right, int allowed_difference);
std::vector<std::vector<std::string>> get_similar(const eptg::Model & model, int allowed_difference, std::function<bool(size_t,size_t)> callback);

#endif // HELPERS_HPP
