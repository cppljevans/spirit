/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_DETAIL_RULE_JAN_08_2012_0326PM)
#define BOOST_SPIRIT_X3_DETAIL_RULE_JAN_08_2012_0326PM

#include <boost/core/ignore_unused.hpp>
#include <boost/spirit/home/x3/auxiliary/guard.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/directive/expect.hpp>
#include <boost/spirit/home/x3/support/traits/make_attribute.hpp>
#include <boost/spirit/home/x3/support/utility/sfinae.hpp>
#include <boost/spirit/home/x3/nonterminal/detail/transform_attribute.hpp>
#include <boost/utility/addressof.hpp>

#if defined(BOOST_SPIRIT_X3_DEBUG)
#include <boost/spirit/home/x3/nonterminal/simple_trace.hpp>
#endif

namespace boost { namespace spirit { namespace x3
{
    template <typename ID>
    struct identity;

    template <typename ID, typename Attribute = unused_type, bool force_attribute = false>
    struct rule;

    struct parse_pass_context_tag;

    namespace detail
    {
        // we use this so we can detect if the default parse_rule
        // is the being called.
        struct default_parse_rule_result
        {
            default_parse_rule_result(bool r)
              : r(r) {}
            operator bool() const { return r; }
            bool r;
        };
    }

    // default parse_rule implementation
    template <typename ID, typename Attribute, typename Iterator
      , typename Context, typename ActualAttribute>
    inline detail::default_parse_rule_result
    parse_rule(
        rule<ID, Attribute> rule_
      , Iterator& first, Iterator const& last
      , Context const& context, ActualAttribute& attr);
}}}

namespace boost { namespace spirit { namespace x3 { namespace detail
{
#if defined(BOOST_SPIRIT_X3_DEBUG)
    template <typename Iterator, typename Attribute>
    struct context_debug
    {
        context_debug(
            char const* rule_name
          , Iterator const& first, Iterator const& last
          , Attribute const& attr
          , bool const& ok_parse //was parse successful?
          )
          : ok_parse(ok_parse), rule_name(rule_name)
          , first(first), last(last)
          , attr(attr)
          , f(detail::get_simple_trace())
        {
            f(first, last, attr, pre_parse, rule_name);
        }

        ~context_debug()
        {
            auto status = ok_parse ? successful_parse : failed_parse ;
            f(first, last, attr, status, rule_name);
        }

        bool const& ok_parse;
        std::string const rule_name;
        Iterator const& first;
        Iterator const& last;
        Attribute const& attr;
        detail::simple_trace_type& f;
    };
#endif

    template <typename ID, typename Iterator, typename Context, typename Enable = void>
    struct has_on_error : mpl::false_ {};

    template <typename ID, typename Iterator, typename Context>
    struct has_on_error<ID, Iterator, Context,
        typename disable_if_substitution_failure<
            decltype(
                std::declval<ID>().on_error(
                    std::declval<Iterator&>()
                  , std::declval<Iterator>()
                  , std::declval<expectation_failure<Iterator>>()
                  , std::declval<Context>()
                )
            )>::type
        >
      : mpl::true_
    {};

    template <typename ID, typename Iterator, typename Attribute, typename Context, typename Enable = void>
    struct has_on_success : mpl::false_ {};

    template <typename ID, typename Iterator, typename Attribute, typename Context>
    struct has_on_success<ID, Iterator, Context, Attribute,
        typename disable_if_substitution_failure<
            decltype(
                std::declval<ID>().on_success(
                    std::declval<Iterator&>()
                  , std::declval<Iterator>()
                  , std::declval<Attribute&>()
                  , std::declval<Context>()
                )
            )>::type
        >
      : mpl::true_
    {};

    template <typename ID>
    struct make_id
    {
        typedef identity<ID> type;
    };

    template <typename ID>
    struct make_id<identity<ID>>
    {
        typedef identity<ID> type;
    };

  #if !BOOST_SPIRIT_X3_EXPERIMENTAL_GET_RHS_NO_CONTEXT
    template <typename ID, typename RHS, typename Context>
    Context const&
    make_rule_context(RHS const& /* rhs */, Context const& context
      , mpl::false_ /* is_default_parse_rule */)
    {
        return context;
    }

    template <typename ID, typename RHS, typename Context>
    auto make_rule_context(RHS const& rhs, Context const& context
      , mpl::true_ /* is_default_parse_rule */ )
    {
        return make_unique_context<ID>(rhs, context);
    }
  #endif// !BOOST_SPIRIT_X3_EXPERIMENTAL_GET_RHS_NO_CONTEXT

      template 
      < typename Attribute
      , typename ID
      >
      /**@brief
       *  Attribute is the rule attribute for rule with id=ID.
       *  IOW, for rule<ID,Attribute,bool force_attribute>
       *  in ../rule.hpp.
       */
    struct rule_parser
    {
        template <typename Iterator, typename Context, typename ActualAttribute>
        static bool call_on_success(
            Iterator& /* first */, Iterator const& /* last */
          , Context const& /* context */, ActualAttribute& /* attr */
          , mpl::false_ /* No on_success handler */ )
        {
            return true;
        }

        template <typename Iterator, typename Context, typename ActualAttribute>
        static bool call_on_success(
            Iterator& first, Iterator const& last
          , Context const& context, ActualAttribute& attr
          , mpl::true_ /* Has on_success handler */)
        {
            bool pass = true;
            ID().on_success
              ( first
              , last
              , attr
              , make_context<parse_pass_context_tag>(pass, context)
              );
            return pass;
        }

        template <typename RHS, typename Iterator, typename Context
          , typename RContext, typename ActualAttribute>
        static bool parse_rhs_have_on_error(
            RHS const& rhs
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, ActualAttribute& attr
          , mpl::false_)
        {
          #if !BOOST_SPIRIT_X3_EXPERIMENTAL_GET_RHS_NO_CONTEXT
            // see if the user has a BOOST_SPIRIT_DEFINE for this rule
            typedef
                decltype(parse_rule(
                    rule<ID, Attribute>(), first, last
                  , make_unique_context<ID>(rhs, context), attr))
            parse_rule_result;

            // If there is no BOOST_SPIRIT_DEFINE for this rule,
            // we'll make a context for this rule tagged by its ID
            // so we can extract the rule later on in the default
            // (generic) parse_rule function.
            typedef
                is_same<parse_rule_result, default_parse_rule_result>
            is_default_parse_rule;
            auto ctx=make_rule_context<ID>(rhs, context, is_default_parse_rule());
          #else
            auto&ctx=context;
          #endif

            Iterator i = first;
            bool r = 
              rhs.parse
              ( i
              , last
              , ctx
              , rcontext
              , attr
              );

            if (r)
            {
                auto first_ = first;
                x3::skip_over(first_, last, context);
                r = call_on_success(first_, i, context, attr
                  , has_on_success<ID, Iterator, Context, ActualAttribute>());
            }

            if (r)
                first = i;
            return r;
        }

        template <typename RHS, typename Iterator, typename Context
          , typename RContext, typename ActualAttribute>
        static bool parse_rhs_have_on_error(
            RHS const& rhs
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, ActualAttribute& attr
          , mpl::true_ /* on_error is found */)
        {
            for (;;)
            {
                try
                {
                    return parse_rhs_have_on_error(
                        rhs, first, last, context, rcontext, attr, mpl::false_());
                }
                catch (expectation_failure<Iterator> const& x)
                {
                    switch (ID().on_error(first, last, x, context))
                    {
                        case error_handler_result::fail:
                            return false;
                        case error_handler_result::retry:
                            continue;
                        case error_handler_result::accept:
                            return true;
                        case error_handler_result::rethrow:
                            throw;
                    }
                }
            }
        }

        template <typename RHS, typename Iterator
          , typename Context, typename RContext, typename ActualAttribute>
        static bool parse_rhs_main(
            RHS const& rhs
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, ActualAttribute& attr)
        {
            return parse_rhs_have_on_error(
                rhs, first, last, context, rcontext, attr
              , has_on_error<ID, Iterator, Context>()
            );
        }

        template <typename RHS, typename Iterator
          , typename Context, typename RContext, typename ActualAttribute>
        static bool parse_rhs(
            RHS const& rhs
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, ActualAttribute& attr
          , mpl::false_)
        {
            return parse_rhs_main(rhs, first, last, context, rcontext, attr);
        }

        template <typename RHS, typename Iterator
          , typename Context, typename RContext, typename ActualAttribute>
        static bool parse_rhs(
            RHS const& rhs
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, ActualAttribute& /* attr */
          , mpl::true_)
        {
            return parse_rhs_main(rhs, first, last, context, rcontext, unused);
        }
          template
          < typename Iterator
          , typename ActualAttribute
          , typename Parser
          >
          static bool 
        rule_attr_transform_f
          ( char const* rule_name 
          , Iterator& first
          , Iterator const& last
          , ActualAttribute& act_attr
          , Parser parser
          )
        /**
         * @brief
         *  call parser with transformation of act_attr.
         *
         *  The transformed attribute should have type
         *  Attribute (i.e. the rule attribute).
         *  (The last sentence is only a guess, at this
         *  point [2017-11-12], but is based on trace
         *  output produced by the code guarded by the
         *  COMPARE_ACTUAL_TRANS_ATTR macro below.)
         */
        { 
          using make_attribute=traits::make_attribute<Attribute, ActualAttribute>;
        
          // do down-stream transformation, provides attribute for
          // rhs parser
          using transform
            = traits::transform_attribute
              < typename make_attribute::type, Attribute, parser_id
              >;
          using value_type=typename make_attribute::value_type;
          using transform_attr=typename transform::type;
          value_type made_attr{make_attribute::call(act_attr)};
          transform_attr xfrm_attr(transform::pre(made_attr));
        #ifdef COMPARE_ACTUAL_TRANS_ATTR
          trace_scope ts("rule_attr_transform_f");
          using trans_attr=typename remove_reference<transform_attr>::type;
          std::cout<<"ActualAttribute="<<type_name<ActualAttribute>()<<"\n";
          bool const same_actual_trans=is_same<ActualAttribute,trans_attr>::value;
          if(!same_actual_trans)
          {
            std::cout<<"same_actual_trans="<<same_actual_trans<<"\n";
            bool const same_rule_trans=is_same<Attribute,trans_attr>::value;
            std::cout<<"same_rule_trans="<<same_rule_trans<<"\n";
          }
        #endif 
          bool ok_parse
            //Creates a place to hold the result of parse_rhs
            //called inside the following scope.
            ;
          {
           // Create a scope to cause the dbg variable below (within
           // the #if...#endif) to call it's DTOR before any
           // modifications are made to the attribute, xfrm_attr, passed
           // to parse_rhs (such as might be done in
           // traits::post_transform when, for example,
           // ActualAttribute is a recursive variant).
#if defined(BOOST_SPIRIT_X3_DEBUG)
                context_debug<Iterator, transform_attr>
              dbg(rule_name, first, last, xfrm_attr, ok_parse);
#endif
              ok_parse = parser(first, last, xfrm_attr);
          }
          // do up-stream transformation, this integrates the results
          // back into the original attribute value, if appropriate
          if(ok_parse)
          {
              traits::post_transform(act_attr, std::forward<transform_attr>(xfrm_attr));
          }
        #ifdef COMPARE_ACTUAL_TRANS_ATTR
          std::cout<<"ok_parse="<<ok_parse<<"\n";
        #endif
          return ok_parse;
        };//rule_attr_transform_f
  
        template <typename RHS, typename Iterator, typename Context
          , typename ActualAttribute, typename ExplicitAttrPropagation>
        static bool call_rule_definition(
            RHS const& rhs
          , char const* rule_name
          , Iterator& first, Iterator const& last
          , Context const& context
          , ActualAttribute& attr
          , ExplicitAttrPropagation)
        {
          auto const parse_flag=
            mpl::bool_
            < (  RHS::has_action
              && !ExplicitAttrPropagation::value
              )
            >();
          #if BOOST_SPIRIT_X3_EXPERIMENTAL_ATTR_XFORM_IN_RULE
            //rule_attr_transform already done in
            //either rule<...>::parse or rule_definition<...>::parse
            //; hence, don't repeat here.
            bool ok_parse=parse_rhs(rhs, first, last, context, attr, attr
                   , parse_flag
                  );
          #else
            auto parser_f=[&]
              ( Iterator& f_first, Iterator const& f_last
              , auto&_attr
              )
              {  return 
                   parse_rhs(rhs, f_first, f_last, context, _attr, _attr
                     , parse_flag
                   );
              };
            bool ok_parse=
              rule_attr_transform_f
              ( rule_name
              , first, last
              , attr
              , parser_f
              );
          #endif//BOOST_SPIRIT_X3_EXPERIMENTAL_ATTR_XFORM_IN_RULE
            return ok_parse;
        }
    };
}}}}

#endif
