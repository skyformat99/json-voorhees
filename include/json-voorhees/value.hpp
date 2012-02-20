/** \file json-voorhees/value.hpp
 *  
 *  Copyright (c) 2012 by Travis Gockel. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify it under the terms of the Apache License
 *  as published by the Apache Software Foundation, either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  \author Travis Gockel (travis@gockelhut.com)
**/
#ifndef __JSON_VOORHEES_VALUE_HPP_INCLUDED__
#define __JSON_VOORHEES_VALUE_HPP_INCLUDED__

#include "standard.hpp"

#include <cstdint>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace jsonv
{

namespace detail
{

class object_impl;
class array_impl;
class string_impl;

union value_storage
{
    object_impl* object;
    array_impl*  array;
    string_impl* string;
    int64_t      integer;
    double       decimal;
    bool         boolean;
};

}

enum class kind
{
    null,
    object,
    array,
    string,
    integer,
    decimal,
    boolean
};

class kind_error :
        public std::logic_error
{
public:
    explicit kind_error(const std::string& description);
    
    virtual ~kind_error() throw();
};

class object;
class array_view;

/** \see http://json.org/
**/
class value
{
public:
    /// Default-construct this to null.
    value();
    value(const value& source);
    value(const string_type& val);
    value(const char_type* val);
    value(int64_t val);
    value(double val);
    value(bool val);
    value(const object& obj);
    value(const array_view& array_view);
    
    #define JSONV_VALUE_INTEGER_ALTERNATIVE_CTOR_PROTO_GENERATOR(type_)              \
        value(type_ val);
    JSON_VOORHEES_INTEGER_ALTERNATES_LIST(JSONV_VALUE_INTEGER_ALTERNATIVE_CTOR_PROTO_GENERATOR)
    
    // Don't allow any implicit conversions that we have not specified.
    template <typename T>
    value(T) = delete;
    
    ~value();
    
    /** Copy-assigns \c source to this.
     *  
     *  If an exception is thrown during the copy, it is propagated out. This instance will be set to a null value - any
     *  value will be disposed.
    **/
    value& operator=(const value& source);
    
    value(value&& source);
    
    /** Move-assigns \c source to this.
     *  
     *  Unlike a copy, this will never throw.
    **/
    value& operator=(value&& source);
    
    static object make_object();
    static value make_array();
    
    /// Resets this value to null.
    void clear();
    
    inline kind get_kind() const
    {
        return _kind;
    }
    
    object& as_object();
    const object& as_object() const;
    
    array_view as_array();
    const array_view as_array() const;
    
    string_type& as_string();
    const string_type& as_string() const;
    
    int64_t& as_integer();
    int64_t  as_integer() const;
    
    double& as_decimal();
    double  as_decimal() const;
    
    bool& as_boolean();
    bool  as_boolean() const;
    
    /** Compares two JSON values for equality. Two JSON values are equal if and only if all of the following conditions
     *  apply:
     *  
     *   1. They have the same valid value for \c type.
     *      - If \c type is invalid (memory corruption), then two JSON values are \e not equal, even if they have been
     *        corrupted in the same way and even if they share \c this (a corrupt object is not equal to itself).
     *   2. The type comparison is also equal:
     *      a. Two null values are always equivalent.
     *      b. string, integer, decimal and boolean follow the classic rules for their type.
     *      c. objects are equal if they have the same keys and values corresponding with the same key are also equal.
     *      d. arrays are equal if they have the same length and the values at each index are also equal.
     *  
     *  \note
     *  The rules for equality are based on Python \c dict.
    **/
    bool operator ==(const value& other) const;
    
    /** Compares two JSON values for inequality. The rules for inequality are the exact opposite of equality.
    **/
    bool operator !=(const value& other) const;
    
    // FUTURE: I guess do it just like Python?
    // bool operator <(const value& other) const;
    
    friend ostream_type& operator <<(ostream_type& stream, const value& val);
    
protected:
    detail::value_storage _data;
    kind                  _kind;
};

/** This is a view of a \c value as a JSON array (with the same lack of lifetime management as \c object_view). It is a
 *  sequence container which behaves a lot like an \c std::deque (probably because that is the type that backs it).
**/
class array_view
{
public:
    typedef size_t       size_type;
    typedef value        value_type;
    typedef value&       reference;
    typedef const value& const_reference;
    typedef value*       pointer;
    typedef const value* const_pointer;
    
    template <typename T, typename TArrayView>
    struct basic_iterator :
            public std::iterator<std::random_access_iterator_tag, T>
    {
    public:
        basic_iterator() :
                _owner(0),
                _index(0)
        { }
        
        basic_iterator(TArrayView* owner, size_type index) :
                _owner(owner),
                _index(index)
        { }
        
        template <typename U, typename UArrayView>
        basic_iterator(const basic_iterator<U, UArrayView>& source,
                       typename std::enable_if<std::is_convertible<U*, T*>::value>::type* = 0
                      ) :
                _owner(source._owner),
                _index(source._index)
        { }
        
        basic_iterator& operator ++()
        {
            ++_index;
            return *this;
        }
        
        basic_iterator operator ++(int) const
        {
            basic_iterator clone = *this;
            ++clone;
            return clone;
        }
        
        basic_iterator& operator --()
        {
            --_index;
            return *this;
        }
        
        basic_iterator operator --(int) const
        {
            basic_iterator clone = *this;
            --clone;
            return clone;
        }
        
        template <typename U, typename UArrayView>
        bool operator ==(const basic_iterator<U, UArrayView>& other) const
        {
            return _owner == other._owner && _index == other._index;
        }
        
        template <typename U, typename UArrayView>
        bool operator !=(const basic_iterator<U, UArrayView>& other) const
        {
            return !operator==(other);
        }
        
        T& operator *() const
        {
            return _owner->operator[](_index);
        }
        
        T* operator ->() const
        {
            return &_owner->operator[](_index);
        }
        
        basic_iterator& operator +=(size_type n)
        {
            _index += n;
            return *this;
        }
        
        basic_iterator operator +(size_type n) const
        {
            basic_iterator clone = *this;
            clone += n;
            return clone;
        }
        
        basic_iterator& operator -=(size_type n)
        {
            _index -= n;
            return *this;
        }
        
        basic_iterator operator -(size_type n) const
        {
            basic_iterator clone = *this;
            clone -= n;
            return clone;
        }
        
        bool operator <(const basic_iterator& rhs) const
        {
            return _index < rhs._index;
        }
        
        bool operator <=(const basic_iterator& rhs) const
        {
            return _index <= rhs._index;
        }
        
        bool operator >(const basic_iterator& rhs) const
        {
            return _index > rhs._index;
        }
        
        bool operator >=(const basic_iterator& rhs) const
        {
            return _index >= rhs._index;
        }
        
        T& operator[](size_type n) const
        {
            return _owner->operator[](_index + n);
        }
    private:
        template <typename U, typename UArrayView>
        friend struct basic_iterator;
        
    private:
        TArrayView* _owner;
        size_type   _index;
    };
    
    typedef basic_iterator<value, array_view>             iterator;
    typedef basic_iterator<const value, const array_view> const_iterator;
    
public:
    size_type size() const;
    bool empty() const;
    
    iterator begin();
    const_iterator begin() const;
    
    iterator end();
    const_iterator end() const;
    
    value& operator[](size_type idx);
    const value& operator[](size_type idx) const;
    
    void push_back(const value& value);
    void pop_back();
    
    void push_front(const value& value);
    void pop_front();
    
    void clear();
    void resize(size_type count, const value& val = value());
    
    /// \see value::operator==
    bool operator==(const array_view& other) const;
    
    /// \see value::operator!=
    bool operator!=(const array_view& other) const;
    
    void push_back_many()
    { }
    
    template <typename T, typename... TRest>
    void push_back_many(T&& val, TRest&&... rest)
    {
        push_back(std::forward<T>(val));
        push_back_many(std::forward<TRest>(rest)...);
    }
    
    friend ostream_type& operator <<(ostream_type& stream, const array_view& view);
    
private:
    friend class value;
    explicit array_view(detail::array_impl* source);
    
private:
    detail::array_impl* _source;
};

/** Creates an array with the given \a values.
 *  
 *  \example
 *  \code
 *  jsonv::value val = jsonv::make_array(1, 2, 3, "foo");
 *  // Creates: [1, 2, 3, "foo"]
 *  \endcode
**/
template <typename... T>
value make_array(T&&... values)
{
    value val = value::make_array();
    val.as_array().push_back_many(std::forward<T>(values)...);
    return val;
}

}

#endif/*__JSON_VOORHEES_VALUE_HPP_INCLUDED__*/
