#ifndef IN_HPP
#define IN_HPP

#include <utility>
#include <algorithm>

namespace eptg {


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
template<typename C,
         typename std::enable_if< ! std::is_same<typename C::value_type,typename C::key_type>{},int>::type = 0>
bool in(const C & container, const typename C::key_type & v)
{
    return find(container, v) != container.end();
}

bool in(const char * values, const char v);


} // namespace

#endif // include guard
