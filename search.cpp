
#include "search.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <assert.h>

bool in(const char * values, const char v)
{
    while(*values)
        if (v == *values++)
            return true;
    return false;
}

std::vector<std::string> split(const char *str, const char * separators = " \t", const char * ignore = "")
{
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

std::string to_lower(const std::string & str)
{
    std::string istr;
    std::transform(str.begin(), str.end(), std::back_inserter(istr), [](const char c){ return std::tolower(c); });
    return istr;
}

SearchNode::SearchNode(const SearchNode::Type type, const std::vector<std::variant<std::string,SearchNode>> & tokens)
    :type(type)
    ,subnodes(tokens)
{}

//SearchNode & SearchNode::operator=(SearchNode && other)
//{
//    this->type = other.type;
//    this->subnodes = std::move(other.subnodes);
//    return *this;
//}

SearchNode::SearchNode(const std::string str)
{
    std::vector<std::variant<std::string,SearchNode>> vars;
    for (std::string & token : split(str.c_str(), "()!", " \t"))
    {
        std::string lowercase_token = to_lower(token);
        if (in(std::vector<std::string>{"and", "or", "not"}, lowercase_token))
            vars.emplace_back(std::move(lowercase_token));
        else
            vars.emplace_back(std::move(token));
    }
    if (vars.empty())
        *this = SearchNode(Type::AND, {}); // will return true
    else
        *this = SearchNode(vars);
}

SearchNode::SearchNode(const std::vector<std::variant<std::string,SearchNode>> & tokens)
{
    std::vector<std::variant<std::string,SearchNode>> no_parenthesis_tokens;
    // process parenthesis
    for (auto it=tokens.begin(),end=tokens.end() ; it!=end ; ++it)
    {
        if (std::holds_alternative<std::string>(*it) && std::get<std::string>(*it) == "(")
        {
            int open_par_count = 1;
            for (auto it2=++it ; it2!=end ; ++it2)
            {
                if ( ! std::holds_alternative<std::string>(*it2))
                    continue;
                if (std::get<std::string>(*it2) == "(")
                    open_par_count++;
                else if (std::get<std::string>(*it2) == ")" && --open_par_count == 0)
                {
                    no_parenthesis_tokens.emplace_back(SearchNode(std::vector<std::variant<std::string,SearchNode>> {it, it2}));
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

    std::vector<std::variant<std::string,SearchNode>> no_not_tokens;
    // process NOT or !
    for (auto it=no_parenthesis_tokens.begin(),end=no_parenthesis_tokens.end() ; it!=end ; ++it)
    {
        if ( ! std::holds_alternative<std::string>(*it))
        {
            no_not_tokens.push_back(*it);
            continue;
        }
        std::string token = std::get<std::string>(*it);
        if (token == "not" || token == "!")
        {
            auto it2 = ++it;
            if (it2 == end)
                throw;
            no_not_tokens.push_back(SearchNode(SearchNode::Type::NOT, {*it2}));
        }
        else if (token[0] == '!')
            no_not_tokens.push_back(SearchNode(SearchNode::Type::NOT, {token.substr(1)}));
        else
            no_not_tokens.push_back(*it);
    }

    if (no_not_tokens.size() == 1 && ! std::holds_alternative<std::string>(no_not_tokens[0]))
    {
        this->type     = std::get<SearchNode>(no_not_tokens[0]).type;
        this->subnodes = std::move(std::get<SearchNode>(no_not_tokens[0]).subnodes);
        return;
    }

    // either it's an OR or an AND

    this->type = SearchNode::Type::AND;
    std::vector<std::variant<std::string,SearchNode>> and_clauses;
    for (auto it=no_not_tokens.begin(),end=no_not_tokens.end() ; it!=end ; ++it)
    {
        if (   ! std::holds_alternative<std::string>(*it)
            || (std::get<std::string>(*it) != "and" && std::get<std::string>(*it) != "or")  )
        {
            and_clauses.push_back(*it); // move?
            continue;
        }
        if (std::get<std::string>(*it) == "and")
            continue;
        if (std::get<std::string>(*it) == "or")
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

bool SearchNode::eval(const std::set<std::string> & words) const
{
    switch(type)
    {
        case Type::NOT:
            assert(this->subnodes.size() == 1);
            if (std::holds_alternative<std::string>(this->subnodes[0]))
                return std::find(words.begin(), words.end(), std::get<std::string>(this->subnodes[0])) == words.end();
            else
                return ! std::get<SearchNode>(this->subnodes[0]).eval(words);

        case Type::OR:
            for (const auto & subnode : this->subnodes)
            {
                if (std::holds_alternative<std::string>(subnode))
                {
                    if (std::find(words.begin(), words.end(), std::get<std::string>(subnode)) != words.end())
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
                if (std::holds_alternative<std::string>(subnode))
                {
                    if (std::find(words.begin(), words.end(), std::get<std::string>(subnode)) == words.end())
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
