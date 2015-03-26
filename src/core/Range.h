/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @author Mark Rabkin (mrabkin@fb.com)
// @author Andrei Alexandrescu (andrei.alexandrescu@fb.com)

#pragma once

#include <cassert>
#include <cstring>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <algorithm>
#include "Preprocessor.h"
#include "Logging.h"

template <class T> class Range;


/**
 * Small internal helper - returns the value just before an iterator.
 */
namespace detail {

/**
 * For random-access iterators, the value before is simply i[-1].
 */
template <class Iter>
typename std::enable_if<
  std::is_same<typename std::iterator_traits<Iter>::iterator_category,
               std::random_access_iterator_tag>::value,
  typename std::iterator_traits<Iter>::reference>::type
value_before(Iter i)
{
    return i[-1];
}

/**
 * For all other iterators, we need to use the decrement operator.
 */
template <class Iter>
typename std::enable_if<
  !std::is_same<typename std::iterator_traits<Iter>::iterator_category,
                std::random_access_iterator_tag>::value,
  typename std::iterator_traits<Iter>::reference>::type
value_before(Iter i)
{
    return *--i;
}

} // namespace detail

/**
 * Range abstraction keeping a pair of iterators. We couldn't use
 * boost's similar range abstraction because we need an API identical
 * with the former StringPiece class, which is used by a lot of other
 * code. This abstraction does fulfill the needs of boost's
 * range-oriented algorithms though.
 *
 * (Keep memory lifetime in mind when using this class, since it
 * doesn't manage the data it refers to - just like an iterator
 * wouldn't.)
 */
template <class Iter>
class Range
{
public:
    typedef std::size_t size_type;
    typedef Iter iterator;
    typedef Iter const_iterator;
    typedef typename std::remove_reference <
        typename std::iterator_traits<Iter>::reference > ::type
        value_type;
    typedef typename std::iterator_traits<Iter>::reference reference;

    /**
     * For MutableStringPiece and MutableByteRange we define StringPiece
     * and ByteRange as const_range_type (for everything else its just
     * identity). We do that to enable operations such as find with
     * args which are const.
     */
    typedef typename std::conditional<
        std::is_same<Iter, char*>::value
        || std::is_same<Iter, unsigned char*>::value,
        Range<const value_type*>,
        Range<Iter >> ::type const_range_type;

    typedef std::char_traits < typename std::remove_const<value_type>::type >
        traits_type;

    static const size_type npos;

    // Works for all iterators
    Range()
        : b_(), e_()
    {
    }

public:
    // Works for all iterators
    Range(Iter start, Iter end)
        : b_(start), e_(end)
    {
    }

    // Works only for random-access iterators
    Range(Iter start, size_t size)
        : b_(start), e_(start + size)
    {
    }


    // Works only for Range<const char*>
    /* implicit */ Range(Iter str)
        : b_(str), e_(str + strlen(str))
    {
    }

    // Works only for Range<const char*>
    /* implicit */ Range(const std::string& str)
        : b_(str.data()), e_(b_ + str.size())
    {
    }

    // Works only for Range<const char*>
    Range(const std::string& str, std::string::size_type startFrom)
    {
        if (UNLIKELY(startFrom > str.size())) 
        {
            throw std::out_of_range("index out of range");
        }
        b_ = str.data() + startFrom;
        e_ = str.data() + str.size();
    }
    // Works only for Range<const char*>
    Range(const std::string& str,
        std::string::size_type startFrom,
        std::string::size_type size)
    {
        if (UNLIKELY(startFrom > str.size())) 
        {
            throw std::out_of_range("index out of range");
        }
        b_ = str.data() + startFrom;
        if (str.size() - startFrom < size)
        {
            e_ = str.data() + str.size();
        }
        else
        {
            e_ = b_ + size;
        }
    }
    Range(const Range<Iter>& str,
        size_t startFrom,
        size_t size)
    {
        if (UNLIKELY(startFrom > str.size())) 
        {
            throw std::out_of_range("index out of range");
        }
        b_ = str.b_ + startFrom;
        if (str.size() - startFrom < size)
        {
            e_ = str.e_;
        }
        else {
            e_ = b_ + size;
        }
    }

    // Allow implicit conversion from Range<const char*> (aka StringPiece) to
    // Range<const unsigned char*> (aka ByteRange), as they're both frequently
    // used to represent ranges of bytes.  Allow explicit conversion in the other
    // direction.
    template <class OtherIter, typename std::enable_if<
        (std::is_same<Iter, const unsigned char*>::value &&
        (std::is_same<OtherIter, const char*>::value ||
        std::is_same<OtherIter, char*>::value)), int>::type = 0>
        /* implicit */ Range(const Range<OtherIter>& other)
        : b_(reinterpret_cast<const unsigned char*>(other.begin())),
          e_(reinterpret_cast<const unsigned char*>(other.end()))
    {
    }

    template <class OtherIter, typename std::enable_if<
        (std::is_same<Iter, unsigned char*>::value &&
        std::is_same<OtherIter, char*>::value), int>::type = 0>
        /* implicit */ Range(const Range<OtherIter>& other)
        : b_(reinterpret_cast<unsigned char*>(other.begin())),
          e_(reinterpret_cast<unsigned char*>(other.end()))
    {
    }

    template <class OtherIter, typename std::enable_if<
        (std::is_same<Iter, const char*>::value &&
        (std::is_same<OtherIter, const unsigned char*>::value ||
        std::is_same<OtherIter, unsigned char*>::value)), int>::type = 0>
        explicit Range(const Range<OtherIter>& other)
        : b_(reinterpret_cast<const char*>(other.begin())),
          e_(reinterpret_cast<const char*>(other.end()))
    {
    }

    template <class OtherIter, typename std::enable_if<
        (std::is_same<Iter, char*>::value &&
        std::is_same<OtherIter, unsigned char*>::value), int>::type = 0>
        explicit Range(const Range<OtherIter>& other)
        : b_(reinterpret_cast<char*>(other.begin())),
          e_(reinterpret_cast<char*>(other.end()))
    {
    }

    // Allow implicit conversion from Range<From> to Range<To> if From is
    // implicitly convertible to To.
    template <class OtherIter, typename std::enable_if<
        (!std::is_same<Iter, OtherIter>::value &&
        std::is_convertible<OtherIter, Iter>::value), int>::type = 0>
        /* implicit */ Range(const Range<OtherIter>& other)
        : b_(other.begin()),
          e_(other.end())
    {
    }

    // Allow explicit conversion from Range<From> to Range<To> if From is
    // explicitly convertible to To.
    template <class OtherIter, typename std::enable_if<
        (!std::is_same<Iter, OtherIter>::value &&
        !std::is_convertible<OtherIter, Iter>::value &&
        std::is_constructible<Iter, const OtherIter&>::value), int>::type = 0>
        explicit Range(const Range<OtherIter>& other)
        : b_(other.begin()),
          e_(other.end())
    {
    }

    void clear()
    {
        b_ = Iter();
        e_ = Iter();
    }

    void assign(Iter start, Iter end)
    {
        b_ = start;
        e_ = end;
    }

    void reset(Iter start, size_type size)
    {
        b_ = start;
        e_ = start + size;
    }

    // Works only for Range<const char*>
    void reset(const std::string& str)
    {
        reset(str.data(), str.size());
    }

    size_type size() const
    {
        assert(b_ <= e_);
        return e_ - b_;
    }
    size_type walk_size() const
    {
        assert(b_ <= e_);
        return std::distance(b_, e_);
    }

    bool empty() const { return b_ == e_; }
    Iter data() const { return b_; }
    Iter start() const { return b_; }
    Iter begin() const { return b_; }
    Iter end() const { return e_; }
    Iter cbegin() const { return b_; }
    Iter cend() const { return e_; }
    value_type& front()
    {
        assert(b_ < e_);
        return *b_;
    }
    value_type& back()
    {
        assert(b_ < e_);
        return detail::value_before(e_);
    }
    const value_type& front() const
    {
        assert(b_ < e_);
        return *b_;
    }
    const value_type& back() const
    {
        assert(b_ < e_);
        return detail::value_before(e_);
    }
    // Works only for Range<const char*>
    std::string str() const { return std::string(b_, size()); }
    std::string toString() const { return str(); }

    const_range_type castToConst() const 
    {
        return const_range_type(*this);
    };

    // Works only for Range<const char*>
    int compare(const const_range_type& o) const
    {
        const size_type tsize = this->size();
        const size_type osize = o.size();
        const size_type msize = std::min(tsize, osize);
        int r = traits_type::compare(data(), o.data(), msize);
        if (r == 0) r = static_cast<int>(tsize - osize);
        return r;
    }

    value_type& operator[](size_t i)
    {
        DCHECK_GT(size(), i) << size() << "," << i;
        return b_[i];
    }

    const value_type& operator[](size_t i) const
    {
        DCHECK_GT(size(), i) << size() << "," << i;;
        return b_[i];
    }

    value_type& at(size_t i)
    {
        if (i >= size())
        {
            throw std::out_of_range("index out of range");
        }
        return b_[i];
    }

    const value_type& at(size_t i) const
    {
        if (i >= size())
        {
            throw std::out_of_range("index out of range");
        }
        return b_[i];
    }

    // Works only for Range<const char*>
    uint32_t hash() const
    {
        // Taken from fbi/nstring.h:
        //    Quick and dirty bernstein hash...fine for short ascii strings
        uint32_t hash = 5381;
        for (size_t ix = 0; ix < size(); ix++)
        {
            hash = ((hash << 5) + hash) + b_[ix];
        }
        return hash;
    }

    void advance(size_type n)
    {
        if (UNLIKELY(n > size())) 
        {
            throw std::out_of_range("index out of range");
        }
        b_ += n;
    }

    void subtract(size_type n)
    {
        if (UNLIKELY(n > size())) 
        {
            throw std::out_of_range("index out of range");
        }
        e_ -= n;
    }

    void pop_front()
    {
        assert(b_ < e_);
        ++b_;
    }

    void pop_back()
    {
        assert(b_ < e_);
        --e_;
    }

    Range subpiece(size_type first,
        size_type length = std::string::npos) const
    {
        if (UNLIKELY(first > size())) 
        {
            throw std::out_of_range("index out of range");
        }
        return Range(b_ + first,
            std::min<std::string::size_type>(length, size() - first));
    }

    size_type find(value_type c) const
    {
        auto r = castToConst();
        return std::find(r.begin(), r.end(), c);
    }

    void swap(Range& rhs)
    {
        std::swap(b_, rhs.b_);
        std::swap(e_, rhs.e_);
    }

    /**
     * Does this Range start with another range?
     */
    bool startsWith(const const_range_type& other) const
    {
        return size() >= other.size() 
            && castToConst().subpiece(0, other.size()) == other;
    }
    bool startsWith(value_type c) const
    {
        return !empty() && front() == c;
    }

    /**
     * Does this Range end with another range?
     */
    bool endsWith(const const_range_type& other) const
    {
        return size() >= other.size() 
            && castToConst().subpiece(size() - other.size()) == other;
    }
    bool endsWith(value_type c) const
    {
        return !empty() && back() == c;
    }

    /**
     * Remove the given prefix and return true if the range starts with the given
     * prefix; return false otherwise.
     */
    bool removePrefix(const const_range_type& prefix)
    {
        return startsWith(prefix) && (b_ += prefix.size(), true);
    }
    bool removePrefix(value_type prefix)
    {
        return startsWith(prefix) && (++b_, true);
    }

    /**
     * Remove the given suffix and return true if the range ends with the given
     * suffix; return false otherwise.
     */
    bool removeSuffix(const const_range_type& suffix)
    {
        return endsWith(suffix) && (e_ -= suffix.size(), true);
    }
    bool removeSuffix(value_type suffix)
    {
        return endsWith(suffix) && (--e_, true);
    }


    /**
     * Replaces the content of the range, starting at position 'pos', with
     * contents of 'replacement'. Entire 'replacement' must fit into the
     * range. Returns false if 'replacements' does not fit. Example use:
     *
     * char in[] = "buffer";
     * auto msp = MutablesStringPiece(input);
     * EXPECT_TRUE(msp.replaceAt(2, "tt"));
     * EXPECT_EQ(msp, "butter");
     *
     * // not enough space
     * EXPECT_FALSE(msp.replace(msp.size() - 1, "rr"));
     * EXPECT_EQ(msp, "butter"); // unchanged
     */
    bool replaceAt(size_t pos, const_range_type replacement)
    {
        if (size() < pos + replacement.size())
        {
            return false;
        }
        std::copy(replacement.begin(), replacement.end(), begin() + pos);
        return true;
    }

private:
    Iter    b_;
    Iter    e_;
};

template <class Iter>
const typename Range<Iter>::size_type Range<Iter>::npos = std::string::npos;

template <class T>
void swap(Range<T>& lhs, Range<T>& rhs)
{
    lhs.swap(rhs);
}

/**
 * Create a range from two iterators, with type deduction.
 */
template <class Iter>
Range<Iter> range(Iter first, Iter last)
{
    return Range<Iter>(first, last);
}

/*
 * Creates a range to reference the contents of a contiguous-storage container.
 */
// Use pointers for types with '.data()' member
template <class Collection,
          class T = typename std::remove_pointer<
              decltype(std::declval<Collection>().data())>::type>
Range<T*> range(Collection&& v)
{
    return Range<T*>(v.data(), v.data() + v.size());
}

template <class T, size_t n>
Range<T*> range(T (&array)[n])
{
  return Range<T*>(array, array + n);
}

typedef Range<const char*> StringPiece;
typedef Range<char*> MutableStringPiece;
typedef Range<const unsigned char*> ByteRange;
typedef Range<unsigned char*> MutableByteRange;

std::ostream& operator<<(std::ostream& os, const StringPiece& piece);
std::ostream& operator<<(std::ostream& os, const MutableStringPiece& piece);

/**
 * Templated comparison operators
 */

template <class T>
inline bool operator==(const Range<T>& lhs, const Range<T>& rhs)
{
    return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template <class T>
inline bool operator!=(const Range<T>& lhs, const Range<T>& rhs)
{
    return !(lhs == rhs);
}

template <class T>
inline bool operator<(const Range<T>& lhs, const Range<T>& rhs)
{
    return lhs.compare(rhs) < 0;
}

template <class T>
inline bool operator>(const Range<T>& lhs, const Range<T>& rhs)
{
    return lhs.compare(rhs) > 0;
}

template <class T>
inline bool operator>=(const Range<T>& lhs, const Range<T>& rhs)
{
    return !(lhs < rhs);
}

template <class T>
inline bool operator<=(const Range<T>& lhs, const Range<T>& rhs)
{
    return !(lhs > rhs);
}

/**
 * Specializations of comparison operators for StringPiece
 */

namespace detail {

template <class A, class B>
struct ComparableAsStringPiece
{
    enum
    {
        value =
        (std::is_convertible<A, StringPiece>::value
        && std::is_same<B, StringPiece>::value)
        ||
        (std::is_convertible<B, StringPiece>::value
        && std::is_same<A, StringPiece>::value)
    };
};

} // namespace detail

/**
 * operator== through conversion for Range<const char*>
 */
template <class T, class U>
typename
std::enable_if<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator==(const T& lhs, const U& rhs)
{
    return StringPiece(lhs) == StringPiece(rhs);
}

template <class T, class U>
typename
std::enable_if<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator!=(const T& lhs, const U& rhs)
{
    return StringPiece(lhs) != StringPiece(rhs);
}

/**
 * operator< through conversion for Range<const char*>
 */
template <class T, class U>
typename
std::enable_if<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator<(const T& lhs, const U& rhs)
{
    return StringPiece(lhs) < StringPiece(rhs);
}

/**
 * operator> through conversion for Range<const char*>
 */
template <class T, class U>
typename
std::enable_if<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator>(const T& lhs, const U& rhs)
{
    return StringPiece(lhs) > StringPiece(rhs);
}

/**
 * operator< through conversion for Range<const char*>
 */
template <class T, class U>
typename
std::enable_if<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator<=(const T& lhs, const U& rhs)
{
    return StringPiece(lhs) <= StringPiece(rhs);
}

/**
 * operator> through conversion for Range<const char*>
 */
template <class T, class U>
typename
std::enable_if<detail::ComparableAsStringPiece<T, U>::value, bool>::type
operator>=(const T& lhs, const U& rhs)
{
    return StringPiece(lhs) >= StringPiece(rhs);
}

struct StringPieceHash
{
    std::size_t operator()(const StringPiece& str) const
    {
        return static_cast<std::size_t>(str.hash());
    }
};

