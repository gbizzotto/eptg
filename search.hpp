#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <vector>
#include <set>
#include <variant>
#include <string>
#include <algorithm>

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

struct SearchNode
{
    enum Type
    {
        AND  = 1,
        OR   = 2,
        NOT  = 3,
    };

    Type type;
    std::vector<std::variant<std::string,SearchNode>> subnodes;

    SearchNode(const std::string str);
    SearchNode(const std::vector<std::variant<std::string,SearchNode>> & tokens);
    SearchNode(const Type type, const std::vector<std::variant<std::string,SearchNode>> & tokens);

//    SearchNode & operator=(SearchNode && other);

    bool eval(const std::set<std::string> & words) const;
};

#endif // SEARCH_HPP
