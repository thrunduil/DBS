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

#include <stdint.h>

namespace dbs_lib { namespace details
{

class dbs_impl;

template<class block_type, int bytes>
struct header;

template<class block_type>
class header<block_type, 4>
{
    public:
        using ushort    = uint16_t;

    public:
        static const int block_bits_log = 5;
        static const int block_bits     = 32;

    private:
        unsigned short      m_level;
        unsigned short      m_size;

    public:
        header()                                :m_level(0), m_size(0){};
        header(ushort lev, ushort size)         : m_level(lev), m_size(size) {};

        block_type&         to_block()          { return *reinterpret_cast<block_type*>(this); };
        const block_type&   to_block() const    { return *reinterpret_cast<const block_type*>(this); };
        ushort              get_level() const   { return m_level; };
        ushort              get_size() const    { return m_size; };

        static size_t       bits_before_pos(size_t bits, size_t pos);
        static size_t       count_bits(size_t bits);
        static size_t       most_significant_bit(size_t bits);
        static size_t       least_significant_bit(size_t bits);
        static size_t       most_significant_bit_pos(size_t bits);
        static size_t       least_significant_bit_pos(size_t bits);
};

template<class block_type>
class header<block_type, 8>
{
    public:
	    using ushort        = uint32_t;

    public:
	    static const int block_bits_log = 6;
	    static const int block_bits     = 64;

    private:
	    ushort              m_level;
	    ushort              m_size;

    public:
	    header()                                :m_level(0), m_size(0){};
	    header(ushort lev, ushort size)         : m_level(lev), m_size(size) {};

	    block_type&         to_block()          { return *reinterpret_cast<block_type*>(this); };
	    const block_type&   to_block() const    { return *reinterpret_cast<const block_type*>(this); };
	    ushort              get_level() const   { return m_level; };
	    ushort              get_size() const    { return m_size; };

	    static size_t       bits_before_pos(size_t bits, size_t pos);
	    static size_t       count_bits(size_t bits);
	    static size_t       most_significant_bit(size_t bits);
	    static size_t       least_significant_bit(size_t bits);
	    static size_t       most_significant_bit_pos(size_t bits);
	    static size_t       least_significant_bit_pos(size_t bits);
};

class dbs_set
{
    private:
        size_t          m_refcount;
        //+variable length array of dbs

    public:
        void            init(size_t pos, const dbs_impl& elem);
        void            init(size_t pos, dbs_impl&& elem);

        const dbs_impl& get_elem(size_t pos) const;        
        void            increase_refcount();
        bool            decrease_refcount();
        void            destroy(size_t elems);
        static dbs_set* create(size_t elems);

    private:
        dbs_impl*       get_elem_ptr();
        const dbs_impl* get_elem_ptr() const;
};

class block
{
    public:
        using header_type       = header<size_t, sizeof(size_t)>;
        using ushort_type       = header_type::ushort;

        static const int block_bits_log = header_type::block_bits_log;
        static const int block_bits     = header_type::block_bits;

    public:
        header_type     m_header;
        size_t          m_flags;
        dbs_set*        m_ptrs;

    public:
        block();
        block(header_type h, size_t f, dbs_set* ptr);
        block(const block& other);
        block(block&&);
        ~block();

        block&          operator=(const block&);
        block&          operator=(block&&);

        dbs_set*        get_fsb_set() const         { return m_ptrs;};
        ushort_type     get_level() const           { return m_header.get_level(); };
        size_t&         get_block_0()               { return m_flags; };
        size_t          get_block_0() const         { return m_flags; };
        size_t&         get_block_1()               { return *reinterpret_cast<size_t*>(&m_ptrs); };
        size_t          get_block_1() const         { return *reinterpret_cast<const size_t*>(&m_ptrs); };
        size_t&         get_block(size_t bl)        { return (&m_flags)[bl]; };
        size_t          get_block(size_t bl) const  { return (&m_flags)[bl]; };

        static size_t	bit_mask(size_t n)          { return size_t(1) << n; };

        static size_t   bits_before_pos(size_t bits, size_t pos);
        static size_t   count_bits(size_t bits);
        static size_t   mod_pow2(size_t a, size_t bits);
        static size_t   div_pow2(size_t a, size_t bits);

        template<size_t bits>
        static size_t   mod_pow2(size_t a);

        template<size_t bits>
        static size_t   div_pow2(size_t a);

    private:
        void            increase_refcount() const;
        void            decrease_refcount() const;
};

};};