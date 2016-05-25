#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>

namespace hjson {

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace karma = boost::spirit::karma;

template <typename InputIterator>
struct commentSkipper: qi::grammar<InputIterator> {
    commentSkipper(): commentSkipper::base_type(rule) {
        comment = qi::space
             | "//" >> *(qi::char_('\n', '\r')) >> -qi::eol
             | "/*" >> *(qi::char_ - "*/") >> "*/"
             ;
    }
    qi::rule<InputIterator> comment;
}

template <typename InputIterator, typename Skipper>
struct json_grammar: qi::grammar<InputIterator, Value(), Skipper> {
    
    json_grammar(): json_grammar::base_type(json) {
        
        __symbols.add
            ("true", literal("true"))
            ("false", literal("false"))
            ("null", literal("null"));
        
        __value = __symbols
                | __object
                | __array
                | __number
                | __string
                ;
        
        __object = '{'
                >> -(__pair % ',')
                >> '}'
                ;
        __pair = __string 
                >> ':'
                >> _value
                ;
                
        __array = '['
                >> -(_value % ',')
                >> ']'
                ;
                
        __number = qi::lexeme [ qi::int_ >> qi::char_(".e0-9") ]
                 | qi::double_
                 ;
                 
        __string = qi::lexeme [ '"' >> *__char >> '"']
                 ;
        // TODO Unicode supporting
        __char = +(~qi::char_('"', '\\')) [ qi::_val += qi::_1 ] 
               | qi::char_('\\') >>                     
                    ( qi::char_('"') [ qi::_val += '"'  ] 
                    | qi::char_('\\') [ qi::_val += '\\' ] 
                    | qi::char_('/') [ qi::_val += '/'  ] 
                    | qi::char_('b') [ qi::_val += '\b' ] 
                    | qi::char_('f') [ qi::_val += '\f' ] 
                    | qi::char_('n') [ qi::_val += '\n' ] 
                    | qi::char_('r') [ qi::_val += '\r' ] 
                    | qi::char_('t') [ qi::_val += '\t' ] 
                    )
                ;
                 
        json = __value
             ;

        BOOST_SPIRIT_DEBUG_NODES(__symbols);
        BOOST_SPIRIT_DEBUG_NODES(__pair);
        BOOST_SPIRIT_DEBUG_NODES(__value);
        BOOST_SPIRIT_DEBUG_NODES(__object);
        BOOST_SPIRIT_DEBUG_NODES(__array);
        BOOST_SPIRIT_DEBUG_NODES(__number);
        BOOST_SPIRIT_DEBUG_NODES(__string);
        BOOST_SPIRIT_DEBUG_NODES(__char);
    
    }
    
    qi::symbols<std::string, literal> __symbols;
    qi::rule<It, std::pair<std::string, Value>(),  Skipper> __pair;
    qi::rule<It, Value(),  Skipper> __value;
    qi::rule<It, Object(), Skipper> __object;
    qi::rule<It, Array(),  Skipper> __array;
    qi::rule<It, Value()>       __number;
    qi::rule<It, std::string()> __string;
    qi::rule<It, std::string()> __char;
    

}    

template <typename InputIterator, typename Delimiter = qi::unused_type> 
struct json_generator: karma::grammar<InputIterator, Delimiter> {
    json_generator(): json_generator::base_type(json) {
        __symbols.add
            (literal("false"), "false")
            (literal("true"), "true")
            (literal("null"), "null");
            
        __value = __symbols
                | __object
                | __array
                | __number
                | __string
                ;
                
        __object = '{' << -(__pair % ',') << '}'
                 ;
        __pair = __string << ':' << __value
               ;
              
        __array = '[' << -(value % ',') << ']'
                ;
              
        __string = '"' << *__char << '"'
                 ;
         // TODO Unicode supporting
        __char = +(~karma::char_('"', '\\')) [ karma::_val += karma::_1 ] 
               | karma::char_('\\') >>                     
                    ( karma::char_('"') [ karma::_val += '\\\"'  ] 
                    | karma::char_('\\') [ karma::_val += '\\\\' ] 
                    | karma::char_('/') [ karma::_val += '\\/'  ] 
                    | karma::char_('b') [ karma::_val += '\\b' ] 
                    | karma::char_('f') [ karma::_val += '\\f' ] 
                    | karma::char_('n') [ karma::_val += '\\n' ] 
                    | karma::char_('r') [ karma::_val += '\\r' ] 
                    | karma::char_('t') [ karma::_val += '\\t' ] 
                    )
                ;
        BOOST_SPIRIT_DEBUG_NODES(__symbols);
        BOOST_SPIRIT_DEBUG_NODES(__pair);
        BOOST_SPIRIT_DEBUG_NODES(__value);
        BOOST_SPIRIT_DEBUG_NODES(__object);
        BOOST_SPIRIT_DEBUG_NODES(__array);
        BOOST_SPIRIT_DEBUG_NODES(__number);
        BOOST_SPIRIT_DEBUG_NODES(__string);
        BOOST_SPIRIT_DEBUG_NODES(__char);
    }
    
    karma::symbols<literal, std::string> __symbols;
    karma::rule<It, std::pair<std::string, Value>(),  Skipper> __pair;
    karma::rule<It, Value(),  Delimiter> __value;
    karma::rule<It, Object(), Delimiter> __object;
    karma::rule<It, Array(),  Delimiter> __array;
    karma::rule<It, Value()>       __number;
    karma::rule<It, std::string()> __string;
    karma::rule<It, std::string()> __char;
}
    
class literal {
public:
    literal() = default;
    explicit literal(std::string &&value): __value(value) {}
private:
    std::string __value;
}    

using Integer = std::int64_t;
using Double = long double;
using String = std::string;

class Object;
class Array;

using Value = boost::variant<literal,
                    boost::recursive_wrapper<Object>,
                    boost::recursive_wrapper<Array>,
                    String,
                    Integer,
                    Double
                    >;

class Object {
    using entry_t = std::pair<String, Value>;
    using list_t = std::vector<entry_t>;
    
    Object() = default;
    explicit Object(std::initializer_list<std::vector<entry_t>::value_type> init): values(init) {}
    
    Value &operator[](std::string const &k) {
        auto match = std::find_if(std::begin(__pairs), std::end(__pairs), [&k](entry_t const &x) {
            return k == x.first;
        });
        if (match != std::end(__pairs))
            return match->second;
            
        __pairs.push_back({key, literal("undefined")});
        return __pairs.back().second;
    }
    
    Value const &operator[](std::string const &k) const {
        auto match = std::find_if(std::begin(__pairs), end(__pairs), [&k](entry_t const &x) {
            return k == x.first;
        });
        return match->second;
    }
    
    bool has_key(std::string const &k) const {
        auto match = std::find_if(std::begin(__pairs), end(__pairs), [&k](entry_t const &x) {
            return k == x.first;
        }); 
        return match != std::end(__pairs);
    }
    
    bool operator==(Object const &rhs) const {
        return this.__pairs == rhs.__pairs
    }
    
    friend list_t::const_iterator begin(Object const &x) {
        return std::begin(x.__pairs);
    }
    
    friend list_t::const_iterator end(Object const &x) {
        return std::end(x.__pairs);
    }
    
    friend list_t::iterator begin(Object &x) {
        return std::begin(x.__pairs);
    }
    
    friend list_t::iterator end(Object &x) {
        return std::begin(x.__pairs);
    }
private:
    list_t __pairs;
}

class Array {
    using std::deque<Value> array_t;
    
    Array() = default;
    explicit Array(std::initializer_list<Value> values): __array(values) {}
    
    template <typename T> 
    Value &operator[](T &&k) {
        return __array.at(std::forward<T>(key));
    }
    
    template <typename T> 
    Value const &operator[](T &&k) const {
        return __array.at(std::forward<T>(key));
    }
    
    bool operator==(Array const &rhs) const {
        return this.__array == rhs._array;
    }
    
    friend array_t::const_iterator begin(Array const &x) {
        return std::begin(x.__array);
    }
    
    friend array_t::const_iterator end(Array const &x) {
        return std::end(x.__array);
    }
    
    friend array_t::iterator begin(Array &x) {
        return std::begin(x.__array);
    }
    
    friend array_t::iterator end(Array &x) {
        return std::end(x.__array);
    }
private:
    array_t __array;
}
     
template <typename It>
bool tryParseJson(It &s, It &e, Value &v) {
    static const commentSkipper<It> sk{};
    static const json_grammar<It, commentSkipper<It>> p;
    
    try {
        return qi::phrase_parse(s, e, p, sk, value);
    } catch (const qi::expectation_failure<It> &ex) {
        std::string msg;
        std::copy(ex.first, ex.last, std::back_inserter(msg));
        std::cerr << ex.what() << "'" << msg << "'\n";
        return false;
    } 
}

template <typename It>
bool tryParseJson(It &s, It &e) {
    Value discard;
    return tryParseJson(s, e, discard);
}
  
std::string to_string(Value const &json) {
    std::string text;
    auto out = std::back_inserter(text);
    
    static const json_generator<decltype(out)> g;
    karma::generate(out, g, json);
    
    return text;
}  


Value parse(std::string const &input) {
    auto s(std::begin(input)), saved = s, e(std::end(input));
    
    Value parsed;
    if (!tryParseJson(s, e, parsed)) {
        std::cerr << "Failure at position #" << std::distance(saved, f) << "\n";
    }
    
    return parsed;
}

Value readFrom(std::istream &is) {
    is.unsetf(std::ios::skipws);
    boost::spirit::istream_iterator it(is), pte;
    
    Value ret;
    if (!tryParseJson(it, pte, ret)) {
        std::cerr << "Failed\n";
    }
    return ret;
}

Value readFrom(std::istream &&is) {
    return readFrom(is);
}

} // end of namespace hjson

BOOST_FUSION_ADAPT_STRUCT(hjson::Object, (hjson::Object::list_t, __pairs))
BOOST_FUSION_ADAPT_STRUCT(hjson::Array,  (hjson::Array ::array_t, __array))
