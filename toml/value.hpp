//              Copyright Toru Niina 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)
#ifndef TOML_VALUE_HPP
#define TOML_VALUE_HPP
#include <toml/type_traits.hpp>
#include <boost/config.hpp>
#include <boost/blank.hpp>
#include <boost/type_traits.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <ostream>
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif// BOOST_NO_CXX11_HDR_INITIALIZER_LIST

namespace toml
{
namespace detail
{
template<typename T> struct first_type_of{};
template<typename T1, typename T2>
struct first_type_of<std::pair<T1, T2> > {typedef T1 type;};
template<typename T> struct second_type_of{};
template<typename T1, typename T2>
struct second_type_of<std::pair<T1, T2> > {typedef T2 type;};
} // detail

template<typename T>
struct to_kind;

struct value
{
    typedef boost::variant<boost::blank, boolean, integer, floating, string,
        date, time, local_datetime, offset_datetime, array, table> storage_type;

    enum kind
    {
        empty_tag           = 0,
        boolean_tag         = 1,
        integer_tag         = 2,
        float_tag           = 3,
        string_tag          = 4,
        date_tag            = 5,
        time_tag            = 6,
        local_datetime_tag  = 7,
        offset_datetime_tag = 8,
        array_tag           = 9,
        table_tag           = 10,
        undefined_tag       = 11
    };

    value(){}
    ~value(){}
    value(const value& v): storage_(v.storage_){}
    value& operator=(const value& v){storage_ = v.storage_; return *this;}

#ifdef BOOST_HAS_RVALUE_REFS
    value(value&& v): storage_(std::move(v.storage_)){}
    value& operator=(value&& v){storage_ = std::move(v.storage_); return *this;}
#endif // rvalue_refs

    template<typename T>
    value(const T& v, typename boost::enable_if<is_convertible<T, boolean>
            >::type* = 0): storage_(static_cast<boolean>(v))
    {}
    template<typename T>
    value(const T& v, typename boost::enable_if<is_convertible<T, integer>
            >::type* = 0): storage_(static_cast<integer>(v))
    {}
    template<typename T>
    value(const T& v, typename boost::enable_if<is_convertible<T, floating>
            >::type* = 0): storage_(static_cast<floating>(v))
    {}

    value(const string& v): storage_(v){}
    value(const char* v)       : storage_(string(v)){}
    value(const std::string& v): storage_(string(v)){}
    value(const char* v,        string::kind_t k): storage_(string(v, k)){}
    value(const std::string& v, string::kind_t k): storage_(string(v, k)){}

    value(const date& v): storage_(v){}
    value(const time& v): storage_(v){}
    value(const local_datetime& v): storage_(v){}
    value(const offset_datetime& v): storage_(v){}

    value(const date& d, const time& t)
        : storage_(local_datetime(d, t))
    {}
    value(const local_datetime& dt, const time_zone_ptr tzp)
        : storage_(offset_datetime(dt, tzp))
    {}
    value(const date& d, const time& t, const time_zone_ptr tzp)
        : storage_(offset_datetime(local_datetime(d, t), tzp))
    {}

    value(const array& v): storage_(v){}
    value(const table& v): storage_(v){}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    template<typename T>
    value(std::initializer_list<T> v, typename boost::enable_if<
            is_convertible<T, value> >::type* = 0)
        : storage_(array(v.begin(), v.end()))
    {}
    value(std::initializer_list<std::pair<key, value>> v)
        : storage_(table(v.begin(), v.end()))
    {}
#endif// BOOST_NO_CXX11_HDR_INITIALIZER_LIST

#ifdef BOOST_HAS_RVALUE_REFS
    value(string&&      v): storage_(std::move(v)) {}
    value(std::string&& v): storage_(string(std::move(v))) {}
    value(std::string&& v, string::kind_t k): storage_(string(std::move(v),k)){}
    value(array&& v): storage_(std::move(v)) {}
    value(table&& v): storage_(std::move(v)) {}
#endif

    template<typename Iterator>
    value(Iterator first, Iterator last, typename boost::enable_if<
        boost::is_same<typename boost::iterator_value<Iterator>::type, char>
        >::type* = 0): storage_(string(first, last))
    {}

    template<typename Iterator>
    value(Iterator first, Iterator last, typename boost::enable_if<
        is_convertible<typename boost::iterator_value<Iterator>::type, value>
        >::type* = 0): storage_(array(first, last))
    {}

    template<typename Iterator>
    value(Iterator first, Iterator last, typename boost::enable_if<
        boost::mpl::and_<
            boost::is_same<typename detail::first_type_of<
                typename boost::iterator_value<Iterator>::type>::type, key>,
            is_convertible<typename detail::second_type_of<
                typename boost::iterator_value<Iterator>::type>::type, value>
        > >::type* = 0)
        : storage_(table(first, last))
    {}

    template<typename T>
    typename boost::enable_if<is_convertible<T, boolean>, value>::type&
    operator=(const T& v)
    {this->storage_ = static_cast<boolean>(v); return *this;}

    template<typename T>
    typename boost::enable_if<is_convertible<T, integer>, value>::type&
    operator=(const T& v)
    {this->storage_ = static_cast<integer>(v); return *this;}

    template<typename T>
    typename boost::enable_if<is_convertible<T, floating>, value>::type&
    operator=(const T& v)
    {this->storage_ = static_cast<floating>(v); return *this;}

    value& operator=(const string& v)      {this->storage_ = v;   return *this;}
    value& operator=(const char* v)        {storage_ = string(v); return *this;}
    value& operator=(const std::string& v) {storage_ = string(v); return *this;}

    value& operator=(const date& v) {this->storage_ = v; return *this;}
    value& operator=(const time& v) {this->storage_ = v; return *this;}
    value& operator=(const local_datetime& v)  {this->storage_ = v; return *this;}
    value& operator=(const offset_datetime& v) {storage_ = v; return *this;}
    value& operator=(const array& v) {this->storage_ = v; return *this;}
    value& operator=(const table& v) {this->storage_ = v; return *this;}

#ifdef BOOST_HAS_RVALUE_REFS
    value& operator=(string&& v)
    {this->storage_ = std::move(v); return *this;}
    value& operator=(std::string&& v)
    {this->storage_ = string(std::move(v)); return *this;}

    value& operator=(array&& v) {this->storage_ = std::move(v); return *this;}
    value& operator=(table&& v) {this->storage_ = std::move(v); return *this;}
#endif

    int  index() const BOOST_NOEXCEPT_OR_NOTHROW {return storage_.which();}
    kind which() const BOOST_NOEXCEPT_OR_NOTHROW
    {return static_cast<kind>(storage_.which());}

    bool is(kind k) const BOOST_NOEXCEPT_OR_NOTHROW {return k == this->which();}

    template<typename T>
    typename boost::enable_if<is_toml_type<T>, bool>::type
    is() const BOOST_NOEXCEPT_OR_NOTHROW
    {return to_kind<T>::val == this->which();}

    template<typename T>
    typename boost::enable_if<is_toml_type<T>, T>::type&
    get()       {return boost::get<T>(this->storage_);}
    template<typename T>
    typename boost::enable_if<is_toml_type<T>, T>::type const&
    get() const {return boost::get<T>(this->storage_);}

    void swap(value& rhs) {this->storage_.swap(rhs.storage_);}

#if __cplusplus < 201103L
    template<typename Visitor>
    typename Visitor::result_type apply_visitor(Visitor v) const
    {return boost::apply_visitor(v, this->storage_);}

    template<typename Visitor>
    typename Visitor::result_type apply_visitor(Visitor v)
    {return boost::apply_visitor(v, this->storage_);}
#else
    template<typename Visitor>
    auto apply_visitor(Visitor&& v) const
        -> decltype(boost::apply_visitor(std::forward<Visitor>(v),
                                         std::declval<storage_type const&>()))
    {return boost::apply_visitor(std::forward<Visitor>(v), this->storage_);}

    template<typename Visitor>
    auto apply_visitor(Visitor&& v)
        -> decltype(boost::apply_visitor(std::forward<Visitor>(v),
                                         std::declval<storage_type&>()))
    {return boost::apply_visitor(std::forward<Visitor>(v), this->storage_);}
#endif

    bool operator==(const value& r) const {return this->storage_ == r.storage_;}
    bool operator!=(const value& r) const {return this->storage_ != r.storage_;}
    bool operator< (const value& r) const {return this->storage_ <  r.storage_;}
    bool operator> (const value& r) const {return this->storage_ >  r.storage_;}
    bool operator<=(const value& r) const {return this->storage_ <= r.storage_;}
    bool operator>=(const value& r) const {return this->storage_ >= r.storage_;}

  private:

    storage_type storage_;
};

inline void swap(value& lhs, value& rhs)
{
    lhs.swap(rhs);
    return;
}

#if __cplusplus <= 201103L
template<typename Visitor>
typename Visitor::result_type apply_visitor(Visitor vis, const value& v)
{
    return v.apply_visitor(vis);
}
template<typename Visitor>
typename Visitor::result_type apply_visitor(Visitor vis, value& v)
{
    return v.apply_visitor(vis);
}

template<typename Visitor>
typename Visitor::result_type visit(Visitor vis, const value& v)
{
    return v.apply_visitor(vis);
}
template<typename Visitor>
typename Visitor::result_type visit(Visitor vis, value& v)
{
    return v.apply_visitor(vis);
}
#else // after c++11
template<typename Visitor>
auto apply_visitor(Visitor vis, const value& v)
    -> decltype(v.apply_visitor(vis))
{
    return v.apply_visitor(vis);
}
template<typename Visitor>
auto apply_visitor(Visitor vis, value& v)
    -> decltype(v.apply_visitor(vis))
{
    return v.apply_visitor(vis);
}
template<typename Visitor>
auto visit(Visitor vis, const value& v) -> decltype(v.apply_visitor(vis))
{
    return v.apply_visitor(vis);
}
template<typename Visitor>
auto visit(Visitor vis, value& v) -> decltype(v.apply_visitor(vis))
{
    return v.apply_visitor(vis);
}
#endif


template<typename T> struct to_kind
{BOOST_STATIC_CONSTEXPR typename value::kind val = value::undefined_tag;};
template<typename T>
BOOST_CONSTEXPR_OR_CONST typename value::kind to_kind<T>::val;

template<> struct to_kind<boolean>
{BOOST_STATIC_CONSTEXPR value::kind val = value::boolean_tag;};
template<> struct to_kind<integer>
{BOOST_STATIC_CONSTEXPR value::kind val = value::integer_tag;};
template<> struct to_kind<floating>
{BOOST_STATIC_CONSTEXPR value::kind val = value::float_tag;};
template<> struct to_kind<string>
{BOOST_STATIC_CONSTEXPR value::kind val = value::string_tag;};
template<> struct to_kind<date>
{BOOST_STATIC_CONSTEXPR value::kind val = value::date_tag;};
template<> struct to_kind<time>
{BOOST_STATIC_CONSTEXPR value::kind val = value::time_tag;};
template<> struct to_kind<local_datetime>
{BOOST_STATIC_CONSTEXPR value::kind val = value::local_datetime_tag;};
template<> struct to_kind<offset_datetime>
{BOOST_STATIC_CONSTEXPR value::kind val = value::offset_datetime_tag;};
template<> struct to_kind<array>
{BOOST_STATIC_CONSTEXPR value::kind val = value::array_tag;};
template<> struct to_kind<table>
{BOOST_STATIC_CONSTEXPR value::kind val = value::table_tag;};

template<typename charT, typename traits>
inline std::basic_ostream<charT, traits>&
operator<<(std::basic_ostream<charT, traits>& os, value::kind k)
{
    switch(k)
    {
        case value::empty_tag           : {os << "empty";           return os;}
        case value::boolean_tag         : {os << "boolean";         return os;}
        case value::integer_tag         : {os << "integer";         return os;}
        case value::float_tag           : {os << "float";           return os;}
        case value::string_tag          : {os << "string";          return os;}
        case value::date_tag            : {os << "date";            return os;}
        case value::time_tag            : {os << "time";            return os;}
        case value::local_datetime_tag  : {os << "local-datetime";  return os;}
        case value::offset_datetime_tag : {os << "offset-datetime"; return os;}
        case value::array_tag           : {os << "array";           return os;}
        case value::table_tag           : {os << "table";           return os;}
        case value::undefined_tag       : {os << "undefined";       return os;}
        default                         : {os << "unknown";         return os;}
    }
}

} // toml
#endif//TOML_VALUE_HPP
