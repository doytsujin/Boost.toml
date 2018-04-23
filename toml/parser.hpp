#ifndef TOML_PARSER_HPP
#define TOML_PARSER_HPP
#include <toml/result.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/static_assert.hpp>
#include <stdexcept>
#include <istream>
#include <sstream>
#include <fstream>

namespace toml
{
namespace detail
{

// for error messages. not for parser.
template<typename InputIterator>
InputIterator
find_linebreak(const InputIterator first, const InputIterator last)
    BOOST_NOEXCEPT_OR_NOTHROW
{
    BOOST_STATIC_ASSERT(boost::is_same<
            typename boost::iterator_value<InputIterator>::type, char>::value);
    const InputIterator CR = std::find(first, last, '\r');
    if(CR != last)
    {
        InputIterator nxt = CR;
        if(*++nxt == '\n')
        {
            return CR;
        }
        else
        {
            return find_linebreak(nxt, last);
        }
    }
    return std::find(first, last, '\n');
}

template<typename Iterator, typename Predicate>
bool all_of(const Iterator first, const Iterator last, Predicate pred)
{
    for(Iterator iter = first; iter != last; ++iter)
    {
        if(!pred(*iter)){return false;}
    }
    return true;
}

inline bool ishex(const char c) BOOST_NOEXCEPT_OR_NOTHROW
{
    return ('0' <= c && c <= '9') ||
           ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f');
}
inline bool isdec(const char c) BOOST_NOEXCEPT_OR_NOTHROW
{
    return ('0' <= c && c <= '9');
}
inline bool isoct(const char c) BOOST_NOEXCEPT_OR_NOTHROW
{
    return ('0' <= c && c <= '7');
}
inline bool isbin(const char c) BOOST_NOEXCEPT_OR_NOTHROW
{
    return ('0' == c || c == '1');
}

inline std::string read_utf8_codepoint(const std::string& str)
{
    boost::uint_least32_t codepoint;
    std::istringstream iss(str);
    iss >> std::hex >> codepoint;

    std::string character;
    if(codepoint < 0x80)
    {
        character += static_cast<unsigned char>(codepoint);
    }
    else if(codepoint < 0x800)
    {
        character += static_cast<unsigned char>(0xC0| codepoint >> 6);
        character += static_cast<unsigned char>(0x80|(codepoint & 0x3F));
    }
    else if(codepoint < 0x10000)
    {
        character += static_cast<unsigned char>(0xE0| codepoint >> 12);
        character += static_cast<unsigned char>(0x80|(codepoint >> 6 & 0x3F));
        character += static_cast<unsigned char>(0x80|(codepoint      & 0x3F));
    }
    else
    {
        character += static_cast<unsigned char>(0xF0| codepoint >> 18);
        character += static_cast<unsigned char>(0x80|(codepoint >> 12 & 0x3F));
        character += static_cast<unsigned char>(0x80|(codepoint >> 6  & 0x3F));
        character += static_cast<unsigned char>(0x80|(codepoint       & 0x3F));
    }
    return character;
}

// XXX: this funciton is used in the loop in parse_string.
//      So, unlike the other parse_* functions, it returns an iterator that
//      points the last element of the escape_sequence for ease.
template<typename InputIterator>
result<string, InputIterator>
parse_escape_sequence(const InputIterator first, const InputIterator last)
{
    // literal tag does not have any escape sequence. the result is basic_string
    typedef result<string, InputIterator> result_t;

    InputIterator iter = first;
    if(iter == last || *iter != '\\')
    {
        throw std::invalid_argument(
                "toml::detail::unescape: got empty or invalid string");
    }
    ++iter; // this is for backslash.

    switch(*iter)
    {
        case '\\': return result_t(string("\\", basic_string),iter,success_t());
        case '"' : return result_t(string("\"", basic_string),iter,success_t());
        case 'b' : return result_t(string("\b", basic_string),iter,success_t());
        case 't' : return result_t(string("\t", basic_string),iter,success_t());
        case 'n' : return result_t(string("\n", basic_string),iter,success_t());
        case 'f' : return result_t(string("\f", basic_string),iter,success_t());
        case 'r' : return result_t(string("\r", basic_string),iter,success_t());
        case 'u' :
        {
            if(std::distance(iter, last) < 5)
            {
                return result_t("toml::detail::parse_escape_sequence: "
                    "\\uXXXX must have 4 numbers -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }

            InputIterator cp_begin = iter; std::advance(cp_begin, 1);
            InputIterator cp_end   = iter; std::advance(cp_end,   5);
            if(!::toml::detail::all_of(cp_begin, cp_end, ishex))
            {
                return result_t("toml::detail::parse_escape_sequence: "
                    "\\uXXXX must be represented by hex -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }

            const std::string unesc =
                read_utf8_codepoint(std::string(cp_begin, cp_end));
            return result_t(string(unesc, basic_string), cp_end, success_t());
        }
        case 'U':
        {
            if(std::distance(iter, last) < 9)
            {
                return result_t("toml::detail::parse_escape_sequence: "
                    "\\UXXXXXXXX must have 8 numbers -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }

            InputIterator cp_begin = iter; std::advance(cp_begin, 1);
            InputIterator cp_end   = iter; std::advance(cp_end,   9);
            if(!::toml::detail::all_of(cp_begin, cp_end, ishex))
            {
                return result_t("toml::detail::parse_escape_sequence: "
                    "\\UXXXXXXXX must be represented by hex -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }

            const std::string unesc =
                read_utf8_codepoint(std::string(cp_begin, cp_end));
            return result_t(string(unesc, basic_string), cp_end, success_t());
        }
        default:
        {
            return result_t("toml::detail::parse_escape_sequence: "
                "unknown escape sequence appeared. -> " +
                std::string(first, find_linebreak(first, last)),
                iter, failure_t());
        }
    }
}

// returns iterator that points next of the value
// t r u e
//         ^ return here
template<typename InputIterator>
result<boolean, InputIterator>
parse_boolean(const InputIterator first, const InputIterator last)
{
    BOOST_STATIC_ASSERT(boost::is_same<
            typename boost::iterator_value<InputIterator>::type, char>::value);
    typedef result<boolean, InputIterator> result_t;

    if(first == last)
    {
        return result_t(std::string(
            "toml::detail::parse_boolean: input is empty."),
            first, failure_t());
    }

    InputIterator iter = first;
    if(*iter == 't')
    {
        if(++iter != last && *iter == 'r' &&
           ++iter != last && *iter == 'u' &&
           ++iter != last && *iter == 'e')
        {
            return result_t(true, ++iter, success_t());
        }
        return result_t("toml::detail::parse_boolean: boolean keywords are "
            "`true` or `false` -> " +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }
    else if(*iter == 'f')
    {
        if(++iter != last && *iter == 'a' &&
           ++iter != last && *iter == 'l' &&
           ++iter != last && *iter == 's' &&
           ++iter != last && *iter == 'e')
        {
            return result_t(false, ++iter, success_t());
        }
        return result_t("toml::detail::parse_boolean: boolean keywords are "
            "`true` or `false` -> " +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }
    else if(*iter == 'T' || *iter == 'F')
    {
        return result_t("toml::detail::parse_boolean: boolean keywords are "
            "`true` or `false` -> " +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }
    return result_t(std::string(
        "toml::detail::parse_boolean: failed. try next"), first, failure_t());
}

template<typename InputIterator>
result<integer, InputIterator>
parse_integer(const InputIterator first, const InputIterator last)
{
    BOOST_STATIC_ASSERT(boost::is_same<
            typename boost::iterator_value<InputIterator>::type, char>::value);
    typedef result<integer, InputIterator> result_t;
    char type = 'd'; // d, x, o, b for dec, hex, oct, bin, respectively.

    integer sign = 1;
    InputIterator iter = first;
    if(*iter == '+') {++iter;} else if(*iter == '-') {++iter; sign = -1;}
    if(iter == last)
    {
        return result_t("toml::detail::parse_integer: input is empty -> " +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }

    if(*iter == '0')
    {
        ++iter;
        if(iter == last) {return result_t(0, iter, success_t());}

        const char n = *iter;
        if(n == 'x' || n == 'o' || n == 'b')
        {
            if(sign == -1)
            {
                return result_t("toml::detail::parse_integer: hex, oct, bin "
                    "representations are not allowd to be negative -> " + 
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            ++iter;
            type = n;
        }
        else if('0' <= n && n <= '9')
        {
            return result_t(
                "toml::detail::parse_integer: leading 0 is not allowed -> " +
                std::string(first, find_linebreak(first, last)),
                iter, failure_t());
        }
        else // just 0.
        {
            return result_t(0, iter, success_t());
        }
    }

    const char t = type;
    std::string token;
    bool underscore = true; // to avoid leading _
    for(; iter != last; ++iter)
    {
        const char n = *iter;
        if(n == '_')
        {
            if(underscore)
            {
                return result_t("toml::detail::parse_integer: "
                    "`_` must be surrounded by at least one number -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            else
            {
                underscore = true;
            }
        }
        else if((t == 'd' && isdec(n)) || (t == 'x' && ishex(n)) ||
                (t == 'o' && isoct(n)) || (t == 'b' && isbin(n)))
        {
            token += n;
            underscore = false;
        }
        else
        {
            break;
        }
    }
    if(token.empty())
    {
        return result_t("toml::detail::parse_integer: token is empty. "
            "maybe main part start with invalid character -> " +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }
    switch(t)
    {
        case 'd':
        {
            return result_t(sign * boost::lexical_cast<integer>(token),
                    iter, success_t());
        }
        case 'x':
        {
            integer i;
            std::stringstream iss(token);
            iss >> std::hex >> i;
            return result_t(sign * i, iter, success_t());
        }
        case 'o':
        {
            integer i;
            std::stringstream iss(token);
            iss >> std::oct >> i;
            return result_t(sign * i, iter, success_t());
        }
        case 'b':
        {
            integer ret = 0;
            integer base = 1;
            for(std::string::const_reverse_iterator
                    i(token.rbegin()), e(token.rend()); i!=e; ++i)
            {
                if(*i == '1') {ret += base;}
                base *= 2;
            }
            return result_t(sign * ret, iter, success_t());
        }
        default:
        {
            throw std::logic_error("toml::detail::integer_parser: "
                    "internal error: NEVER REACH HERE");
        }
    }
}

template<typename InputIterator>
result<floating, InputIterator>
parse_floating(const InputIterator first, const InputIterator last)
{
    BOOST_STATIC_ASSERT(boost::is_same<
            typename boost::iterator_value<InputIterator>::type, char>::value);
    typedef result<floating, InputIterator> result_t;

    floating sign = 1;
    InputIterator iter = first;
    if(*iter == '+') {++iter;} else if(*iter == '-') {++iter; sign = -1;}
    if(iter == last)
    {
        return result_t("toml::detail::parse_integer: input is empty -> " +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }

    // special values (nan, inf)
    if(*iter == 'n')
    {
        if(++iter != last && *iter == 'a' && ++iter != last && *iter == 'n')
        {
            return result_t(std::numeric_limits<floating>::quiet_NaN(),
                    iter, success_t());
        }
        return result_t(
            "toml::detail::parse_floating: unknown keyword, maybe `nan`? ->" +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }
    if(*iter == 'i')
    {
        if(++iter != last && *iter == 'n' && ++iter != last && *iter == 'f')
        {
            return result_t(
                sign * std::numeric_limits<floating>::infinity(),
                iter, success_t());
        }
        return result_t(
            "toml::detail::parse_floating: unknown keyword, maybe `inf`? ->" +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }

    std::string token;
    bool underscore = true; // to avoid leading _
    bool integer_part_started = false;
    bool is_fractional_part   = false;
    bool is_exponential_part  = false;
    // integer part...
    if(*iter == '0')
    {
        token += *iter;
        ++iter;
        if(iter == last)
        {
            return result_t("toml::detail::parse_floating: got `0`. "
                "it's not a floating point number, it's integer -> " +
                std::string(first, find_linebreak(first, last)),
                iter, failure_t());
        }
        integer_part_started = true;
    }
    for(; iter != last; ++iter)
    {
        const char n = *iter;
        if(n == '_')
        {
            if(underscore)
            {
                return result_t("toml::detail::parse_floating: "
                    "`_` must be surrounded by at least one number -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            else
            {
                underscore = true;
            }
        }
        else if(isdec(n))
        {
            token += n;
            underscore = false;
            integer_part_started = true;
        }
        else if(n == '.')
        {
            if(!integer_part_started)
            {
                return result_t(
                    "toml::detial::parse_floating: invalid `.` appeared -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            is_fractional_part = true;
            token += n;
            ++iter;
            break;
        }
        else if(n == 'e' || n == 'E')
        {
            if(!integer_part_started)
            {
                return result_t(
                    "toml::detial::parse_floating: invalid `e|E` appeared -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            is_exponential_part = true;
            token += n;
            ++iter;
            break;
        }
        else
        {
            break;
        }
    }
    if(is_fractional_part)
    {
        underscore = true; //to avoid leading 0 for fractional part (ex. 0._1)
        bool fractional_part_has_size = false;
        for(; iter != last; ++iter)
        {
            const char n = *iter;
            if(n == '_')
            {
                if(underscore)
                {
                    return result_t("toml::detail::parse_floating: "
                        "`_` must be surrounded by at least one number -> " +
                        std::string(first, find_linebreak(first, last)),
                        iter, failure_t());
                }
                else
                {
                    underscore = true;
                }
            }
            else if(isdec(n))
            {
                token += n;
                underscore = false;
                fractional_part_has_size = true;
            }
            else if(n == '.')
            {
                return result_t(
                    "toml::detial::parse_floating: invalid `.` appeared -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            else if(n == 'e' || n == 'E')
            {
                is_exponential_part = true;
                token += n;
                ++iter;
                break;
            }
            else
            {
                if(!fractional_part_has_size)
                {
                    return result_t("toml::detial::parse_floating: "
                        "fractional part must have size -> " +
                        std::string(first, find_linebreak(first, last)),
                        iter, failure_t());
                }
                break;
            }
        }
    }
    if(is_exponential_part)
    {
        underscore = true; //to avoid leading 0 for fractional part (ex. 1e_10)
        if(*iter == '+' || *iter == '-')
        {
            token += *iter;
            ++iter;
        }
        for(; iter != last; ++iter)
        {
            const char n = *iter;
            if(n == '_')
            {
                if(underscore)
                {
                    return result_t("toml::detail::parse_floating: "
                        "`_` must be surrounded by at least one number -> " +
                        std::string(first, find_linebreak(first, last)),
                        iter, failure_t());
                }
                else
                {
                    underscore = true;
                }
            }
            else if(isdec(n))
            {
                token += n;
                underscore = false;
            }
            else if(n == '.')
            {
                return result_t(
                    "toml::detial::parse_floating: invalid `.` appeared -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            else if(n == 'e' || n == 'E')
            {
                return result_t(
                    "toml::detial::parse_floating: invalid `e|E` appeared -> " +
                    std::string(first, find_linebreak(first, last)),
                    iter, failure_t());
            }
            else
            {
                break;
            }
        }
    }
    if(token.empty())
    {
        return result_t("toml::detail::parse_floating: input is empty -> " +
            std::string(first, find_linebreak(first, last)), iter, failure_t());
    }
    return result_t(sign * boost::lexical_cast<floating>(token),
                    iter, success_t());
}

template<typename InputIterator>
result<string, InputIterator>
parse_multi_basic_string(const InputIterator first, const InputIterator last)
{
    return result<string, InputIterator>("TODO", first, failure_t());
}

template<typename InputIterator>
result<string, InputIterator>
parse_multi_literal_string(const InputIterator first, const InputIterator last)
{
    return result<string, InputIterator>("TODO", first, failure_t());
}

template<typename InputIterator>
result<string, InputIterator>
parse_basic_string(const InputIterator first, const InputIterator last)
{
    InputIterator iter(first);
    if(*iter != '"')
    {
        throw std::invalid_argument("toml::detail::parse_basic_string: "
                "internal error appeared");
    }
    ++iter;

    std::string token;
    for(; iter != last; ++iter)
    {
        if(*iter == '"')
        {
            return result_t(string(token, string::basic), iter, success_t());
        }
        else if(*iter == '\\')
        {
            const result_t unesc = parse_escape_sequence(iter, last);
            if(unesc.is_err()) {return unesc;}
            token += unesc.unwrap();
            iter = unesc.iterator();
            // XXX after this, the iterator will be incremented.
            // read the comment at the front of parse_escape_sequence.
        }
        else if(0x00 <= *iter && *iter <= 0x1F || *iter == 0x7F)
        {
            const int ch = *iter;
            std::ostringstream oss; oss << std::hex << ch;
            return result_t("toml::detail::parse_basic_string: "
                "bare control character appeared -> 0x" + oss.str(),
                iter, failure_t());
        }
        else
        {
            token += *iter;
        }
    }
    return result_t("toml::detail::parse_basic_string: "
        "basic string is not closed by `\"` -> " +
        std::string(first, find_linebreak(first, last)), iter, failure_t());
}

template<typename InputIterator>
result<string, InputIterator>
parse_literal_string(const InputIterator first, const InputIterator last)
{
    InputIterator iter(first);
    if(*iter != '\'')
    {
        throw std::invalid_argument("toml::detail::parse_literal_string: "
                "internal error appeared");
    }
    ++iter;

    std::string token;
    for(; iter != last; ++iter)
    {
        if(*iter == '\'')
        {
            return result_t(string(token, string::literal), iter, success_t());
        }
        else if((*iter != 0x09 && 0x00 <= *iter && *iter <= 0x1F) ||
                *iter == 0x7F)
        {
            const int ch = *iter;
            std::ostringstream oss; oss << std::hex << ch;
            return result_t("toml::detail::parse_literal_string: "
                "bare control character appeared -> 0x" + oss.str(),
                iter, failure_t());
        }
        else
        {
            token += *iter;
        }
    }
    return result_t("toml::detail::parse_literal_string: "
        "literal string is not closed by `'` -> " +
        std::string(first, find_linebreak(first, last)), iter, failure_t());
}

template<typename InputIterator>
result<string, InputIterator>
parse_string(const InputIterator first, const InputIterator last)
{
    typedef result<string, InputIterator> result_t;

    if(first == last)
    {
        return result_t(std::string(
            "toml::detail::parse_string: input is empty."), first, failure_t());
    }

    InputIterator iter = first;
    if(*iter == '"')
    {
        if(++iter != last && *iter == '"' && ++iter != last && *iter == '"')
        {
            return parse_multi_basic_string(first, last);
        }
        return parse_basic_string(first, last);
    }
    else if(*iter == '\'')
    {
        if(++iter != last && *iter == '\'' && ++iter != last && *iter == '\'')
        {
            return parse_multi_literal_string(first, last);
        }
        return parse_literal_string(first, last);
    }
    return result_t(std::string(
        "toml::detail::parse_string: failed. try next"), first, failure_t());
}

template<typename InputIterator>
result<toml::value, InputIterator>
parse_value(const InputIterator first, const InputIterator last)
{
    BOOST_STATIC_ASSERT(boost::is_same<
            typename boost::iterator_value<InputIterator>::type, char>::value);
    typedef result<toml::value, InputIterator> result_t;

    if(first == last)
    {
        return result_t(std::string(
            "toml::detail::parse_value: input is empty."), first, failure_t());
    }

    {
        const result<boolean, InputIterator> r = parse_boolean(first, last);
        if(r.is_ok()) {return result_t(r.unwrap(), r.iterator(), success_t());}
        else if(r.iterator() != first) {return r;} // partial match
    }
    {
        // floating parser should be applied earlier than integer parser.
        const result<floating, InputIterator> r = parse_floating(first, last);
        if(r.is_ok()) {return result_t(r.unwrap(), r.iterator(), success_t());}
        else if(r.iterator() != first) {return r;}
    }
    {
        const result<integer, InputIterator> r = parse_integer(first, last);
        if(r.is_ok()) {return result_t(r.unwrap(), r.iterator(), success_t());}
        else if(r.iterator() != first) {return r;}
    }
    {
        const result<string, InputIterator> r = parse_string(first, last);
        if(r.is_ok()) {return result_t(r.unwrap(), r.iterator(), success_t());}
        else if(r.iterator() != first) {return r;}
    }
    return result_t("toml::detail::parse_value: unknown token appeared -> " +
        std::string(first, find_linebreak(first, last)), first, failure_t());
}

} // detail
} // toml
#endif// TOML_PARSER_HPP
