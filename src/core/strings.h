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
 
#pragma once

#include <string>
#include <vector>
#include <type_traits>
#include "Platform.h"
#include "Range.h"
#include "Conv.h"


/**
 * stringPrintf is much like printf but deposits its result into a
 * string. Two signatures are supported: the first simply returns the
 * resulting string, and the second appends the produced characters to
 * the specified string and returns a reference to it.
 */
std::string stringPrintf(PRINTF_FORMAT const char* format, ...)
    PRINTF_FORMAT_ATTR(1, 2);

/** Similar to stringPrintf, with different signiture.
 */
void stringPrintf(std::string* out, PRINTF_FORMAT const char* fmt, ...)
    PRINTF_FORMAT_ATTR(2, 3);

std::string& stringAppendf(std::string* output, 
    PRINTF_FORMAT const char* format, ...)
    PRINTF_FORMAT_ATTR(2, 3);


/*
 * Split a string into a list of tokens by delimiter.
 *
 * You can also use splitTo() to write the output to an arbitrary
 * OutputIterator (e.g. std::inserter() on a std::set<>), in which
 * case you have to tell the function the type.  (Rationale:
 * OutputIterators don't have a value_type, so we can't detect the
 * type in splitTo without being told.)
 *
 * Examples:
 *
 *   std::vector<StringPiece> v;
 *   split(":", "asd:bsd", v);
 *
 *   std::set<StringPiece> s;
 *   splitTo<StringPiece>(":", "asd:bsd:asd:csd",
 *   std::inserter(s, s.begin()));
 *
 * Split also takes a flag (ignoreEmpty) that indicates whether adjacent
 * delimiters should be treated as one single separator (ignoring empty tokens)
 * or not (generating empty tokens).
 */
template<class Delim, class String, class OutputType>
void split(const Delim& delimiter,
           const String& input,
           std::vector<OutputType>& out,
           bool ignoreEmpty = false);

template<class OutputValueType, class Delim, class String, class OutputIterator>
void splitTo(const Delim& delimiter,
             const String& input,
             OutputIterator out,
             bool ignoreEmpty = false);


/*
 * Split a string into a fixed number of string pieces and/or numeric types
 * by delimiter. Any numeric type that to<> can convert to from a
 * string piece is supported as a target. Returns 'true' if the fields were
 * all successfully populated.
 *
 * Examples:
 *
 *  StringPiece name, key, value;
 *  if (split('\t', line, name, key, value))
 *    ...
 *
 *  StringPiece name;
 *  double value;
 *  int id;
 *  if (split('\t', line, name, value, id))
 *    ...
 *
 * The 'exact' template parameter specifies how the function behaves when too
 * many fields are present in the input string. When 'exact' is set to its
 * default value of 'true', a call to split will fail if the number of fields in
 * the input string does not exactly match the number of output parameters
 * passed. If 'exact' is overridden to 'false', all remaining fields will be
 * stored, unsplit, in the last field, as shown below:
 *
 *  StringPiece x, y.
 *  if (split<false>(':', "a:b:c", x, y))
 *    assert(x == "a" && y == "b:c");
 *
 * Note that this will likely not work if the last field's target is of numeric
 * type, in which case to<> will throw an exception.
 */

template<bool exact = true,
         class Delim,
         class OutputType,
         class... OutputTypes>
typename std::enable_if<std::is_arithmetic<OutputType>::value
    || std::is_same<OutputType, StringPiece>::value
    || IsSomeString<OutputType>::value,
    bool>::type
split(const Delim& delimiter,
      StringPiece input,
      OutputType& outHead,
      OutputTypes&... outTail);


/*
 * Join list of tokens.
 *
 * Stores a string representation of tokens in the same order with
 * deliminer between each element.
 */

template <class Delim, class Iterator, class String>
void join(const Delim& delimiter,
          Iterator begin,
          Iterator end,
          String& output);

template <class Delim, class Container, class String>
void join(const Delim& delimiter,
          const Container& container,
          String& output) 
{
    join(delimiter, container.begin(), container.end(), output);
}

template <class Delim, class Value, class String>
void join(const Delim& delimiter,
          const std::initializer_list<Value>& values,
          String& output) 
{
    join(delimiter, values.begin(), values.end(), output);
}

template <class Delim, class Container>
std::string join(const Delim& delimiter,
                 const Container& container) 
{
    std::string output;
    join(delimiter, container.begin(), container.end(), output);
    return output;
}

template <class Delim, class Value>
std::string join(const Delim& delimiter,
                 const std::initializer_list<Value>& values) 
{
    std::string output;
    join(delimiter, values.begin(), values.end(), output);
    return output;
}

/**
 * Returns a subpiece with all whitespace removed from the front of @sp.
 * Whitespace means any of [' ', '\n', '\r', '\t'].
 */
StringPiece skipWhitespace(StringPiece sp);

/*
 * A pretty-printer for numbers that appends suffixes of units of the
 * given type.  It prints 4 sig-figs of value with the most
 * appropriate unit.
 *
 * If `addSpace' is true, we put a space between the units suffix and
 * the value.
 *
 * Current types are:
 *   PRETTY_TIME         - s, ms, us, ns, etc.
 *   PRETTY_BYTES_METRIC - kB, MB, GB, etc (goes up by 10^3 = 1000 each time)
 *   PRETTY_BYTES        - kB, MB, GB, etc (goes up by 2^10 = 1024 each time)
 *   PRETTY_BYTES_IEC    - KiB, MiB, GiB, etc
 *   PRETTY_UNITS_METRIC - k, M, G, etc (goes up by 10^3 = 1000 each time)
 *   PRETTY_UNITS_BINARY - k, M, G, etc (goes up by 2^10 = 1024 each time)
 *   PRETTY_UNITS_BINARY_IEC - Ki, Mi, Gi, etc
 *   PRETTY_SI           - full SI metric prefixes from yocto to Yotta
 *                         http://en.wikipedia.org/wiki/Metric_prefix
 * @author Mark Rabkin <mrabkin@fb.com>
 */
enum PrettyType 
{
    PRETTY_TIME,

    PRETTY_BYTES_METRIC,
    PRETTY_BYTES_BINARY,
    PRETTY_BYTES = PRETTY_BYTES_BINARY,
    PRETTY_BYTES_BINARY_IEC,
    PRETTY_BYTES_IEC = PRETTY_BYTES_BINARY_IEC,

    PRETTY_UNITS_METRIC,
    PRETTY_UNITS_BINARY,
    PRETTY_UNITS_BINARY_IEC,

    PRETTY_SI,
    PRETTY_NUM_TYPES,
};

std::string prettyPrint(double val, PrettyType, bool addSpace = true);

/**
 * This utility converts StringPiece in pretty format (look above) to double,
 * with progress information. Alters the  StringPiece parameter
 * to get rid of the already-parsed characters.
 * Expects string in form <floating point number> {space}* [<suffix>]
 * If string is not in correct format, utility finds longest valid prefix and
 * if there at least one, returns double value based on that prefix and
 * modifies string to what is left after parsing. Throws and std::range_error
 * exception if there is no correct parse.
 * Examples(for PRETTY_UNITS_METRIC):
 * '10M' => 10 000 000
 * '10 M' => 10 000 000
 * '10' => 10
 * '10 Mx' => 10 000 000, prettyString == "x"
 * 'abc' => throws std::range_error
 */
double prettyToDouble(StringPiece *const prettyString,
                      const PrettyType type);

/*
 * Same as prettyToDouble(StringPiece*, PrettyType), but
 * expects whole string to be correctly parseable. Throws std::range_error
 * otherwise
 */
double prettyToDouble(StringPiece prettyString, const PrettyType type);

#include "Strings-inl.h"
