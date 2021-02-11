/*
*  This file is a part of DBS library.
*
*  Copyright (c) Pawe³ Kowal 2017 - 2021
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#pragma once

#include "dbs/config.h"
#include "dbs/details/dbs_details.h"
#include "dbs/details/dbs_impl.h"

#include <vector>
#include <iosfwd>

namespace dbs_lib
{

// The dbs class represents a set of bits and can store as many elements
// as number of values represented by size_t type. This class is intended
// to offer similar functionality to std::bitset or boost::dynamic_bitset
// but without any restriction on number of bits that can be stored. 
// Internally dbs is implemented as tree like structure, which allows for
// representing sparse bitsets with low memory overhead.
//
// Bitset once created cannot be modified later, any operation on a bitset
// creates an independent copy. However only modified blocks are copied.
class dbs : public details::dbs_impl
{
    public:
        // maximum value for size_t type
        static const size_t npos        = details::dbs_impl::npos;

    public:
        // create empty bitset
        dbs();

        // create bitset with one element elem
        explicit dbs(size_t elem);

        // create bitset with count elements stored in the array elems
        // values in the elems array must be different and sorted increasingly
        dbs(size_t count, const size_t* elems);

        // create bitset with count elements stored in the array elems
        // values in the elems array must be different and sorted increasingly
        dbs(std::initializer_list<size_t> elems);

        // standard copy and move constructors
        dbs(const dbs& copy);
        dbs(dbs&& copy);
        
        // destructor
        ~dbs();

        // standard assignment and move assignment 
        dbs&	            operator=(const dbs& copy);
        dbs&	            operator=(dbs&& copy);

    public:
        // return number of elements stored in the bitset
        size_t              size() const;

        // return true if the bitset stores at least one element;
        // equivalent to size() > 0, but faster
        bool				any() const;

        // return true if the bitset is empty;
        // equivalent to size() == 0, but faster
        bool				none() const;
        
        // construct a new bitset containing bit n
        dbs		            set(size_t n) const;

        // construct a new bitset not containing bit n
        dbs		            reset(size_t n) const;

        // construct a new bitset with bit n flipped
        dbs		            flip(size_t n) const;

        // return true if bit n is set and false is bit n is 0
        bool				test(size_t n) const;
        
        // return lowest index n, such that bit n is set or npos
        // if this set is empty
        size_t              first() const;

        // return highest index n, such that bit n is set or npos
        // if this set is empty
        size_t              last() const;               

        // return true if this bitset contains at least one bit stored
        // in other bitset
        bool                test_any(const dbs& other) const;

        // return true if this bitset contains all bits stored in other
        // bitset
        bool                test_all(const dbs& other) const;

        // append indices of stored bits in this bitset to the vector elems
        void                get_elements(std::vector<size_t>& elems) const;

    public:
        // internal use only
        explicit dbs(const details::dbs_impl& impl);
        explicit dbs(details::dbs_impl&& impl);
};

// return a new bitset that is the bitwise-AND of the bitsets x and y
dbs         operator&(const dbs& x, const dbs& y);

// return a new bitset that is the bitwise-OR of the bitsets x and y
dbs         operator|(const dbs& x, const dbs& y);

// return a new bitset that is the bitwise-XOR of the bitsets x and y
dbs         operator^(const dbs& x, const dbs& y);

// calculate hash function of a bitset x
size_t      hash_value(const dbs& x);

// return true if two bitsets contain the same elements
bool        operator==(const dbs& x, const dbs& y);

// return true if two bitsets are different in at least one bit
bool        operator!=(const dbs& x, const dbs& y);

// return true if bitset x is lexicographically less than y
bool        operator<(const dbs& x, const dbs& y);

// return true if bitset x is lexicographically less than y or
// equal to y
bool        operator<=(const dbs& x, const dbs& y);

// return true if bitset x is lexicographically greater than y
// or equal to y
bool        operator>(const dbs& x, const dbs& y);

// return true if bitset x is lexicographically greater than y
bool        operator>=(const dbs& x, const dbs& y);

// result of compare function
enum class order_type
{
    // first bitset is lexicographically less than the second
    less, 

    // two bitsets are equal
    equal, 

    // first bitset is lexicographically greater than the second
    greater
};

// compare two bitsets
order_type  compare(const dbs& x, const dbs& y);

// print content of a bitset
std::ostream&   operator<<(std::ostream& os, const dbs& x);

}