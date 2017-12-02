/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_PARSE_INTO_CONTAINER_JAN_15_2013_0957PM)
#define SPIRIT_PARSE_INTO_CONTAINER_JAN_15_2013_0957PM

#include <type_traits>

#include <boost/spirit/home/x3/support/traits/container_traits.hpp>
//#define BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_COUNT_PASS_FAIL
#ifdef BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_COUNT_PASS_FAIL
namespace boost { namespace spirit { namespace x3 { namespace traits
{
    static unsigned r_pass;
    static unsigned r_fail;
}    
}}}
#endif//BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_COUNT_PASS_FAIL
#ifndef BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_PUSH_BACK_OPT
  #define BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_PUSH_BACK_OPT 1
#endif//BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_PUSH_BACK_OPT
namespace boost { namespace spirit { namespace x3 { namespace traits
{

    template <typename Container>
    inline auto& emplace_back(Container& c)
    { c.emplace_back(); return c.back();
    }
    template <typename Container>
    inline void emplace_back(Container& c, typename Container::value_type& v)
    { c.emplace_back(v);
    }
    inline auto& emplace_back(std::string& c)
    { std::string::value_type v; return c.append(1,v).back();
    }
    inline void emplace_back(std::string& c, typename std::string::value_type v)
    { c.push_back(v);
    }
    template <typename Container>
    inline void pop_back(Container& c)
    { c.pop_back();
    }
}
}}}  
#include <boost/spirit/home/x3/support/traits/value_traits.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/support/traits/handles_container.hpp>
#include <boost/spirit/home/x3/support/traits/has_attribute.hpp>
#include <boost/spirit/home/x3/support/traits/is_substitute.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/mpl/and.hpp>
#include <boost/fusion/include/front.hpp>
#include <boost/fusion/include/back.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace boost { namespace spirit { namespace x3 { namespace detail
{
    template <typename Attribute, typename Value>
    struct saver_visitor;

    // save to associative fusion container where Key is simple type
    template <typename Key, typename Enable = void>
    struct save_to_assoc_attr
    {
        template <typename Value, typename Attribute>
        static void call(const Key, Value& value, Attribute& attr)
        {
            traits::move_to(value, fusion::at_key<Key>(attr));
        }
    };

/*	$$$ clang reports: warning: class template partial specialization contains
 *	a template parameter that can not be deduced; this partial specialization
 *	will never be used $$$
 *
    // save to associative fusion container where Key
    // is variant over possible keys
    template <typename ...T>
    struct save_to_assoc_attr<variant<T...> >
    {
        typedef variant<T...> variant_t;

        template <typename Value, typename Attribute>
        static void call(const variant_t key, Value& value, Attribute& attr)
        {
            apply_visitor(saver_visitor<Attribute, Value>(attr, value), key);
        }
    };
*/
    template <typename Attribute, typename Value>
    struct saver_visitor  : boost::static_visitor<void>
    {
        saver_visitor(Attribute& attr, Value& value)
            : attr(attr), value(value) {};

        Attribute& attr;
        Value& value;

        template <typename Key>
        void operator()(Key) const
        {
            save_to_assoc_attr<Key>::call(Key(), value,attr);
        }
    };

    template <typename Parser, typename Container, typename Context>
    struct parser_accepts_container
        : traits::is_substitute<
                typename traits::attribute_of<Parser, Context>::type
              , Container
            >
    {};

    template <typename Parser>
    struct parse_into_container_base_impl
    {
    private:

        // Parser has attribute (synthesize; Attribute is a container)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize_x(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::false_)
        {
            typedef typename
                traits::container_value<Attribute>::type
            value_type;
          #if BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_PUSH_BACK_OPT
            //The rationale for assuming that, on the average, this
            //runs faster, is that, in both the #if and #else parts
            //a value_type has to be created, but in the #else
            //case, it also has to be copied if r is true.
            //OTOH, in case r is false, then the #if part has to
            //adjust a pointer in attr in addition to what the #else part
            //has to do; hence, it would seem the #if part should run faster
            //since ajusting a pointer is faster than copying a value.
            //Of course, if the parse fails most of the time, then
            //the #else part might run faster because then there would
            //no need to adjust the pointer in attr (assuming attr=vector<T>).
            //
            //Benchmarks supports this:
            //  This shows number of times when parse passed or failed:
            //  
            //    r_fail=11,000,000
            //    r_pass=66,038,000
            //  
            //  and times:
            //  
            //    #if BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_PUSH_BACK_OPT
            //      mean 8.56ms
            //    #else
            //      mean 10.744ms
            //    #endif
            //
            auto&val=traits::emplace_back(attr);
            bool r = parser.parse(first, last, context, rcontext, val);
            if(!r)
              // rm just emplace_back'd val.
              traits::pop_back(attr);
          #else        
            // synthesized attribute needs to be value initialized
            value_type val = traits::value_initialize<value_type>::call();

            bool r = parser.parse(first, last, context, rcontext, val);
            if(r)
              // push the parsed value into our attribute
              traits::push_back(attr, val);
          #endif//BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_PUSH_BACK_OPT
          #ifdef TRACE_PARSE_INTO_CONTAINER
            trace_scope ts("parse_into_container_base_impl::call_synthesize_x(...,accepts_container=mpl::false_)");
            std::cout<<":r="<<r<<":size="<<attr.size()<<":type_name<attr>="<<type_name<Attribute>()
              <<"\n:attr=\n";
            print_attr(std::cout,attr)<<"\n";
          #endif
          #ifdef BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_COUNT_PASS_FAIL
            r?traits::r_pass++
             :traits::r_fail++
             ;
          #endif//BOOST_SPIRIT_PARSE_INTO_CONTAINER_BASE_IMPL_COUNT_PASS_FAIL
            return r;
        }

        // Parser has attribute (synthesize; Attribute is a container)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize_x(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::true_)
        {
            bool r =  parser.parse(first, last, context, rcontext, attr);
            #ifdef TRACE_PARSE_INTO_CONTAINER
              trace_scope ts("parse_into_container_base_impl::call_synthesize_x(...,accepts_container=mpl::true_)");
              std::cout<<":r="<<r<<":size="<<attr.size()<<":type_name<attr>="<<type_name<Attribute>()
                <<"\n:attr=\n";
              print_attr(std::cout,attr)<<"\n";
            #endif
            return r;
        }

        // Parser has attribute (synthesize; Attribute is a container)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr)
        {
            typedef
                parser_accepts_container<Parser, Attribute, Context>
            parser_accepts_container;

            return call_synthesize_x(parser, first, last, context, rcontext, attr
                , parser_accepts_container());
        }

        // Parser has attribute (synthesize; Attribute is a single element fusion sequence)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize_into_fusion_seq(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::false_ /* is_associative */)
        {
            static_assert(traits::has_size<Attribute, 1>::value,
                "Expecting a single element fusion sequence");
            return call_synthesize(parser, first, last, context, rcontext,
                fusion::front(attr));
        }

        // Parser has attribute (synthesize; Attribute is fusion map sequence)
        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call_synthesize_into_fusion_seq(
            Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::true_ /*is_associative*/)
        {
            using attribute_type = typename traits::attribute_of<Parser, Context>::type;
            static_assert(traits::has_size<attribute_type, 2>::value,
                "To parse directly into fusion map parser must produce 2 element attr");

            // use type of  first element of attribute as key
            using key = typename std::remove_reference<
            typename fusion::result_of::front<attribute_type>::type>::type;

            attribute_type attr_;
            if (!parser.parse(first, last, context, rcontext, attr_))
                return false;

            save_to_assoc_attr<key>::call(fusion::front(attr_), fusion::back(attr_), attr);
            return true;
        }

        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call_synthesize_dispatch_by_seq(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::true_ /*is_sequence*/)
        {
            return call_synthesize_into_fusion_seq(
                parser, first, last, context, rcontext, attr
              , fusion::traits::is_associative<Attribute>());
        }

        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call_synthesize_dispatch_by_seq(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::false_ /*is_sequence*/)
        {
            return call_synthesize(parser, first, last, context, rcontext, attr);
        }

        // Parser has attribute (synthesize)
        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::true_)
        {
            return call_synthesize_dispatch_by_seq(parser, first, last, context, rcontext, attr
                , fusion::traits::is_sequence<Attribute>());
        }

        // Parser has no attribute (pass unused)
        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call(
            Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& /* attr */, mpl::false_)
        {
            return parser.parse(first, last, context, rcontext, unused);
        }


    public:

        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr)
        {
            return call(parser, first, last, context, rcontext, attr
              , mpl::bool_<traits::has_attribute<Parser, Context>::value>());
        }
    };

    template <typename Parser, typename Context, typename RContext, typename Enable = void>
    struct parse_into_container_impl : parse_into_container_base_impl<Parser> {};

    template <typename Parser, typename Container, typename Context>
    struct parser_attr_is_substitute_for_container_value
        : traits::is_substitute<
            typename traits::attribute_of<Parser, Context>::type
          , typename traits::container_value<Container>::type
        >
    {};
    
    template <typename Parser, typename Context, typename RContext>
    struct parse_into_container_impl<Parser, Context, RContext,
        typename enable_if<traits::handles_container<Parser, Context>>::type>
    {
        template <typename Iterator, typename Attribute>
        static bool call(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::false_)
        {
            bool r = parse_into_container_base_impl<Parser>::call(
                parser, first, last, context, rcontext, attr);
            #ifdef TRACE_PARSE_INTO_CONTAINER
              trace_scope ts("parse_into_container_impl::call(...,pass_as_is=mpl::false_)");
              std::cout<<":r="<<r<<":size="<<attr.size()<<":type_name<attr>="<<type_name<Attribute>()
                <<"\n:attr=\n";
              print_attr(std::cout,attr)<<"\n";
            #endif
            return r;
        }

        template <typename Iterator, typename Attribute>
        static bool call(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::true_)
        {
          #define BOOST_SPIRIT_X3_EXPERIMENTAL_PARSE_INTO_CONTAINER_IMPL_RESIZE
          #ifdef BOOST_SPIRIT_X3_EXPERIMENTAL_PARSE_INTO_CONTAINER_IMPL_RESIZE
            //Purpose:
            //  Should be faster then the #else branch *if*:
            //    creation in existing attr then erasure of attr elements
            //    in case parsing fails 
            //  is, on the average,  be faster than:
            //    creation into local rest followed by copy into attr
            //    in case parsing succeeds,
            //However(2017-11-08):
            //  * csv_parser benchmark shows no difference :(
            //  * the test/attr.cpp fails at last test.
            auto attr_inp_size=attr.size();
            bool r = parser.parse(first, last, context, rcontext, attr);
            if(!r)
              attr.resize(attr_inp_size);
            #ifdef TRACE_PARSE_INTO_CONTAINER
              trace_scope ts("parse_into_container_impl::call(...,pass_as_is=mpl::true_)");
              std::cout<<":r="<<r<<":size="<<attr.size()<<"\n";
            #endif
          #else
            if (attr.empty())
                return parser.parse(first, last, context, rcontext, attr);
            Attribute rest;
            bool r = parser.parse(first, last, context, rcontext, rest);
            if (r)
                attr.insert(attr.end(), rest.begin(), rest.end());
          #endif//BOOST_SPIRIT_X3_EXPERIMENTAL_PARSE_INTO_CONTAINER_IMPL_ERASE
            return r;
        }

        template <typename Iterator, typename Attribute>
        static bool call(Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr)
        {
            typedef parser_accepts_container<
                    Parser, Attribute, Context>
            parser_accepts_container;

            typedef parser_attr_is_substitute_for_container_value<
                    Parser, Attribute, Context>
            parser_attr_is_substitute_for_container_value;

            typedef mpl::or_<
                parser_accepts_container
              , mpl::not_<parser_attr_is_substitute_for_container_value>>
            pass_attibute_as_is;

            return call(parser, first, last, context, rcontext, attr,
                pass_attibute_as_is());
        }
    };

    template <typename Parser, typename Iterator, typename Context
      , typename RContext, typename Attribute>
    bool parse_into_container(
        Parser const& parser
      , Iterator& first, Iterator const& last, Context const& context
      , RContext& rcontext, Attribute& attr)
    {
        return parse_into_container_impl<Parser, Context, RContext>::call(
            parser, first, last, context, rcontext, attr);
    }

}}}}

#endif
