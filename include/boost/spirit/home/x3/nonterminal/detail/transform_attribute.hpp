/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_X3_DETAIL_ATTRIBUTES_APR_18_2010_0458PM)
#define SPIRIT_X3_DETAIL_ATTRIBUTES_APR_18_2010_0458PM

#include <boost/spirit/home/x3/support/traits/transform_attribute.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <utility>

#ifndef BOOST_SPIRIT_X3_EXPERIMENTAL_TRANSFORM_PRE_ITERATOR_RANGE
#define BOOST_SPIRIT_X3_EXPERIMENTAL_TRANSFORM_PRE_ITERATOR_RANGE 1
#endif//BOOST_SPIRIT_X3_EXPERIMENTAL_TRANSFORM_PRE_ITERATOR_RANGE
#if BOOST_SPIRIT_X3_EXPERIMENTAL_TRANSFORM_PRE_ITERATOR_RANGE
#include <boost/fusion/adapted.hpp>
#include <boost/fusion/iterator/deref.hpp>
#endif//BOOST_SPIRIT_X3_EXPERIMENTAL_TRANSFORM_PRE_ITERATOR_RANGE

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace x3
{
    struct parser_id;

    template <typename Exposed, typename Transformed>
    struct default_transform_attribute
    {
        typedef Transformed type;

        static Transformed pre(Exposed& val) 
        { 
          #ifdef TRACE_TRANSFORM_ATTRIBUTE_HPP
            std::cout<<"x3::default_transform_attribute::pre(Exposed& val)\n";
            std::cout<<"type_name<Exposed>="<<type_name<Exposed>()<<"\n";
            std::cout<<"type_name<Transformed>="<<type_name<Transformed>()<<"\n";
          #endif
            return Transformed(); 
        }
        
        static void post(Exposed& val, Transformed&& attr)
        {
            traits::move_to(std::forward<Transformed>(attr), val);
        }
    };

  #if BOOST_SPIRIT_X3_EXPERIMENTAL_TRANSFORM_PRE_ITERATOR_RANGE
      template
      < typename Attribute
      , int IndexBegin
      , typename IterEnd
      , typename Transformed
      >
    struct default_transform_attribute
      < fusion::iterator_range
        < fusion::basic_iterator
          < fusion::struct_iterator_tag
          , fusion::random_access_traversal_tag
          , Attribute
          , IndexBegin
          >
        , IterEnd
        >//Exposed
      , Transformed
      >
    {
        using type=Transformed&;
        using Exposed=
          fusion::iterator_range
          < fusion::basic_iterator
            < fusion::struct_iterator_tag
            , fusion::random_access_traversal_tag
            , Attribute
            , IndexBegin
            >
          , IterEnd
          >;
        static type
        pre
        ( Exposed& attr
        ) 
        { 
        #ifdef TRACE_TRANSFORM_ATTRIBUTE_HPP
          std::cout<<"x3::default_transform_attribute<iterator_range<>,>::pre(iterator_range<...>& attr)\n";
          std::cout<<"type_name<Attribute>="<<type_name<Attribute>()<<"\n";
          std::cout<<"IndexBegin="<<IndexBegin<<"\n";
          using attr_t=decltype(attr);
          std::cout<<"type_name<attr_t>="<<type_name<attr_t>()<<"\n";
        #endif
          return fusion::deref(attr.first);
        }
        
        static void post(Exposed&, Transformed&&) 
        {
        }
    };
  #endif//BOOST_SPIRIT_X3_EXPERIMENTAL_TRANSFORM_PRE_ITERATOR_RANGE
    
    // handle case where no transformation is required as the types are the same
    template <typename Attribute>
    struct default_transform_attribute<Attribute, Attribute>
    {
        typedef Attribute& type;
        static Attribute& pre(Attribute& val) 
        { 
          #ifdef TRACE_TRANSFORM_ATTRIBUTE_HPP
            std::cout<<"x3::default_transform_attribute::pre(Attribute&)\n";
            std::cout<<"type_name<Attribute>="<<type_name<Attribute>()<<"\n";
          #endif
            return val; 
        }
        static void post(Attribute& val, Attribute const&) 
        {
        }
    };

    // main specialization for x3
    template <typename Exposed, typename Transformed, typename Enable = void>
    struct transform_attribute
      : default_transform_attribute<Exposed, Transformed> {};

    // reference types need special handling
    template <typename Attribute>
    struct transform_attribute<Attribute&, Attribute>
    {
        typedef Attribute& type;
        static Attribute& pre(Attribute& val) 
        { 
          #ifdef TRACE_TRANSFORM_ATTRIBUTE_HPP
            std::cout<<"x3::transform_attribute::pre(Attribute& val)\n";
            std::cout<<"type_name<Attribute>="<<type_name<Attribute>()<<"\n";
          #endif
            return val; 
        }
        static void post(Attribute& val, Attribute const&) 
        {
        }
    };

    // unused_type needs some special handling as well
    template <>
    struct transform_attribute<unused_type, unused_type>
    {
        typedef unused_type type;
        static unused_type pre(unused_type) { return unused; }
        static void post(unused_type, unused_type) {}
    };

    template <>
    struct transform_attribute<unused_type const, unused_type>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<unused_type, Attribute>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<unused_type const, Attribute>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<Attribute, unused_type>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<Attribute const, unused_type>
      : transform_attribute<unused_type, unused_type> {};
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Exposed, typename Transformed>
    struct transform_attribute<Exposed, Transformed, x3::parser_id>
      : x3::transform_attribute<Exposed, Transformed> {};

    template <typename Exposed, typename Transformed>
    struct transform_attribute<Exposed&, Transformed, x3::parser_id>
      : transform_attribute<Exposed, Transformed, x3::parser_id> {};

    template <typename Attribute>
    struct transform_attribute<Attribute&, Attribute, x3::parser_id>
      : x3::transform_attribute<Attribute&, Attribute> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Exposed, typename Transformed>
    void post_transform(Exposed& dest, Transformed&& attr)
    {
      #ifdef TRACE_TRANSFORM_ATTRIBUTE_HPP
        std::cout<<"post_transform(Exposed&, Transformed&&)\n";
        std::cout<<"type_name<Exposed>="<<type_name<Exposed>()<<"\n";
        std::cout<<"type_name<Transformed>="<<type_name<Transformed>()<<"\n";
      #endif
        return transform_attribute<Exposed, Transformed, x3::parser_id>
            ::post(dest, std::forward<Transformed>(attr));
    }
}}}}

#endif
