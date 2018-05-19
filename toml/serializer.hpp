//              Copyright Toru Niina 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)
#ifndef TOML_SERIALIZER_HPP
#define TOML_SERIALIZER_HPP
#include <toml/value.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <iomanip>

namespace toml
{

BOOST_CONSTEXPR inline std::size_t forceinline()
{
    return std::numeric_limits<std::size_t>::max();
}

// TODO fix everything
struct serializer : boost::static_visitor<std::string>
{
    serializer(const std::size_t ln): line_(ln){}
    ~serializer(){}

    std::string operator()(const boost::blank) const {return "<blank>";}
    std::string operator()(const boolean b) const {return b ? "true" : "false";}
    std::string operator()(const integer i) const
    {
        return boost::lexical_cast<std::string>(i);
    }
    std::string operator()(const floating f) const
    {
        std::ostringstream oss; oss << std::fixed << f;
        return oss.str();
    }
    std::string operator()(const string& s) const
    {
        const std::string quote(1, (s.kind == string::literal) ? '\'' : '"');
        return quote + s.str + quote;
    }
    std::string operator()(const date& v) const
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
    std::string operator()(const time& v) const
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
    std::string operator()(const local_datetime& v) const
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
    std::string operator()(const offset_datetime& v) const
    {
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
    std::string operator()(const array& v) const
    {
        std::string serial;
        serial += '[';
        for(typename array::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
        {
            serial += apply_visitor(*this, *i);
            serial += ", ";
        }
        serial += ']';
        return serial;
    }
    std::string operator()(const table& v) const
    {
        std::string serial;
        serial += '{';
        for(typename table::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
        {
            serial += i->first;
            serial += " = ";
            serial += apply_visitor(*this, i->second);
        }
        serial += '}';
        return serial;
    }

  private:
    std::size_t line_;    // TODO use it to serialize
    std::string valname_; // to output table...
};

inline std::string serialize(const value& v, std::size_t line = 80)
{
    return apply_visitor(serializer(line), v);
}

template<typename charT, typename traits>
std::basic_ostream<charT, traits>&
operator<<(std::basic_ostream<charT, traits>& os, const value& v)
{
    os << serialize(v);
    return os;
}

} // toml
#endif// TOML_SERIALIZER_HPP
