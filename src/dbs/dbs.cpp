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

#include "dbs/dbs.h"
#include "dbs/details/dbs_impl.h"
#include "dbs/details/dbs_details.inl"

#include <boost/pool/pool.hpp>
#include <boost/functional/hash.hpp>

namespace dbs_lib { namespace details
{

//------------------------------------------------------------
//                      Allocator
//------------------------------------------------------------
struct dbs_tag{};

template<class tag>
struct symbolic_allocator
{
    using size_type         = std::size_t;
    using difference_type   = std::ptrdiff_t;

    static void report_bad_alloc()
    {
        throw std::bad_alloc();
    }

    static char* malloc(const size_type bytes)
    { 
        size_t* ptr = reinterpret_cast<size_t*>(::malloc(bytes));

        if (!ptr)
            report_bad_alloc();            

        return reinterpret_cast<char*>(ptr);
    }
    static void free(void* block)
    { 
        size_t* ptr     = reinterpret_cast<size_t*>(block);        
        ::free(ptr);
    }
};

template<class value_type>
struct pod_type
{    
    char m_data[sizeof(value_type)];
};

struct allocator_pools
{
    using allocator_type    = details::symbolic_allocator<dbs_tag>;
    using pool_type         = boost::pool<allocator_type>;

    pool_type*  m_pools;
    size_t*     m_elems;

    allocator_pools();
    ~allocator_pools();
};

allocator_pools::allocator_pools()
{
    static const int block_bits = details::block::block_bits;

    void* pool_ptr  = allocator_type::malloc((block_bits+1) * sizeof(pool_type));
    void* elem_ptr  = allocator_type::malloc((block_bits+1)*sizeof(size_t));

    m_pools         = reinterpret_cast<pool_type*>(pool_ptr);
    m_elems         = reinterpret_cast<size_t*>(elem_ptr);

    for (size_t i = 1; i <= block_bits; ++i)
    {
        new(m_pools + i) pool_type(i * sizeof(dbs) + sizeof(details::dbs_set));
        m_elems[i]  = 0;
    };
};

allocator_pools::~allocator_pools()
{
    static const int block_bits = details::block::block_bits;

    for (size_t i = 1; i <= block_bits; ++i)
    {
        assert(m_elems[i] == 0);
        m_pools[i].purge_memory();
    };

    allocator_type::free(m_pools);
    allocator_type::free(m_elems);
};

//------------------------------------------------------------
//                      dbs_initializer
//------------------------------------------------------------
static allocator_pools* apools = nullptr;

// nifty counter
static int g_counter = 0;

dbs_initializer::dbs_initializer()
{
    if (g_counter == 0)
        apools = new allocator_pools();

    ++g_counter;
};

dbs_initializer::~dbs_initializer()
{
    --g_counter;

    if (g_counter == 0)   
    {
        delete apools;
        apools = nullptr;
    };
}

details::dbs_set* details::Allocator::create(size_t elems)
{
    ++apools->m_elems[elems];
    return static_cast<details::dbs_set*>(apools->m_pools[elems].malloc());
};

void details::Allocator::destroy(dbs_set* ptr, size_t elems)
{
    --apools->m_elems[elems];
    apools->m_pools[elems].free(ptr);
};

//------------------------------------------------------------
//                      dbs_impl
//------------------------------------------------------------
dbs_impl::dbs_impl()
{};

dbs_impl::dbs_impl(details::block::header_type h, size_t f, details::dbs_set* ptr)
    :m_data(h,f,ptr)
{};

dbs_impl::dbs_impl(const dbs_impl& copy)
    :m_data(copy.m_data)
{};

dbs_impl::dbs_impl(dbs_impl&& copy)
    :m_data(std::move(copy.m_data))
{};

dbs_impl::~dbs_impl()
{}

dbs_impl& dbs_impl::operator=(const dbs_impl& copy)
{
    this->m_data = copy.m_data;
    return *this;
};

dbs_impl& dbs_impl::operator=(dbs_impl&& copy)
{
    this->m_data = std::move(copy.m_data);
    return *this;
}

dbs_impl::dbs_impl(size_t elem)
{
    using block             = details::block;
    ushort_type level       = get_level(elem);
    size_t capacity_bits    = block_bits_log*level + 1;

    if (level == 0)
    {
        size_t pos              = block::div_pow2<1>(elem);
        size_t block            = block::mod_pow2<1>(elem);
        m_data.get_block(block) = block::bit_mask(pos);

        return;
    };
    
    size_t this_code        = block::div_pow2(elem, capacity_bits);
    size_t prev_code        = block::mod_pow2(elem, capacity_bits);

    m_data.m_header         = block::header_type(level,1);
    m_data.m_ptrs           = details::dbs_set::create(1);
    m_data.m_flags          = block::bit_mask(this_code);    

    m_data.get_fsb_set()->init(0, dbs_impl(prev_code));
};

dbs_impl::dbs_impl(size_t count, const size_t* elems)
{
    *this = build_dbs(count,elems);
};

dbs_impl dbs_impl::build_dbs(size_t count, const size_t* elems)
{
    if (count == 0)
        return dbs_impl();

    if (count == 1)
    {
        dbs_impl ret(elems[0]);
        return ret;
    };

    size_t max_elem     = elems[count - 1];
    ushort_type level   = get_level(max_elem);

    return build_level(level, count, elems);
};

dbs_impl dbs_impl::build_dbs(size_t count, const size_t* elems, size_t offset_bits)
{
    using block         = details::block;

    if (count == 0)
        return dbs_impl();

    if (count == 1)
    {
        dbs_impl ret(block::mod_pow2(elems[0], offset_bits));
        return ret;
    };

    size_t max_elem     = elems[count - 1];
    ushort_type level   = get_level(block::mod_pow2(max_elem, offset_bits));

    return build_level(level, count, elems);
};

dbs_impl::ushort_type dbs_impl::get_level(size_t max_elem)
{
    using block             = details::block;

    ushort_type level       = 0;
    size_t capacity_bits    = block_bits_log + 1;

    max_elem                = block::div_pow2(max_elem, capacity_bits);

    while (max_elem != 0)
    {
        max_elem            = block::div_pow2(max_elem, block_bits_log);
        ++level;
    };

    return level;
};

dbs_impl dbs_impl::build_level(ushort_type level, size_t count, const size_t* elems)
{
    using block             = details::block;

    if (level == 0)
    {
        dbs_impl ret;

        size_t capacity_bits= block_bits_log + 1;

        size_t& flags_0     = ret.m_data.get_block_0();
        size_t& flags_1     = ret.m_data.get_block_1();

        for (size_t i = 0; i < count; ++i)
        {
            size_t item     = block::mod_pow2(elems[i], capacity_bits);
            size_t pos      = block::div_pow2<1>(item);
            size_t block    = block::mod_pow2<1>(item);

            if (block == 0) 
                flags_0     |= block::bit_mask(pos);
            else
                flags_1     |= block::bit_mask(pos);
        };

        return ret;
    };

    using block             = block;

    size_t capacity_bits    = block_bits_log*(level-1) + block_bits_log + 1;

    size_t rem              = block::mod_pow2(block::div_pow2(elems[0], 
                                capacity_bits), block_bits_log);

    size_t ret_flags        = 0;
    ushort_type ret_size    = 0;

    using pod_dbs           = details::pod_type<dbs_impl>;
    pod_dbs buf[dbs_impl::block_bits];

    for(;;)
    {
        size_t cur_rem      = rem;
        size_t k            = 0;

        while(cur_rem == rem && k < count)
        {
            ++k;

            cur_rem         = block::mod_pow2(block::div_pow2(elems[k], 
                                capacity_bits), block_bits_log);
        };                     

        dbs_impl tmp        = build_dbs(k, elems, capacity_bits);
        new (buf + ret_size) dbs_impl(std::move(tmp));

        ret_flags           |= (size_t(1) << rem);
        ret_size            += 1;
            
        if (k >= count)
            break;

        rem                 = cur_rem;   
        elems               += k;
        count               -= k;
    };

    //build dbs
    if (ret_size == 1 && ret_flags == 1)
    {
        dbs_impl ret(reinterpret_cast<dbs_impl&&>(buf[0]));
        return ret;
    };

    block::header_type h(level, ret_size);
    dbs_impl ret(h, ret_flags, details::dbs_set::create(ret_size));

    for(ushort_type i = 0; i < ret_size; ++i)
        ret.m_data.get_fsb_set()->init(i, reinterpret_cast<dbs_impl&&>(buf[i]));

    return ret;
};

dbs_impl dbs_impl::set(size_t pos) const
{
    bool changed;
    return set(pos,changed);
};

dbs_impl dbs_impl::set(size_t pos, bool& changed) const
{
    using block             = details::block;
    size_t level            = m_data.get_level();
    size_t capacity_bits    = block_bits_log*level + 1;    
    
    size_t prev_lev_coord   = block::mod_pow2(pos, capacity_bits);
    size_t this_lev_coord   = block::div_pow2(pos, capacity_bits);

    if (this_lev_coord >= (size_t(1) << block_bits_log))
    {
        //dbs needs new levels        
        dbs_impl ret        = this->increase_level(pos);
        changed             = true;
        return ret;
    };

    if (level == 0)
    {
        dbs_impl ret(m_data.m_header, m_data.m_flags, m_data.get_fsb_set());                

        size_t& current_flags   = ret.m_data.get_block(prev_lev_coord);
        size_t old_flags        = current_flags;
        current_flags           |= block::bit_mask(this_lev_coord);
        changed                 = (old_flags != current_flags);

        return ret;
    };

    using block             = block;
    size_t has_this_block   = (this->m_data.m_flags & block::bit_mask(this_lev_coord));

    if (has_this_block == false)
    {
        dbs_impl ret        = this->insert_block(this_lev_coord, prev_lev_coord);
        changed             = true;
        return ret;
    }
    else
    {
        dbs_impl ret        = this->set_elem(this_lev_coord, prev_lev_coord, changed);
        return ret;
    };
};

dbs_impl dbs_impl::reset(size_t pos) const
{
    bool changed;
    return reset(pos,changed);
};

dbs_impl dbs_impl::reset(size_t pos, bool& changed) const
{
    using block             = details::block;
    size_t level            = m_data.get_level();
    size_t capacity_bits    = block_bits_log*level + 1;    

    size_t prev_lev_coord   = block::mod_pow2(pos, capacity_bits);
    size_t this_lev_coord   = block::div_pow2(pos, capacity_bits);

    if (this_lev_coord >= (size_t(1) << block_bits_log))
    {
        changed = false;
        return *this;
    };

    if (level == 0)
    {
        dbs_impl ret(m_data.m_header, m_data.m_flags, m_data.get_fsb_set());                

        size_t& current_flags   = ret.m_data.get_block(prev_lev_coord);
        size_t old_flags        = current_flags;
        current_flags           &= ~block::bit_mask(this_lev_coord);
        changed                 = (current_flags != old_flags);
        
        return ret;
    };

    using block             = block;
    size_t has_this_block   = (this->m_data.m_flags & block::bit_mask(this_lev_coord));

    if (has_this_block == false)
    {
        changed = false;
        return *this;
    };

    dbs_impl ret = this->reset_elem(this_lev_coord, prev_lev_coord, changed);
    return ret;
};

dbs_impl dbs_impl::flip(size_t pos) const
{
    using block             = details::block;

    size_t level            = m_data.get_level();
    size_t capacity_bits    = block_bits_log*level + 1;    

    size_t prev_lev_coord   = block::mod_pow2(pos, capacity_bits);
    size_t this_lev_coord   = block::div_pow2(pos, capacity_bits);

    if (this_lev_coord >= (size_t(1) << block_bits_log))
    {
        //dbs needs new levels
        dbs_impl ret        = this->increase_level(pos);
        return ret;
    };

    if (level == 0)
    {
        dbs_impl ret(m_data.m_header, m_data.m_flags, m_data.get_fsb_set());               

        size_t& current_flags   = ret.m_data.get_block(prev_lev_coord);
        current_flags           ^= block::bit_mask(this_lev_coord);

        return ret;
    };

    size_t has_this_block   = (this->m_data.m_flags & block::bit_mask(this_lev_coord));

    if (has_this_block == false)
    {
        dbs_impl ret = this->insert_block(this_lev_coord, prev_lev_coord);
        return ret;
    }
    else
    {
        dbs_impl ret = this->flip_elem(this_lev_coord, prev_lev_coord);
        return ret;
    };
};

bool dbs_impl::test(size_t pos) const
{
    using block             = details::block;

    size_t level            = m_data.get_level();
    size_t capacity_bits    = block_bits_log*level + 1;    
    
    size_t this_lev_coord   = block::div_pow2(pos, capacity_bits);

    if (this_lev_coord >= (size_t(1) << block_bits_log))
        return false;

    size_t prev_lev_coord   = block::mod_pow2(pos, capacity_bits);

    if (level == 0)
    {
        size_t test_res = m_data.get_block(prev_lev_coord) & block::bit_mask(this_lev_coord);
        return test_res != 0;
    };

    size_t has_this_block   = (this->m_data.m_flags & block::bit_mask(this_lev_coord));

    if (has_this_block == false)
        return false;
 
    size_t bits_before          = block::bits_before_pos(m_data.m_flags, this_lev_coord);
    size_t bits_before_count    = block::count_bits(bits_before);
    return this->m_data.get_fsb_set()->get_elem(bits_before_count).test(prev_lev_coord);
};

dbs_impl dbs_impl::increase_level(size_t pos) const
{    
    if (this->none() == true)
        return dbs_impl(pos);

    using block             = details::block;
    ushort_type level       = get_level(pos);
    size_t capacity_bits    = block_bits_log*level + 1;
    
    size_t this_code        = block::div_pow2(pos, capacity_bits);
    size_t prev_code        = block::mod_pow2(pos, capacity_bits);

    block::header_type h(level,2);
    size_t flags            = (block::bit_mask(this_code) | size_t(1));

    dbs_impl ret(h,flags,details::dbs_set::create(2));
    dbs_impl new_item(prev_code);

    ret.m_data.get_fsb_set()->init(0, *this);
    ret.m_data.get_fsb_set()->init(1, std::move(new_item));

    return ret;
};

dbs_impl dbs_impl::insert_block(size_t this_level_coord, size_t prev_level_coord) const
{    
    using block             = details::block;
    ushort_type old_size    = this->m_data.m_header.get_size();
    ushort_type new_size    = old_size + 1;

    block::header_type h(this->m_data.get_level(), new_size);

    size_t old_flags        = this->m_data.m_flags;
    size_t new_flags        = (old_flags | block::bit_mask(this_level_coord));

    dbs_impl ret(h,new_flags, details::dbs_set::create(new_size));

    size_t bits_before      = block::bits_before_pos(old_flags, this_level_coord);
    size_t bits_before_count= block::count_bits(bits_before);

    for (size_t i = 0; i < bits_before_count; ++i)
        ret.m_data.get_fsb_set()->init(i,this->m_data.get_fsb_set()->get_elem(i));

    {
        dbs_impl new_dbs(prev_level_coord);    
        ret.m_data.get_fsb_set()->init(bits_before_count,std::move(new_dbs));
    };

    for (size_t i = bits_before_count; i < old_size; ++i)
        ret.m_data.get_fsb_set()->init(i + 1, this->m_data.get_fsb_set()->get_elem(i));

    return ret;
};

dbs_impl dbs_impl::set_elem(size_t this_level_coord, size_t prev_level_coord, 
                            bool& changed) const
{
    using block                 = details::block;

    ushort_type old_size        = this->m_data.m_header.get_size();
    size_t old_flags            = this->m_data.m_flags;
    
    size_t bits_before          = block::bits_before_pos(old_flags, this_level_coord);
    size_t bits_before_count    = block::count_bits(bits_before);

    dbs_impl new_dbs            = this->m_data.get_fsb_set()->get_elem(bits_before_count)
                                            .set(prev_level_coord, changed);

    if (changed == false)
        return *this;

    block::header_type h(this->m_data.get_level(), old_size);

    dbs_impl ret(h, old_flags, details::dbs_set::create(old_size)); 

    for (size_t i = 0; i < bits_before_count; ++i)
        ret.m_data.get_fsb_set()->init(i,this->m_data.get_fsb_set()->get_elem(i));

    ret.m_data.get_fsb_set()->init(bits_before_count,std::move(new_dbs));

    for (size_t i = bits_before_count + 1; i < old_size; ++i)
        ret.m_data.get_fsb_set()->init(i,this->m_data.get_fsb_set()->get_elem(i));

    return ret;
};

dbs_impl dbs_impl::reset_elem(size_t this_level_coord, size_t prev_level_coord, bool& changed) const
{
    using block                 = details::block;

    ushort_type old_size        = this->m_data.m_header.get_size();
    size_t old_flags            = this->m_data.m_flags;
    
    size_t bits_before          = block::bits_before_pos(old_flags, this_level_coord);
    size_t bits_before_count    = block::count_bits(bits_before);

    dbs_impl new_dbs            = this->m_data.get_fsb_set()->get_elem(bits_before_count)
                                            .reset(prev_level_coord, changed);
    
    if (changed == false)
        return *this;   

    bool new_empty  = new_dbs.none();

    if (new_empty == true)
    {
        if (old_size == 1)
            return dbs_impl();

        old_flags   &= ~block::bit_mask(this_level_coord);

        if (old_flags == 1)
            return this->m_data.get_fsb_set()->get_elem(0);

        block::header_type h(this->m_data.get_level(), old_size - 1);

        dbs_impl ret(h, old_flags, details::dbs_set::create(old_size - 1)); 

        for (size_t i = 0; i < bits_before_count; ++i)
            ret.m_data.get_fsb_set()->init(i, this->m_data.get_fsb_set()->get_elem(i));

        for (size_t i = bits_before_count + 1; i < old_size; ++i)
            ret.m_data.get_fsb_set()->init(i - 1, this->m_data.get_fsb_set()->get_elem(i));

        return ret;
    }
    else
    {
        block::header_type h(this->m_data.get_level(), old_size);

        dbs_impl ret(h, old_flags, details::dbs_set::create(old_size)); 

        for (size_t i = 0; i < bits_before_count; ++i)
            ret.m_data.get_fsb_set()->init(i, this->m_data.get_fsb_set()->get_elem(i));

        ret.m_data.get_fsb_set()->init(bits_before_count,std::move(new_dbs));

        for (size_t i = bits_before_count + 1; i < old_size; ++i)
            ret.m_data.get_fsb_set()->init(i, this->m_data.get_fsb_set()->get_elem(i));

        return ret;
    };    
};

dbs_impl dbs_impl::flip_elem(size_t this_level_coord, size_t prev_level_coord) const
{
    using block                 = details::block;

    ushort_type old_size        = this->m_data.m_header.get_size();
    size_t old_flags            = this->m_data.m_flags;
    
    size_t bits_before          = block::bits_before_pos(old_flags, this_level_coord);
    size_t bits_before_count    = block::count_bits(bits_before);

    dbs_impl new_dbs            = this->m_data.get_fsb_set()->get_elem(bits_before_count)
                                        .flip(prev_level_coord);    

    bool new_empty              = new_dbs.none();

    if (new_empty == true)
    {
        if (old_size == 1)
            return dbs_impl();

        old_flags   &= ~block::bit_mask(this_level_coord);

        if (old_flags == 1)
            return this->m_data.get_fsb_set()->get_elem(0);

        block::header_type h(this->m_data.get_level(), old_size - 1);

        dbs_impl ret(h, old_flags, details::dbs_set::create(old_size - 1)); 

        for (size_t i = 0; i < bits_before_count; ++i)
            ret.m_data.get_fsb_set()->init(i, this->m_data.get_fsb_set()->get_elem(i));

        for (size_t i = bits_before_count + 1; i < old_size; ++i)
            ret.m_data.get_fsb_set()->init(i - 1, this->m_data.get_fsb_set()->get_elem(i));

        return ret;
    }
    else
    {
        block::header_type h(this->m_data.get_level(), old_size);

        dbs_impl ret(h, old_flags, details::dbs_set::create(old_size)); 

        for (size_t i = 0; i < bits_before_count; ++i)
            ret.m_data.get_fsb_set()->init(i, this->m_data.get_fsb_set()->get_elem(i));

        ret.m_data.get_fsb_set()->init(bits_before_count,std::move(new_dbs));

        for (size_t i = bits_before_count + 1; i < old_size; ++i)
            ret.m_data.get_fsb_set()->init(i, this->m_data.get_fsb_set()->get_elem(i));

        return ret;
    };    
};

bool dbs_impl::any() const
{
    if (this->m_data.m_flags != 0)
        return true;

    if (this->m_data.get_level() == 0 && this->m_data.get_block_1() != 0)
        return true;

    return false;
};

bool dbs_impl::none() const
{
    if (this->m_data.m_flags != 0)
        return false;

    if (this->m_data.get_level() == 0 && this->m_data.get_block_1() != 0)
        return false;

    return true;
};

size_t dbs_impl::size() const
{
    using block     = details::block;

    if (this->m_data.get_level() == 0)
    {
        size_t count = block::count_bits(this->m_data.get_block_0());
        count       += block::count_bits(this->m_data.get_block_1());
        return count;
    };

    size_t count    = 0;
    size_t elem     = this->m_data.m_header.get_size();

    for (size_t i = 0; i < elem; ++i)
        count       += this->m_data.get_fsb_set()->get_elem(i).size();

    return count;
};

size_t dbs_impl::hash_value_impl() const
{
    if (this->m_data.get_level() == 0)
    {
        size_t seed = this->m_data.get_block_0();
        boost::hash_combine(seed, this->m_data.get_block_1());
        return seed;
    };

    size_t seed = 0;
    size_t elem = this->m_data.m_header.get_size();

    for (size_t i = 0; i < elem; ++i)
    {
        size_t hash = this->m_data.get_fsb_set()->get_elem(i).hash_value_impl();
        boost::hash_combine(seed, hash);
    }

    return seed;
};

void dbs_impl::get_elements(std::vector<size_t>& elems) const
{
    size_t n = this->size();
    elems.reserve(n);

    return get_elements(0,elems);
};

size_t dbs_impl::first() const
{
    if (this->size() == 0)
        return npos;

    using block         = details::block;
    size_t level        = m_data.get_level();    

    using header_type   = block::header_type;

    if (level == 0)
    {
        size_t block1   = this->m_data.get_block_0();
        size_t block2   = this->m_data.get_block_1();

        size_t pos_1    = (size_t)-1;
        size_t pos_2    = (size_t)-1;
        
        if (block1)
        {
            pos_1       = header_type::least_significant_bit_pos(block1);
            pos_1       = pos_1 * 2;
        }
        if (block2)
        {
            pos_2       = header_type::least_significant_bit_pos(block2);
            pos_2       = pos_2 * 2 + 1;
        };

        return std::min(pos_1, pos_2);
    };

    size_t offset_bits  = block_bits_log*(level-1) + block_bits_log + 1;
    size_t block_flags  = this->m_data.m_flags;

    size_t pos          = header_type::least_significant_bit_pos(block_flags);
    size_t offset       = (pos << offset_bits);

    size_t val          = this->m_data.get_fsb_set()->get_elem(0).first();
    return val + offset;
};

dbs_impl::block_type& dbs_impl::get_data()
{
    return m_data;
}

const dbs_impl::block_type& dbs_impl::get_data() const
{
    return m_data;
}

size_t dbs_impl::last() const
{
    if (this->size() == 0)
        return npos;

    size_t level        = m_data.get_level();    
    using block         = details::block;
    using header_type   = block::header_type;

    if (level == 0)
    {
        size_t block1   = this->m_data.get_block_0();
        size_t block2   = this->m_data.get_block_1();

        size_t pos_1    = 0;
        size_t pos_2    = 0;
        
        if (block1)
        {
            pos_1       = header_type::most_significant_bit_pos(block1);
            pos_1       = pos_1 * 2;
        }

        if (block2)
        {
            pos_2       = header_type::most_significant_bit_pos(block2);
            pos_2       = pos_2 * 2 + 1;
        };

        return std::max(pos_1, pos_2);
    };

    size_t offset_bits  = block_bits_log*(level-1) + block_bits_log + 1;
    size_t block_flags  = this->m_data.m_flags;
    size_t size         = this->m_data.m_header.get_size();

    size_t pos          = header_type::most_significant_bit_pos(block_flags);
    size_t offset       = (pos << offset_bits);

    size_t val          = this->m_data.get_fsb_set()->get_elem(size-1).first();
    return val + offset;
};

void dbs_impl::get_elements(size_t offset, std::vector<size_t>& elems) const
{
    size_t level        = m_data.get_level();    

    if (level == 0)
    {
        size_t block1   = this->m_data.get_block_0();
        size_t block2   = this->m_data.get_block_1();
        size_t pos      = 0;

        while(block1 && block2)
        {
            if (block1 % 2 == size_t(1))
                elems.push_back(offset + (pos << 1));

            if (block2 % 2 == size_t(1))
                elems.push_back(offset + (pos << 1) + 1);

            ++pos;            
            block1      = block1 >> size_t(1);
            block2      = block2 >> size_t(1);
        };

        while(block1)
        {
            if (block1 % 2 == size_t(1))
                elems.push_back(offset + (pos << 1));

            ++pos;            
            block1      = block1 >> size_t(1);
        };

        while(block2)
        {
            if (block2 % 2 == size_t(1))
                elems.push_back(offset + (pos << 1) + 1);

            ++pos;            
            block2      = block2 >> size_t(1);
        };

        return;
    };

    size_t offset_bits  = block_bits_log*(level-1) + block_bits_log + 1;
    size_t block        = this->m_data.m_flags;
    size_t pos          = 0;
    size_t k            = 0;

    while(block)
    {
        if (block % 2 == size_t(1))
        {
            this->m_data.get_fsb_set()->get_elem(k)
                    .get_elements(offset + (pos << offset_bits), elems);
            ++k;
        };
    
        ++pos;            
        block      = block >> size_t(1);
    };
};

}}

namespace dbs_lib
{

//-----------------------------------------------------------------------------------
//                              dbs
//-----------------------------------------------------------------------------------

dbs::dbs()
{};

dbs::dbs(size_t elem)
    :details::dbs_impl(elem)
{}

dbs::dbs(size_t count, const size_t* elems)
    :details::dbs_impl(count, elems)
{};
        
dbs::dbs(std::initializer_list<size_t> elems)
    :details::dbs_impl(elems.size(), elems.begin())
{};

dbs::dbs(const dbs& copy)
    :details::dbs_impl(copy)
{};

dbs::dbs(dbs&& copy)
    :details::dbs_impl(std::move(copy))
{};
 
dbs::dbs(const details::dbs_impl& impl)
    :details::dbs_impl(impl)
{};

dbs::dbs(details::dbs_impl&& impl)
    :details::dbs_impl(std::move(impl))
{};

dbs::~dbs()
{};

dbs& dbs::operator=(const dbs& copy)
{
    details::dbs_impl::operator=(copy);
    return *this;
};

dbs& dbs::operator=(dbs&& copy)
{
    details::dbs_impl::operator=(std::move(copy));
    return *this;
};

size_t dbs::size() const
{
    return details::dbs_impl::size();
};

bool dbs::any() const
{
    return details::dbs_impl::any();
};

bool dbs::none() const
{
    return details::dbs_impl::none();
};

dbs dbs::set(size_t pos) const
{
    return dbs(details::dbs_impl::set(pos));
};

dbs dbs::reset(size_t pos) const
{
    return dbs(details::dbs_impl::reset(pos));
};

dbs dbs::flip(size_t pos) const
{
    return dbs(details::dbs_impl::flip(pos));
};

bool dbs::test(size_t n) const
{
    return details::dbs_impl::test(n);
};

size_t dbs::first() const
{
    return details::dbs_impl::first();
};

size_t dbs::last() const
{
    return details::dbs_impl::last();
};

bool dbs::test_any(const dbs& other) const
{
    //TODO: optimize this
    dbs link = *this & other;

    return link.any();
};

bool dbs::test_all(const dbs& other) const
{
    //TODO: optimize this
    dbs link = *this & other;

    return link == other;
};


void dbs::get_elements(std::vector<size_t>& elems) const
{
    return details::dbs_impl::get_elements(elems);
};

//-----------------------------------------------------------------------------------
//                              OPERATORS
//-----------------------------------------------------------------------------------

size_t hash_value(const dbs& x)
{
    return x.hash_value_impl();
}

dbs operator&(const dbs& x, const dbs& y)
{
    using block_type    = details::block;
    using ushort_type   = details::block::ushort_type;
    using dbs_impl      = details::dbs_impl;

    ushort_type level_1 = x.get_data().get_level();
    ushort_type level_2 = y.get_data().get_level();
    ushort_type level   = std::min(level_1, level_2);

    const dbs_impl* xl  = &x;
    const dbs_impl* yl  = &y;

    while(level_1 > level)
    {
        size_t sel      = xl->get_data().m_flags & size_t(1);

        if (sel == 0)
            return dbs();

        xl              = &xl->get_data().get_fsb_set()->get_elem(0);
        --level_1;
    };

    while(level_2 > level)
    {
        size_t sel      = yl->get_data().m_flags & size_t(1);

        if (sel == 0)
            return dbs();

        yl              = &yl->get_data().get_fsb_set()->get_elem(0);
        --level_2;
    };    

    if (level == 0)
    {
        dbs ret;
        ret.get_data().get_block_0() = xl->get_data().get_block_0() 
                                        & yl->get_data().get_block_0();
        ret.get_data().get_block_1() = xl->get_data().get_block_1() 
                                        & yl->get_data().get_block_1();

        return ret;
    };

    size_t flags_1          = xl->get_data().m_flags;
    size_t flags_2          = yl->get_data().m_flags;
    size_t ret_flags_est    = flags_1 & flags_2;

    if (ret_flags_est == 0)
        return dbs();

    using pod_dbs   = details::pod_type<dbs>;
    
    pod_dbs buf[block_type::block_bits];
    ushort_type ret_size    = 0;
    size_t pos_flag_1       = 0;
    size_t pos_flag_2       = 0;
    size_t ret_flags        = 0;
    size_t cur_mask         = 1;

    //build items array
    while(flags_1 && flags_2)
    {
        bool has_1      = (flags_1 % 2) != 0;
        bool has_2      = (flags_2 % 2) != 0;

        if (has_1)
        {
            if (has_2)
            {
                const dbs_impl& e1  = xl->get_data().get_fsb_set()->get_elem(pos_flag_1);
                const dbs_impl& e2  = yl->get_data().get_fsb_set()->get_elem(pos_flag_2);

                dbs res         = dbs_lib::operator&(dbs(e1), dbs(e2));

                if (res.any() == true)
                { 
                    new (buf + ret_size) dbs(std::move(res));

                    ret_flags   += cur_mask;
                    ++ret_size;
                };

                ++pos_flag_2;
            };

            ++pos_flag_1;
        }
        else if (has_2)
        {
            ++pos_flag_2;
        };

        flags_1     = flags_1 >> size_t(1);
        flags_2     = flags_2 >> size_t(1);
        cur_mask    = cur_mask << size_t(1);
    };

    //construct dbs
    if (ret_size == 0)
        return dbs();

    if (ret_flags == 1)
    {
        dbs ret(reinterpret_cast<dbs&&>(buf[0]));
        return ret;
    };

    using block         = details::block;

    block::header_type h(level, ret_size);
    dbs_impl ret(h, ret_flags, details::dbs_set::create(ret_size));

    for(ushort_type i = 0; i < ret_size; ++i)
        ret.get_data().get_fsb_set()->init(i, reinterpret_cast<dbs&&>(buf[i]));

    return dbs(ret);
};

dbs operator|(const dbs& x, const dbs& y)
{
    using block_type    = details::block;
    using ushort_type   = details::block::ushort_type;

    ushort_type level_1 = x.get_data().get_level();
    ushort_type level_2 = y.get_data().get_level();

    if (level_1 < level_2)
        return dbs_lib::operator|(y,x);

    ushort_type level   = std::max(level_1, level_2);

    if (level == 0)
    {
        dbs ret;
        ret.get_data().get_block_0() = x.get_data().get_block_0() 
                                        | y.get_data().get_block_0();
        ret.get_data().get_block_1() = x.get_data().get_block_1() 
                                        | y.get_data().get_block_1();

        return ret;
    };

    if (y.none() == true)
        return x;

    using block         = details::block;
    using dbs_impl      = details::dbs_impl;

    if (level_1 > level_2)
    {
        bool has_zero       = (x.get_data().m_flags & size_t(1)) != 0;

        if (has_zero)
        {
            dbs elem_zero   = dbs(x.get_data().get_fsb_set()->get_elem(0))
                            | dbs(y);

            //construct dbs
            ushort_type ret_size    = x.get_data().m_header.get_size();
            size_t ret_flags        = x.get_data().m_flags;

            block::header_type h(level, ret_size);
            dbs_impl ret(h, ret_flags, details::dbs_set::create(ret_size));

            ret.get_data().get_fsb_set()->init(0, std::move(elem_zero));

            for(ushort_type i = 1; i < ret_size; ++i)
            {
                ret.get_data().get_fsb_set()->init(i, 
                            x.get_data().get_fsb_set()->get_elem(i));
            }

            return dbs(ret);
        }
        else
        {
            //construct dbs
            ushort_type x_size      = x.get_data().m_header.get_size();
            size_t ret_flags        = (x.get_data().m_flags | size_t(1));

            block::header_type h(level, x_size + 1);
            dbs_impl ret(h, ret_flags, details::dbs_set::create(x_size + 1));

            ret.get_data().get_fsb_set()->init(0, y);

            for(ushort_type i = 0; i < x_size; ++i)
            {
                ret.get_data().get_fsb_set()->init(i + 1, 
                            x.get_data().get_fsb_set()->get_elem(i));
            }

            return dbs(ret);
        };
    }

    using pod_dbs           = details::pod_type<dbs>;
    using dbs_impl          = details::dbs_impl;
    
    pod_dbs buf[block::block_bits];

    size_t flags_1          = x.get_data().m_flags;
    size_t flags_2          = y.get_data().m_flags;

    ushort_type ret_size    = 0;
    size_t pos_flag_1       = 0;
    size_t pos_flag_2       = 0;
    size_t ret_flags        = 0;
    size_t cur_mask         = 1;

    //build items array
    while(flags_1 || flags_2)
    {
        bool has_1      = (flags_1 % 2) != 0;
        bool has_2      = (flags_2 % 2) != 0;

        if (has_1)
        {
            if (has_2)
            {
                const dbs_impl& e1  = x.get_data().get_fsb_set()->get_elem(pos_flag_1);
                const dbs_impl& e2  = y.get_data().get_fsb_set()->get_elem(pos_flag_2);

                dbs res         = dbs(e1) | dbs(e2);

                new (buf + ret_size) dbs(std::move(res));

                ret_flags       += cur_mask;

                ++ret_size;

                ++pos_flag_1;
                ++pos_flag_2;
            }
            else
            {
                const dbs_impl& e1  = x.get_data().get_fsb_set()->get_elem(pos_flag_1);

                new (buf + ret_size) dbs(e1);

                ret_flags       += cur_mask;

                ++ret_size;
                ++pos_flag_1;
            };
        }
        else if (has_2)
        {
            const dbs_impl& e2  = y.get_data().get_fsb_set()->get_elem(pos_flag_2);

            new (buf + ret_size) dbs(e2);

            ret_flags           += cur_mask;

            ++ret_size;
            ++pos_flag_2;
        };

        flags_1     = flags_1 >> size_t(1);
        flags_2     = flags_2 >> size_t(1);
        cur_mask    = cur_mask << size_t(1);
    };

    //construct dbs
    block::header_type h(level, ret_size);
    dbs_impl ret(h, ret_flags, details::dbs_set::create(ret_size));

    for(ushort_type i = 0; i < ret_size; ++i)
        ret.get_data().get_fsb_set()->init(i, reinterpret_cast<dbs&&>(buf[i]));

    return dbs(ret);
};

dbs operator^(const dbs& x, const dbs& y)
{
    using block_type    = details::block;
    using ushort_type   = details::block::ushort_type;

    ushort_type level_1 = x.get_data().get_level();
    ushort_type level_2 = y.get_data().get_level();

    if (level_1 < level_2)
        return dbs_lib::operator^(y,x);

    ushort_type level   = std::max(level_1, level_2);

    if (level == 0)
    {
        dbs ret;
        ret.get_data().get_block_0() = x.get_data().get_block_0() 
                                        ^ y.get_data().get_block_0();
        ret.get_data().get_block_1() = x.get_data().get_block_1() 
                                        ^ y.get_data().get_block_1();

        return ret;
    };

    if (x.none() == true)
        return y;

    if (y.none() == true)
        return x;

    using block             = details::block;
    using dbs_impl          = details::dbs_impl;

    if (level_1 > level_2)
    {
        bool has_zero       = (x.get_data().m_flags & size_t(1)) != 0;

        if (has_zero)
        {
            dbs elem_zero   = dbs(x.get_data().get_fsb_set()->get_elem(0))
                                ^ dbs(y);

            if (elem_zero.none() == true)
            {
                //construct dbs
                ushort_type x_size  = x.get_data().m_header.get_size();

                if (x_size == 1)
                    return dbs();

                size_t ret_flags    = x.get_data().m_flags & ~size_t(1);

                block::header_type h(level, x_size - 1);
                dbs_impl ret(h, ret_flags, details::dbs_set::create(x_size - 1));

                for(ushort_type i = 1; i < x_size; ++i)
                {
                    ret.get_data().get_fsb_set()->init(i - 1, 
                                x.get_data().get_fsb_set()->get_elem(i));
                }

                return dbs(ret);
            }
            else
            {
                //construct dbs
                ushort_type ret_size    = x.get_data().m_header.get_size();
                size_t ret_flags        = x.get_data().m_flags;

                block::header_type h(level, ret_size);
                dbs_impl ret(h, ret_flags, details::dbs_set::create(ret_size));

                ret.get_data().get_fsb_set()->init(0, std::move(elem_zero));

                for(ushort_type i = 1; i < ret_size; ++i)
                {
                    ret.get_data().get_fsb_set()->init(i, 
                                    x.get_data().get_fsb_set()->get_elem(i));
                }

                return dbs(ret);
            };
        }
        else
        {
            //construct dbs
            ushort_type x_size      = x.get_data().m_header.get_size();
            size_t ret_flags        = (x.get_data().m_flags | size_t(1));

            block::header_type h(level, x_size + 1);
            dbs_impl ret(h, ret_flags, details::dbs_set::create(x_size + 1));

            ret.get_data().get_fsb_set()->init(0, y);

            for(ushort_type i = 0; i < x_size; ++i)
            {
                ret.get_data().get_fsb_set()->init(i + 1, 
                                x.get_data().get_fsb_set()->get_elem(i));
            }

            return dbs(ret);
        };
    }

    using pod_dbs           = details::pod_type<dbs>;
    using dbs_impl          = details::dbs_impl;
    
    pod_dbs buf[block::block_bits];

    size_t flags_1          = x.get_data().m_flags;
    size_t flags_2          = y.get_data().m_flags;

    ushort_type ret_size    = 0;
    size_t pos_flag_1       = 0;
    size_t pos_flag_2       = 0;
    size_t ret_flags        = 0;
    size_t cur_mask         = 1;

    //build items array
    while(flags_1 || flags_2)
    {
        bool has_1      = (flags_1 % 2) != 0;
        bool has_2      = (flags_2 % 2) != 0;

        if (has_1)
        {
            if (has_2)
            {
                const dbs_impl& e1  = x.get_data().get_fsb_set()->get_elem(pos_flag_1);
                const dbs_impl& e2  = y.get_data().get_fsb_set()->get_elem(pos_flag_2);

                dbs res         = dbs(e1) ^ dbs(e2);

                if (res.none() == false)
                {
                    new (buf + ret_size) dbs(std::move(res));

                    ret_flags   += cur_mask;
                    ++ret_size;
                };

                ++pos_flag_1;
                ++pos_flag_2;
            }
            else
            {
                const dbs_impl& e1  = x.get_data().get_fsb_set()->get_elem(pos_flag_1);

                new (buf + ret_size) dbs(e1);

                ret_flags       += cur_mask;

                ++ret_size;
                ++pos_flag_1;
            };
        }
        else if (has_2)
        {
            const dbs_impl& e2  = y.get_data().get_fsb_set()->get_elem(pos_flag_2);

            new (buf + ret_size) dbs(e2);

            ret_flags           += cur_mask;

            ++ret_size;
            ++pos_flag_2;
        };

        flags_1     = flags_1 >> size_t(1);
        flags_2     = flags_2 >> size_t(1);
        cur_mask    = cur_mask << size_t(1);
    };

    //construct dbs
    if (ret_size == 0)
        return dbs();

    if (ret_flags == 1)
    {
        dbs ret(reinterpret_cast<dbs&&>(buf[0]));
        return ret;
    };

    block::header_type h(level, ret_size);
    dbs_impl ret(h, ret_flags, details::dbs_set::create(ret_size));

    for(ushort_type i = 0; i < ret_size; ++i)
        ret.get_data().get_fsb_set()->init(i, reinterpret_cast<dbs&&>(buf[i]));

    return dbs(ret);
};

order_type compare(const dbs& x, const dbs& y)
{
    //lexicographic order

    size_t level_1  = x.get_data().get_level();
    size_t level_2  = y.get_data().get_level();

    if (level_1 < level_2)
        return order_type::less;

    if (level_1 > level_2)
        return order_type::greater;

    if (level_1 == 0)
    {
        {
            size_t flags_1  = x.get_data().get_block_0();
            size_t flags_2  = y.get_data().get_block_0();

            if (flags_1 < flags_2)
                return order_type::less;
    
            if (flags_1 > flags_2)
                return order_type::greater;
        };

        {
            size_t flags_1  = x.get_data().get_block_1();
            size_t flags_2  = y.get_data().get_block_1();

            if (flags_1 < flags_2)
                return order_type::less;
            
            if (flags_1 > flags_2)
                return order_type::greater;
        };

        //elements are equal
        return order_type::equal;
    };

    if (x.get_data().m_flags < y.get_data().m_flags)
        return order_type::less;

    if (x.get_data().m_flags > y.get_data().m_flags)
        return order_type::greater;

    size_t size = x.get_data().m_header.get_size();

    using dbs_impl  = details::dbs_impl;

    for (size_t i = 0; i < size; ++i)
    {
        const dbs_impl& elem_1  = x.get_data().get_fsb_set()->get_elem(i);
        const dbs_impl& elem_2  = y.get_data().get_fsb_set()->get_elem(i);

        order_type ot       = dbs_lib::compare(dbs(elem_1), dbs(elem_2));
    
        if (ot != order_type::equal)
            return ot;
    };

    return order_type::equal;
};

bool operator==(const dbs& x, const dbs& y)
{
    return compare(x,y) == order_type::equal;
};

bool operator!=(const dbs& x, const dbs& y)
{
    return compare(x,y) != order_type::equal;
};

bool operator<(const dbs& x, const dbs& y)
{
    return compare(x,y) == order_type::less;
};

bool operator>(const dbs& x, const dbs& y)
{
    return compare(x,y) == order_type::greater;
};

bool operator<=(const dbs& x, const dbs& y)
{
    return compare(x,y) != order_type::greater;
};

bool operator>=(const dbs& x, const dbs& y)
{
    return compare(x,y) != order_type::less;
};

std::ostream& dbs_lib::operator<<(std::ostream& os, const dbs& x)
{
    std::vector<size_t> elems;
    x.get_elements(elems);

    if (elems.size() == 0)
    {
        os << "{}";
        return os;
    }

    os << "{";
    os << elems[0];

    for (size_t i = 1; i < elems.size(); ++i)
        os << ", " << elems[i];

    os << "}";

    return os;
};

}