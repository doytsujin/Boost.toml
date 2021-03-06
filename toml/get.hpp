//              Copyright Toru Niina 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)
#ifndef TOML_GET_HPP
#define TOML_GET_HPP
#include <toml/type_traits.hpp>
#include <toml/value.hpp>
#include <boost/format.hpp>
#include <boost/type_index.hpp>

namespace toml
{

struct bad_get : public std::exception
{
    bad_get(const std::string& mes): message_(mes){}
    ~bad_get() BOOST_NOEXCEPT_OR_NOTHROW {}
    const char * what() const BOOST_NOEXCEPT_OR_NOTHROW
    {return message_.c_str();}

  private:
    std::string message_;
};

// non-conversion. conversion for datetime is provided in a different way
// to support conversion like date -> datetime (0:00), time -> datetime(today, local)
template<typename T>
inline typename boost::enable_if<is_toml_type<T>, T>::type&
get(value& v)
{
    try
    {
        return v.template get<T>();
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}

template<typename T>
inline typename boost::enable_if<is_toml_type<T>, T>::type const&
get(value const& v)
{
    try
    {
        return v.template get<T>();
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}

// toml::string -> string, returning lvalue.
template<typename T>
typename boost::enable_if<boost::is_same<T, std::string>, T>::type&
get(value& v)
{
    try
    {
        return v.get<string>().str;
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}
template<typename T>
typename boost::enable_if<boost::is_same<T, std::string>, T>::type const&
get(value const& v)
{
    try
    {
        return v.get<string>().str;
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}

// toml::value itself. nothing needed!
template<typename T>
typename boost::enable_if<boost::is_same<T, value>, T>::type&
get(value& v)
{
    return v;
}
template<typename T>
typename boost::enable_if<boost::is_same<T, value>, T>::type const&
get(value const& v)
{
    return v;
}


// ---------------------------------------------------------------------------
// conversions. return a prvalue

// integral types but not exactly toml::integer...
template<typename T>
typename boost::enable_if<boost::mpl::and_<
        boost::mpl::not_<is_toml_type<T> >,
        boost::is_integral<T>
    >, T>::type
get(const toml::value& v)
{
    try
    {
        return static_cast<T>(v.get<integer>());
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}

// floating types but not exactly toml::floating...
template<typename T>
typename boost::enable_if<boost::mpl::and_<
        boost::mpl::not_<is_toml_type<T> >,
        boost::is_floating_point<T>
    >, T>::type
get(const toml::value& v)
{
    try
    {
        return static_cast<T>(v.get<floating>());
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}

// string_view
template<typename T>
typename boost::enable_if<is_string_view_like<T>, T>::type
get(const toml::value& v)
{
    try
    {
        return T(v.get<string>().str);
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}

// std::tm from date, time, local_datetime, offset_datetime
template<typename T>
typename boost::enable_if<boost::is_same<T, std::tm>, T>::type
get(const value& v)
{
    try
    {
        switch(v.which())
        {
            case value::date_tag:
            {
                return boost::gregorian::to_tm(v.get<toml::date>());
            }
            case value::time_tag:
            {
                return boost::posix_time::to_tm(v.get<toml::time>());
            }
            case value::local_datetime_tag:
            {
                return boost::posix_time::to_tm(v.get<toml::local_datetime>());
            }
            case value::offset_datetime_tag:
            {
                return boost::local_time::to_tm(v.get<toml::offset_datetime>());
            }
            default:
            {
                throw bad_get((boost::format("toml::get: type of toml value is "
                    "`toml::%1%`, but type `%2%` is specified.") % v.which() %
                    boost::typeindex::type_id<T>().pretty_name()).str());
            }
        }
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<T>().pretty_name()).str());
    }
}

// array_like
template<typename Array>
typename boost::enable_if<is_array_like<Array>, Array>::type
get(const toml::value& v)
{
    typedef typename Array::value_type value_type;
    try
    {
        toml::array const& ar = v.get<toml::array>();

        Array retval(ar.size());
        typename Array::iterator out(retval.begin());
        for(toml::array::const_iterator i(ar.begin()), e(ar.end()); i!=e; ++i)
        {
            *(out++) = get<value_type>(*i);
        }
        return retval;
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<Array>().pretty_name()).str());
    }

}

// (boost|std)::array
template<typename Array>
typename boost::enable_if<is_fixed_size_array<Array>, Array>::type
get(const toml::value& v)
{
    typedef typename Array::value_type value_type;
    try
    {
        toml::array const& ar = v.get<toml::array>();
        Array retval;
        if(ar.size() != retval.size())
        {
            throw std::out_of_range((boost::format("toml::get<%1%>: specified "
                "size differs from its actual size (%2% != %3%).") %
                boost::typeindex::type_id<Array>().pretty_name() %
                retval.size() % ar.size()).str());
        }

        typename Array::iterator out(retval.begin());
        for(toml::array::const_iterator i(ar.begin()), e(ar.end()); i!=e; ++i)
        {
            *(out++) = get<value_type>(*i);
        }
        return retval;
    }
    catch(boost::bad_get const& bg) // thrown from v.get<toml::array>
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<Array>().pretty_name()).str());
    }
}

// table-like case
template<typename Map>
typename boost::enable_if<is_map_like<Map>, Map>::type
get(const toml::value& v)
{
    try
    {
        toml::table const& tb = v.get<toml::table>();
        Map retval;
        for(toml::table::const_iterator i(tb.begin()), e(tb.end()); i!=e; ++i)
        {
            retval.insert(*i);
        }
        return retval;
    }
    catch(boost::bad_get const& bg) // thrown from v.get<toml::table>
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<Map>().pretty_name()).str());
    }
}

// pair
template<typename Pair>
typename boost::enable_if<is_pair_type<Pair>, Pair>::type
get(const toml::value& v)
{
    typedef typename detail::first_type_of<Pair>::type  first_type;
    typedef typename detail::second_type_of<Pair>::type second_type;
    try
    {
        toml::array const& ar = v.get<toml::array>();
        if(ar.size() != 2)
        {
            throw std::out_of_range((boost::format("toml::get<pair<T1, T2>>: "
                "no enough size (%1% != 2).") % ar.size()).str());
        }
        return std::make_pair(toml::get< first_type>(ar.at(0)),
                              toml::get<second_type>(ar.at(1)));
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<Pair>().pretty_name()).str());
    }
}

#ifdef TOML_HAS_CXX11_TUPLE
namespace detail
{

template<typename Tuple, std::size_t ...I>
Tuple get_tuple_impl(const toml::array& a, index_sequence<I...>)
{
    return std::make_tuple(
        toml::get<typename std::tuple_element<I, Tuple>::type>(a.at(I))...);
}

} // detail

// c++11 tuple
template<typename Tuple>
typename boost::enable_if<is_tuple_type<Tuple>, Tuple>::type
get(const toml::value& v)
{
    constexpr std::size_t SZ = std::tuple_size<Tuple>::value;
    try
    {
        toml::array const& ar = v.get<toml::array>();
        if(ar.size() != SZ)
        {
            throw std::out_of_range((boost::format("toml::get<tuple>: "
                "no enough size (%1% != %2%).") % ar.size() % SZ).str());
        }
        return detail::get_tuple_impl<Tuple>(ar, make_index_sequence<SZ>{});
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<Tuple>().pretty_name()).str());
    }
}
#endif// TOML_HAS_CXX11_TUPLE

#ifdef TOML_HAS_CXX11_CHRONO

// c++11 chrono
template<typename TimePoint>
typename boost::enable_if<
    is_std_chrono_system_clock_time_point<TimePoint>, TimePoint>::type
get(const toml::value& v)
{
    try
    {
        switch(v.which())
        {
            case value::date_tag:
            {
                const std::time_t t = boost::posix_time::to_time_t(
                    boost::posix_time::ptime(
                        v.get<date>(), boost::posix_time::hours(0)));
                return std::chrono::system_clock::from_time_t(t);
            }
            case value::local_datetime_tag:
            {
                const std::time_t t = boost::posix_time::to_time_t(
                        v.get<local_datetime>());
                return std::chrono::system_clock::from_time_t(t);
            }
            case value::offset_datetime_tag:
            {
                const std::time_t t = boost::posix_time::to_time_t(
                        v.get<offset_datetime>().utc_time());
                return std::chrono::system_clock::from_time_t(t);
            }
            default:
            {
                throw bad_get((boost::format("toml::get: type of toml value is "
                    "`toml::%1%`, but type `%2%` is specified.") % v.which() %
                    boost::typeindex::type_id<TimePoint>().pretty_name()).str());
            }
        }
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<TimePoint>().pretty_name()).str());
    }
}

template<typename Duration>
typename boost::enable_if<is_std_chrono_duration<Duration>, Duration>::type
get(const toml::value& v)
{
    try
    {
        const time& t = v.get<time>();
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
        return std::chrono::duration_cast<Duration>(
                std::chrono::nanoseconds(t.total_nanoseconds()));
#else
        return std::chrono::duration_cast<Duration>(
                std::chrono::microseconds(t.total_microseconds()));
#endif
    }
    catch(boost::bad_get const& bg)
    {
        throw bad_get((boost::format("toml::get: type of toml value is "
            "`toml::%1%`, but type `%2%` is specified.") % v.which() %
            boost::typeindex::type_id<Duration>().pretty_name()).str());
    }
}

#endif // TOML_HAS_CXX11_CHRONO

} // toml
#endif// TOML98_GET_HPP
