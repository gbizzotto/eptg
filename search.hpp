#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <vector>
#include <set>
#include <variant>
#include <string>
#include <algorithm>

#include <QString>

#include "helpers.hpp"

std::vector<std::string> split(const std::string & s, const char * separators = " \t", const char * ignore = "");
std::vector<QString>     split(const     QString & s, const char * separators = " \t", const char * ignore = "");
std::string to_lower(const std::string & str);
QString to_lower(const QString & str);

template<typename STR>
struct SearchNode
{
    enum Type
    {
        AND  = 1,
        OR   = 2,
        NOT  = 3,
    };

    Type type;
    std::vector<std::variant<STR,SearchNode>> subnodes;

    SearchNode(const STR str)
    {
        std::vector<std::variant<STR,SearchNode>> vars;
        for (STR & token : split(str, "()!", " \t"))
        {
            STR lowercase_token = to_lower(token);
            if (in(std::vector<STR>{"and", "or", "not"}, lowercase_token))
                vars.emplace_back(std::move(lowercase_token));
            else
                vars.emplace_back(std::move(token));
        }
        if (vars.empty())
            *this = SearchNode(Type::AND, {}); // will return true
        else
            *this = SearchNode(vars);
    }
    SearchNode(const std::vector<std::variant<STR,SearchNode>> & tokens)
    {
        std::vector<std::variant<STR,SearchNode>> no_parenthesis_tokens;
        // process parenthesis
        for (auto it=tokens.begin(),end=tokens.end() ; it!=end ; ++it)
        {
            if (std::holds_alternative<STR>(*it) && std::get<STR>(*it) == "(")
            {
                int open_par_count = 1;
                for (auto it2=++it ; it2!=end ; ++it2)
                {
                    if ( ! std::holds_alternative<STR>(*it2))
                        continue;
                    if (std::get<STR>(*it2) == "(")
                        open_par_count++;
                    else if (std::get<STR>(*it2) == ")" && --open_par_count == 0)
                    {
                        no_parenthesis_tokens.emplace_back(SearchNode(std::vector<std::variant<STR,SearchNode>> {it, it2}));
                        it = it2;
                        break;
                    }
                }
                if (open_par_count != 0)
                    throw; // didn't find the closing parenthesis
            }
            else
                no_parenthesis_tokens.push_back(*it);
        }

        std::vector<std::variant<STR,SearchNode>> no_not_tokens;
        // process NOT or !
        for (auto it=no_parenthesis_tokens.begin(),end=no_parenthesis_tokens.end() ; it!=end ; ++it)
        {
            if ( ! std::holds_alternative<STR>(*it))
            {
                no_not_tokens.push_back(*it);
                continue;
            }
            STR token = std::get<STR>(*it);
            if (token == "not" || token == "!")
            {
                auto it2 = ++it;
                if (it2 == end)
                    throw;
                no_not_tokens.push_back(SearchNode(SearchNode::Type::NOT, {*it2}));
            }
            else if (token[0] == '!')
                no_not_tokens.push_back(SearchNode(SearchNode::Type::NOT, {substring(token, 1)}));
            else
                no_not_tokens.push_back(*it);
        }

        if (no_not_tokens.size() == 1 && ! std::holds_alternative<STR>(no_not_tokens[0]))
        {
            this->type     = std::get<SearchNode>(no_not_tokens[0]).type;
            this->subnodes = std::move(std::get<SearchNode>(no_not_tokens[0]).subnodes);
            return;
        }

        // either it's an OR or an AND

        this->type = SearchNode::Type::AND;
        std::vector<std::variant<STR,SearchNode>> and_clauses;
        for (auto it=no_not_tokens.begin(),end=no_not_tokens.end() ; it!=end ; ++it)
        {
            if (   ! std::holds_alternative<STR>(*it)
                || (std::get<STR>(*it) != "and" && std::get<STR>(*it) != "or")  )
            {
                and_clauses.push_back(*it); // move?
                continue;
            }
            if (std::get<STR>(*it) == "and")
                continue;
            if (std::get<STR>(*it) == "or")
            {
                if (and_clauses.empty())
                    throw; // starts with and OR or has OR OR
                this->type = SearchNode::Type::OR;
                if (and_clauses.size() == 1)
                    this->subnodes.emplace_back(std::move(and_clauses[0]));
                else
                    this->subnodes.emplace_back(SearchNode(SearchNode::Type::AND, std::move(and_clauses)));
                and_clauses.clear();
            }
        }
        if (and_clauses.empty())
            throw; // starts with and OR or has OR OR

        if (this->type == SearchNode::Type::OR)
            this->subnodes.emplace_back(SearchNode(SearchNode::Type::AND, std::move(and_clauses)));
        else
            this->subnodes = std::move(and_clauses);
    }
    SearchNode(const Type type, const std::vector<std::variant<STR,SearchNode>> & tokens)
        :type(type)
        ,subnodes(tokens)
    {}

//    SearchNode & operator=(SearchNode && other);

    bool eval(const std::set<STR> & words) const
    {
        switch(type)
        {
            case Type::NOT:
                assert(this->subnodes.size() == 1);
                if (std::holds_alternative<STR>(this->subnodes[0]))
                    return std::find(words.begin(), words.end(), std::get<STR>(this->subnodes[0])) == words.end();
                else
                    return ! std::get<SearchNode>(this->subnodes[0]).eval(words);

            case Type::OR:
                for (const auto & subnode : this->subnodes)
                {
                    if (std::holds_alternative<STR>(subnode))
                    {
                        if (std::find(words.begin(), words.end(), std::get<STR>(subnode)) != words.end())
                            return true;
                    }
                    else
                    {
                        if (std::get<SearchNode>(subnode).eval(words))
                            return true;
                    }
                }
                return false;

            case Type::AND:
                for (const auto & subnode : this->subnodes)
                {
                    if (std::holds_alternative<STR>(subnode))
                    {
                        if (std::find(words.begin(), words.end(), std::get<STR>(subnode)) == words.end())
                            return false;
                    }
                    else
                    {
                        if ( ! std::get<SearchNode>(subnode).eval(words))
                            return false;
                    }
                }
                return true;
        }
        return false;
    }
};

#endif // SEARCH_HPP
