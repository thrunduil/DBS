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

#include <vector>

namespace dbs_lib { namespace details
{

class dbs_impl
{
    public:
        using ushort_type               = details::block::ushort_type;
        using block_type                = details::block;
        static const int block_bits_log = details::block::block_bits_log;
        static const int block_bits     = details::block::block_bits;

        static const size_t npos        = size_t(-1);

    private:
        details::block      m_data;

    public:
        // create empty bitset
        dbs_impl();

        // create bitset with one element elem
        explicit dbs_impl(size_t elem);

        // create bitset with count elements stored in the array elems
        // values in the elems array must be different and sorted increasingly
        dbs_impl(size_t count, const size_t* elems);
        
        // standard copy and move constructors
        dbs_impl(const dbs_impl& copy);
        dbs_impl(dbs_impl&& copy);
        
        // destructor
        ~dbs_impl();

        // standard assignment and move assignment 
        dbs_impl&	        operator=(const dbs_impl& copy);
        dbs_impl&	        operator=(dbs_impl&& copy);

    public:
        // return number of elements stored in the bitset
        size_t              size() const;

        // return true if the bitset stores at least one element;
        // equivalent to size() > 0, but faster
        bool				any() const;

        // return true if the bitset is empty;
        // equivalent to size() == 0, but faster
        bool				none() const;
        
        dbs_impl		    set(size_t pos) const;
        dbs_impl		    reset(size_t pos) const;
        dbs_impl		    flip(size_t pos) const;
        bool				test(size_t n) const;
        
        size_t              first() const;
        size_t              last() const;        

        size_t              hash_value_impl() const;
        void                get_elements(std::vector<size_t>& elems) const;

    public:
        block_type&         get_data();
        const block_type&   get_data() const;

        dbs_impl(details::block::header_type h, size_t f, details::dbs_set* ptr);

    private:                
        dbs_impl            increase_level(size_t pos) const;
        dbs_impl            insert_block(size_t this_level_coord, size_t prev_level_coord) const;

        dbs_impl            set(size_t pos, bool& changed) const;
        dbs_impl            set_elem(size_t this_level_coord, size_t prev_level_coord, bool& changed) const;
        dbs_impl            reset(size_t pos, bool& changed) const;
        dbs_impl            reset_elem(size_t this_level_coord, size_t prev_level_coord, bool & changed) const;
        dbs_impl            flip_elem(size_t this_level_coord, size_t prev_level_coord) const;

        static ushort_type  get_level(size_t max_elem);
        static dbs_impl     build_dbs(size_t count, const size_t* elems, size_t offset_bits);
        static dbs_impl     build_dbs(size_t count, const size_t* elems);
        static dbs_impl     build_level(ushort_type level, size_t count, const size_t* elems);

        void                get_elements(size_t offset, std::vector<size_t>& elems) const;        

        friend class details::block;
};

// initializer of dbs library
struct dbs_initializer 
{
    dbs_initializer();
    ~dbs_initializer();
};

// static initializer for every translation unit
static dbs_initializer init;

}};