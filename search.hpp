#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <vector>
#include <set>
#include <variant>
#include <string>
#include <algorithm>

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
