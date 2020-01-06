#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <vector>
#include <deque>
#include <set>
#include <string>
#include <functional>
#include <type_traits>
#include <fstream>
#include <codecvt>
#include <sstream>
#include <iostream>

#include <QImage>
#include <QTableWidgetItem>
#include <QListWidget>
#include <QString>
#include <QPixmap>
#include <QSize>

#include "eptg/path.hpp"
#include "eptg/string.hpp"
#include "eptg/in.hpp"
#include "eptg/string.hpp"


int get_column(const QTableWidgetItem *item);
int get_column(const QModelIndex      &item);
QVariant get_data(const QTableWidgetItem *item);
QVariant get_data(const QModelIndex      &item);

template<typename ITEM_TYPE>
std::set<QString> names_from_list(const QList<ITEM_TYPE> & selected_items)
{
    std::set<QString> result;
    for (int i=0 ; i<selected_items.size() ; i++)
        if (get_column(selected_items[i]) == 0)
            result.insert(get_data(selected_items[i]).toString());
    return result;
}
std::set<QString> names_from_list(const QListWidget * list);


bool images_close(const QImage & left, const QImage & right, int allowed_difference);
std::tuple<QPixmap,QSize,int> make_image  (const QString & full_path, const QSize & initial_size, const QSize & max_size);
QPixmap make_preview(const QString & base_path, const std::set<QString> & selected_items_text, const QSize & size);

template<typename T>
std::set<T> added(const std::set<T> & before, const std::set<T> & after)
{
    std::set<T> result;
    for (const T & t : after)
        if (before.find(t) == before.end())
            result.insert(t);
    return result;
}

template<typename C, typename A>
A accumulate(const C & iterable, A accumulator)
{
    return accumulate(std::begin(iterable), std::end(iterable), accumulator);
}
template<typename C, typename A, typename F>
A accumulate(const C & iterable, A accumulator, const F & accumulate_functor)
{
    return accumulate(std::begin(iterable), std::end(iterable), accumulator, accumulate_functor);
}

// range-based for loop on KEYS
template<typename C>
class keys_it
{
    typename C::const_iterator it_;
public:
    using key_type        = typename C::key_type;
    using pointer         = typename C::key_type*;
    using difference_type = std::ptrdiff_t;

    keys_it(const typename C::const_iterator & it) : it_(it) {}

    keys_it         operator++(int               ) /* postfix */ { return it_++         ; }
    keys_it&        operator++(                  ) /*  prefix */ { ++it_; return *this  ; }
    const key_type& operator* (                  ) const         { return it_->first    ; }
    const key_type& operator->(                  ) const         { return it_->first    ; }
    keys_it         operator+ (difference_type v ) const         { return it_ + v       ; }
    bool            operator==(const keys_it& rhs) const         { return it_ == rhs.it_; }
    bool            operator!=(const keys_it& rhs) const         { return it_ != rhs.it_; }
};
template<typename C>
class keys_impl
{
    const C & c;
public:
    keys_impl(const C & container) : c(container) {}
    const keys_it<C> begin() const { return keys_it<C>(std::begin(c)); }
    const keys_it<C> end  () const { return keys_it<C>(std::end  (c)); }
};
template<typename C>
keys_impl<C> keys(const C & container) { return keys_impl<C>(container); }

// range-based for loop on VALUES
template<typename C>
class values_it
{
    typename C::const_iterator it_;
public:
    using mapped_type     = typename C::mapped_type;
    using difference_type = std::ptrdiff_t;

    values_it(const typename C::const_iterator & it) : it_(it) {}

    values_it          operator++(int                 ) /* postfix */ { return it_++         ; }
    values_it&         operator++(                    ) /*  prefix */ { ++it_; return *this  ; }
    const mapped_type& operator* (                    ) const         { return it_->second   ; }
    const mapped_type& operator->(                    ) const         { return it_->second   ; }
    values_it          operator+ (difference_type v   ) const         { return it_ + v       ; }
    bool               operator==(const values_it& rhs) const         { return it_ == rhs.it_; }
    bool               operator!=(const values_it& rhs) const         { return it_ != rhs.it_; }
};
template<typename C>
class values_impl
{
    const C & c;
public:
    values_impl(const C & container) : c(container) {}
    const values_it<C> begin() const { return values_it<C>(std::begin(c)); }
    const values_it<C> end  () const { return values_it<C>(std::end  (c)); }
};
template<typename C>
values_impl<C> values(const C & container) { return values_impl<C>(container); }


template<typename STR, typename C>
std::vector<std::vector<STR>> get_similar(const STR & base_path, const C & rel_paths, int allowed_difference, std::function<bool(size_t,size_t)> progress_callback)
{
    std::vector<std::vector<STR>> result;

    size_t count = 0;
    std::vector<std::tuple<STR, QImage, double, bool>> thumbs;

    // read all images, resize, calculate avg luminosity
    for (const STR & rel_path : keys(rel_paths))
    {
        STR full_path = path::append(base_path, rel_path);
        QImage thumb = QImage(eptg::str::to<QString>(full_path)).scaled(8, 8);
		if (thumb.isNull())
			continue;
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
        thumbs.emplace_back(rel_path, std::move(thumb), grad, false);

        count++;
        if ( ! progress_callback(count, rel_paths.size()))
            return result;
    }

    // sort by
    std::sort(thumbs.begin(), thumbs.end(),
        [](const typename decltype(thumbs)::value_type & left, const typename decltype(thumbs)::value_type & right)
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

template<typename STR>
std::wstring read_file(const STR & full_path)
{
	if ( ! eptg::fs::exists(eptg::str::to<std::string>(full_path)))
		return std::wstring();
	std::wifstream fin(eptg::str::to<std::string>(full_path), std::wifstream::binary);
	fin.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<wchar_t>));
	std::wstringstream buffer;
	buffer << fin.rdbuf();
	std::wstring json_string = buffer.str();
	return json_string;
}

template <class K, class T, class C, class A, class Predicate>
void erase_if(std::map<K, T, C, A>& c, Predicate pred) {
    for (auto i = c.begin(), last = c.end(); i != last; )
        if (pred(*i))
            i = c.erase(i);
        else
            ++i;
}

#endif // HELPERS_HPP
