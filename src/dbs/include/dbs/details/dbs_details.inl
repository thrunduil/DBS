/* 
 *  This file is a part of DBS library.
 *
 *  Copyright (c) Pawe³ Kowal 2017
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

#include "dbs_details.h"

namespace dbs_lib { namespace details
{

//-----------------------------------------------------------------
//                      header
//-----------------------------------------------------------------
template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 4>::bits_before_pos(size_t bits, size_t pos)
{
    size_t mask     = ~(0xffffffff << pos);
    size_t sel      = bits & mask;
    return sel;
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 4>::count_bits(size_t bits)
{
    #ifdef DBS_HAS_POPCNT
        return _mm_popcnt_u32(bits);
    #else
        //taken from The Aggregate Magic Algorithms
        bits = bits - ((bits >> size_t(1)) & 0x55555555);
        bits = (bits & 0x33333333) + ((bits >> 2) & 0x33333333);
        bits = (((bits + (bits >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;

        return bits;
    #endif
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 4>::most_significant_bit(size_t x)
{
    //taken from The Aggregate Magic Algorithms
    x       |= (x >> 1);
    x       |= (x >> 2);
    x       |= (x >> 4);
    x       |= (x >> 8);
    x       |= (x >> 16);

    return(x & ~(x >> 1));
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 4>::most_significant_bit_pos(size_t x)
{
    //taken from The Aggregate Magic Algorithms
    x       |= (x >> 1);
    x       |= (x >> 2);
    x       |= (x >> 4);
    x       |= (x >> 8);
    x       |= (x >> 16);

    return count_bits(x)-1;
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 4>::least_significant_bit_pos(size_t x)
{
    //taken from The Aggregate Magic Algorithms
    x       |= (x << 1);
    x       |= (x << 2);
    x       |= (x << 4);
    x       |= (x << 8);
    x       |= (x << 16);

    return 32 - count_bits(x);
};

#pragma warning(push)
#pragma warning(disable: 4146) // unary minus operator applied to unsigned type, result still unsigned

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 4>::least_significant_bit(size_t bits)
{
    //taken from The Aggregate Magic Algorithms
    return bits & -bits;
};

#pragma warning(pop)
template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 8>::bits_before_pos(size_t bits, size_t pos)
{
	size_t mask = ~(0xffffffffffffffff << pos);
	size_t sel = bits & mask;
	return sel;
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 8>::count_bits(size_t x)
{
    #ifdef DBS_HAS_POPCNT
        return _mm_popcnt_u64(x);
    #else
        // Hamming weight algorithm with fast multiplication
        // http://en.wikipedia.org/wiki/Hamming_weight

	    static const uint64_t m1  = 0x5555555555555555; //binary: 0101...
	    static const uint64_t m2  = 0x3333333333333333; //binary: 00110011..
	    static const uint64_t m4  = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
	    static const uint64_t h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...

	    x -= (x >> 1) & m1;             //put count of each 2 bits into those 2 bits
	    x = (x & m2) + ((x >> 2) & m2); //put count of each 4 bits into those 4 bits 
	    x = (x + (x >> 4)) & m4;        //put count of each 8 bits into those 8 bits 
	    return (x * h01) >> 56;         //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
    #endif
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 8>::most_significant_bit(size_t x)
{
	//taken from The Aggregate Magic Algorithms
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);

	return(x & ~(x >> 1));
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 8>::most_significant_bit_pos(size_t x)
{
	//taken from The Aggregate Magic Algorithms
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);

	return count_bits(x) - 1;
};

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 8>::least_significant_bit_pos(size_t x)
{
	//taken from The Aggregate Magic Algorithms
	x |= (x << 1);
	x |= (x << 2);
	x |= (x << 4);
	x |= (x << 8);
	x |= (x << 16);
	x |= (x << 32);

	return 64 - count_bits(x);
};

#pragma warning(push)
#pragma warning(disable: 4146) // unary minus operator applied to unsigned type, result still unsigned

template<class block_type>
DBS_FORCE_INLINE
size_t header<block_type, 8>::least_significant_bit(size_t bits)
{
	//taken from The Aggregate Magic Algorithms
	return bits & -bits;
};

#pragma warning(pop)

//------------------------------------------------------------
//                      Allocator
//------------------------------------------------------------
class Allocator
{
    public:
        static dbs_set*     create(size_t elems);
        static void         destroy(dbs_set*, size_t elems);
};

//-----------------------------------------------------------------
//                      dbs_set
//-----------------------------------------------------------------
DBS_FORCE_INLINE
void dbs_set::increase_refcount()
{
    ++m_refcount;
};

DBS_FORCE_INLINE
bool dbs_set::decrease_refcount()
{
    return (--m_refcount) == 0;
};

DBS_FORCE_INLINE
void dbs_set::destroy(size_t elems)
{
    for (size_t i = 0; i < elems; ++i)
        get_elem(i).~dbs_impl();

    Allocator::destroy(this,elems);    
};

DBS_FORCE_INLINE
dbs_set* dbs_set::create(size_t elems)
{
    dbs_set* ptr = Allocator::create(elems);
    ptr->m_refcount = 1;
    return ptr;
};

DBS_FORCE_INLINE
void dbs_set::init(size_t pos, const dbs_impl& elem)
{
    new(get_elem_ptr() + pos) dbs_impl(elem);
};

DBS_FORCE_INLINE
void dbs_set::init(size_t pos, dbs_impl&& elem)
{
    new(get_elem_ptr() + pos) dbs_impl(std::move(elem));
};

DBS_FORCE_INLINE
const dbs_impl& dbs_set::get_elem(size_t pos) const
{
    return get_elem_ptr()[pos];
};

DBS_FORCE_INLINE
const dbs_impl* dbs_set::get_elem_ptr() const
{
    const size_t* ptr = &m_refcount + 1;
    return reinterpret_cast<const dbs_impl*>(ptr);
};

DBS_FORCE_INLINE 
dbs_impl* dbs_set::get_elem_ptr()
{
    size_t* ptr = &m_refcount + 1;
    return reinterpret_cast<dbs_impl*>(ptr);
};

//-----------------------------------------------------------------
//                      block
//-----------------------------------------------------------------
DBS_FORCE_INLINE 
size_t block::bits_before_pos(size_t bits, size_t pos)
{ 
    return header_type::bits_before_pos(bits, pos); 
}

DBS_FORCE_INLINE
size_t block::count_bits(size_t bits)
{ 
    return header_type::count_bits(bits); 
}

DBS_FORCE_INLINE
size_t block::mod_pow2(size_t a, size_t bits)
{
    size_t mask = (size_t(1) << bits) - size_t(1);
    return a & mask;
};

DBS_FORCE_INLINE
size_t block::div_pow2(size_t a, size_t bits)
{
    return a >> bits;
};

template<size_t bits>
DBS_FORCE_INLINE
size_t block::mod_pow2(size_t a)
{
    return a & ( (size_t(1) << bits) - size_t(1));
};

template<size_t bits>
DBS_FORCE_INLINE
size_t block::div_pow2(size_t a)
{
    return a >> bits;
};

DBS_FORCE_INLINE
block::block()
    : m_header(), m_flags(0), m_ptrs(nullptr) 
{};

DBS_FORCE_INLINE
block::block(header_type h, size_t f, dbs_set* ptr)
    : m_header(h), m_flags(f), m_ptrs(ptr) 
{};

DBS_FORCE_INLINE
block::block(const block& other)
    : m_header(other.m_header), m_flags(other.m_flags), m_ptrs(other.m_ptrs)
{
    increase_refcount();
};

DBS_FORCE_INLINE
block::block(block&& other)
    : m_header(other.m_header), m_flags(other.m_flags), m_ptrs(other.m_ptrs)
{
    if (other.get_level() > 0)
        other.m_header.to_block() = 0;
};

DBS_FORCE_INLINE
block::~block()
{
    decrease_refcount();
};

DBS_FORCE_INLINE
block& block::operator=(const block& other)
{
    other.increase_refcount();
    this->decrease_refcount();

    m_header    = other.m_header;
    m_flags     = other.m_flags;
    m_ptrs      = other.m_ptrs;

    return *this;
};

DBS_FORCE_INLINE
block& block::operator=(block&& other)
{
    other.increase_refcount();
    this->decrease_refcount();

    m_header    = other.m_header;
    m_flags     = other.m_flags;
    m_ptrs      = other.m_ptrs;

    return *this;
};

DBS_FORCE_INLINE
void block::increase_refcount() const
{
    if (this->get_level() > 0)
        m_ptrs->increase_refcount();
};

DBS_FORCE_INLINE
void block::decrease_refcount() const
{
    if (this->get_level() > 0)
    {
        if (m_ptrs->decrease_refcount() == true)
            m_ptrs->destroy(this->m_header.get_size());
    };
};

}}