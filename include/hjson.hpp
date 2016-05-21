#pragma once

#include <boost/spirit/include/qi.hpp>


namespace hjson {

namespace qi = boost::spirit::qi;
namespace spirit = boost::spirit;

template <typename InputIterator>
struct json_grammar: qi::grammar<InputIterator, json_object(), boost::spirit::ascii::space_type> {
    json_grammar(): json_grammar::base_type(json_object_) {
        json_object_ = qi::char_('{') 
                    >> *(json_pair_ % ',')
                    >> qi::lit('};')
                    ;
        json_pair_ = 
        json_string_ %= qi::omit[ qi::char_("\"'")[ spirit::_a = _1 ]]
                     >> qi::lexeme[ *(qi::char_ - qi::char_(spirit::_a) ) ]
                     >> qi::omit[ qi::char_(spirit::_a) ]
                     ;
        json_number_ = qi::int_
                     | qi::double_
                     ;
        json_name_ = qi::lexeme[ ]
    }
}    
    
}