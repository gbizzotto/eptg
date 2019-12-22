#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <vector>
#include <set>
#include <string>
#include <functional>
#include <type_traits>

#include <QImage>
#include <QTableWidgetItem>

template<class Container, class T>
auto find_impl(Container& c, const T& value, int) -> decltype(c.find(value)){
    return c.find(value);
}
template<class Container, class T>
auto find_impl(Container& c, const T& value, long) -> decltype(std::begin(c)){
    return std::find(std::begin(c), std::end(c), value);
}
template<class Container, class T>
auto find(Container& c, const T& value) -> decltype(find_impl(c, value, 0)) {
    return find_impl(c, value, 0);
}
template<typename C>
bool in(const C & container, const typename C::value_type & v)
{
    return find(container, v) != container.end();
}
template<typename C>
bool in(const C & container, const typename C::mapped_type & v)
{
    return find(container, v) != container.end();
}

bool in(const char * values, const char v);

int get_column(const QTableWidgetItem *item);
int get_column(const QModelIndex      &item);
QVariant get_data(const QTableWidgetItem *item);
QVariant get_data(const QModelIndex      &item);

template<typename ITEM_TYPE>
std::set<QString> names_from_list_selection(const QList<ITEM_TYPE> & selected_items)
{
    std::set<QString> result;
    for (int i=0 ; i<selected_items.size() ; i++)
        if (get_column(selected_items[i]) == 0)
            result.insert(get_data(selected_items[i]).toString());
    return result;
}

template<typename C,
         typename std::enable_if<std::is_same<typename C::value_type,std::string>{},int>::type = 0>
QStringList qstring_list_from_std_container(const C & container)
{
    QStringList result;
    for (const std::string & s : container)
        result.append(QString::fromStdString(s));
    return result;
}
template<typename C,
         typename std::enable_if<std::is_same<typename C::value_type,std::string>{},int>::type = 0>
QStringList qstring_list_from_std_container(C && string_collection)
{
    QStringList result;
    for (const std::string & str : string_collection)
        result += QString::fromStdString(str);
    return result;
}

template<typename C,
         typename std::enable_if<std::is_same<typename C::value_type,QString>{},int>::type = 0>
QStringList qstring_list_from_std_container(const C & string_collection)
{
    QStringList result;
    for (const QString & str : string_collection)
        result += str;
    return result;
}
template<typename C,
         typename std::enable_if<std::is_same<typename C::value_type,QString>{},int>::type = 0>
QStringList qstring_list_from_std_container(C && string_collection)
{
    QStringList result;
    for (const QString & str : string_collection)
        result += str;
    return result;
}

template<typename T>
QString & operator<<(QString & out, const T & t)
{
    return out.append(t);
}
template<>
QString & operator<<(QString & out, const std::string & t);

bool images_close(const QImage & left, const QImage & right, int allowed_difference);


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

std::string substring(const std::string & str, size_t idx);
    QString substring(          QString   str,    int idx);
std::vector<std::string> split(const std::string & s, const char * separators = " \t", const char * ignore = "");
std::vector<    QString> split(const     QString & s, const char * separators = " \t", const char * ignore = "");
std::string to_lower(const std::string & str);
    QString to_lower(const     QString & str);

#endif // HELPERS_HPP
