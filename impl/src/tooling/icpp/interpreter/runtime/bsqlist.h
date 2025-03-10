//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "bsqvalue.h"

#include <bitset>

#define LIST_LOAD_DATA(SL) SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(SL)
#define LIST_LOAD_REPR_TYPE(SL) SLPTR_LOAD_HEAP_TYPE_AS(BSQListReprType, SL)

#define LIST_STORE_RESULT_REPR(R, SL) SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(SL, R)
#define LIST_STORE_RESULT_EMPTY(SL) SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(SL, nullptr)

enum class ListReprKind
{
    PV4,
    PV8,
    TreeElement
};

class BSQListReprType : public BSQRefType
{
public:
    const BSQTypeID entrytype;
    const ListReprKind lkind;

    BSQListReprType(BSQTypeID tid, uint64_t allocsize, RefMask heapmask, DisplayFP fpDisplay, std::string name, BSQTypeID entrytype, ListReprKind lkind)
    : BSQRefType(tid, allocsize, heapmask, {}, EMPTY_KEY_CMP, fpDisplay, name), entrytype(entrytype), lkind(lkind)
    {;}

    virtual ~BSQListReprType() {;}

    virtual uint64_t getCount(void* repr) const = 0;
};

std::string entityPartialVectorDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQPartialVectorType : public BSQListReprType
{
public:
    const size_t entrysize;

    BSQPartialVectorType(BSQTypeID tid, uint64_t allocsize, RefMask heapmask, std::string name, BSQTypeID entrytype, ListReprKind lkind, size_t entrysize) 
    : BSQListReprType(tid, allocsize, heapmask, entityPartialVectorDisplay_impl, name, entrytype, lkind), entrysize(entrysize)
    {;}

    virtual ~BSQPartialVectorType() {;}

    virtual uint64_t getCount(void* repr) const override final
    {
        return *((uint64_t*)repr);
    }

    inline static int16_t getPVCount(void* repr) 
    {
        return (int16_t)*((uint64_t*)repr);
    }

    inline static void setPVCount(void* repr, int16_t count)
    {
        *((uint64_t*)repr) = (uint64_t)count;
    }

    inline StorageLocationPtr get(void* repr, int16_t i) const
    {
        return ((uint8_t*)repr) + sizeof(uint64_t) + (i * this->entrysize);
    }

    inline static void initializePVData(void* pvinto, const std::vector<StorageLocationPtr>& vals, const BSQType* entrytype)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);

        BSQPartialVectorType::setPVCount(pvinto, vals.size());
        for(size_t i = 0; i < vals.size(); ++i)
        {
            entrytype->storeValue(intoloc, vals[i]);
            intoloc += entrytype->allocinfo.inlinedatasize;
        }
    }

    inline static void initializePVDataSingle(void* pvinto, StorageLocationPtr val, const BSQType* entrytype)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);

        BSQPartialVectorType::setPVCount(pvinto, 1);
        entrytype->storeValue(intoloc, val);
    }

    inline static void directSetPVData(void* pvinto, void* pvfrom, size_t count, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        auto bytecount = (count * entrysize);

        BSQPartialVectorType::setPVCount(pvinto, count);
        GC_MEM_COPY(intoloc, fromloc, bytecount);
    }

    inline static void appendPVData(void* pvinto, void* pvfrom, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + (sizeof(uint64_t) + (*((uint64_t*)pvinto) * entrysize));
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        auto bytecount = (*((uint64_t*)pvfrom) * entrysize);

        GC_MEM_COPY(intoloc, fromloc, bytecount);
        *((uint64_t*)pvinto) += *((uint64_t*)pvfrom);
    }

    inline static void pushFrontPVData(void* pvinto, void* pvfrom, StorageLocationPtr val, uint64_t entrysize)
    {
        auto newloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t) + entrysize;
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        auto bytecount = (*((uint64_t*)pvfrom) * entrysize);

        GC_MEM_COPY(newloc, val, entrysize);
        GC_MEM_COPY(intoloc, fromloc, bytecount);
        *((uint64_t*)pvinto) = *((uint64_t*)pvfrom) + 1;
    }

    inline static void pushBackPVData(void* pvinto, void* pvfrom, StorageLocationPtr val, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto newloc = ((uint8_t*)pvinto) + sizeof(uint64_t) + (*((uint64_t*)pvfrom) * entrysize);
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        auto bytecount = (*((uint64_t*)pvfrom) * entrysize);

        GC_MEM_COPY(intoloc, fromloc, bytecount);
        GC_MEM_COPY(newloc, val, entrysize);
        *((uint64_t*)pvinto) = *((uint64_t*)pvfrom) + 1;
    }

    inline static void slicePVData(void* pvinto, void* pvfrom, int16_t start, int16_t end, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto fromloc = ((uint8_t*)pvfrom) + (sizeof(uint64_t) + (*((uint64_t*)pvinto) * start));
        auto bytecount = ((end - start) * entrysize);

        GC_MEM_COPY(intoloc, fromloc, bytecount);
        *((uint64_t*)pvinto) = (end - start);
    }

    inline static void packPVData(void* pvinto, void* pvfrom, std::bitset<8> mask, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        
        uint64_t jj = 0;
        for(size_t i = 0; i < 8; ++i)
        {
            if(mask[i])
            {
                auto dst = intoloc + (entrysize * jj);
                auto src = fromloc + (entrysize * i);

                GC_MEM_COPY(dst, src, entrysize);
                jj++;
            }
        }

        *((uint64_t*)pvinto) = jj;
    }

    inline static void setPVData(void* pvinto, void* pvfrom, int16_t idx, StorageLocationPtr v, int16_t end, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        
        for(size_t i = 0; i < end; ++i)
        {
            auto dst = intoloc + (entrysize * i);
            auto src = (i != idx) ? (fromloc + (entrysize * i)) : ((uint8_t*)v);

            GC_MEM_COPY(dst, src, entrysize);
        }

        *((uint64_t*)pvinto) = end;
    }

    inline static void initializePVDataInsert(void* pvinto, void* pvfrom, int16_t ipos, StorageLocationPtr v, int16_t start, int16_t end, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        
        int16_t jj = 0;
        for(int16_t ii = start; ii < end; ++ii)
        {
            if(ii == ipos)
            {
                auto dstv = intoloc + (entrysize * jj);
                GC_MEM_COPY(dstv, v, entrysize);

                jj++;
            }

            auto src = fromloc + (entrysize * ii);
            auto dst = intoloc + (entrysize * jj);
            GC_MEM_COPY(dst, src, entrysize);
        }

        *((uint64_t*)pvinto) = (uint64_t)jj;
    }

    inline static void removePVData(void* pvinto, void* pvfrom, int16_t idx, int16_t end, uint64_t entrysize)
    {
        auto intoloc = ((uint8_t*)pvinto) + sizeof(uint64_t);
        auto fromloc = ((uint8_t*)pvfrom) + sizeof(uint64_t);
        
        uint64_t jj = 0;
        for(size_t i = 0; i < 8 && i < end; ++i)
        {
            if(i != idx)
            {
                auto dst = intoloc + (entrysize * jj);
                auto src = fromloc + (entrysize * i);

                GC_MEM_COPY(dst, src, entrysize);
                jj++;
            }
        }

        *((uint64_t*)pvinto) = jj;
    }
};

struct BSQListTreeRepr
{
    void* l;
    void* r;
    uint32_t lcount;
    uint32_t color; //TODO: when we balance
};

std::string entityListTreeDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQListTreeType : public BSQListReprType
{
public:
    BSQListTreeType(BSQTypeID tid, std::string name, BSQTypeID entrytype)
    : BSQListReprType(tid, sizeof(BSQListTreeRepr), "22", entityListTreeDisplay_impl, name, entrytype, ListReprKind::TreeElement) 
    {;}

    virtual ~BSQListTreeType() {;}

    virtual uint64_t getCount(void* repr) const override final
    {
        return ((BSQListTreeRepr*)repr)->lcount;
    }
};

struct BSQListTypeFlavor
{
    const BSQTypeID ltype;

    const BSQType* entrytype;

    const BSQPartialVectorType* pv4type;
    const BSQPartialVectorType* pv8type;
    const BSQListTreeType* treetype;
};

class BSQListForwardIterator : public BSQCollectionIterator
{
public:
    int64_t curr;
    int64_t lmax;
    const BSQPartialVectorType* lctype;
    int16_t icurr;
    int16_t imax;

    BSQListForwardIterator(const BSQType* lreprtype, void* lroot): BSQCollectionIterator(), curr(0), lmax(0), lctype(nullptr), icurr(0), imax(0)
    {
        if(lroot != nullptr) 
        {
            this->lmax = (int64_t)dynamic_cast<const BSQListReprType*>(lreprtype)->getCount(lroot); 

            void* rr = lroot;
            const BSQListReprType* rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
            while(rt->lkind == ListReprKind::TreeElement)
            {
                this->iterstack.push_back(static_cast<BSQListTreeRepr*>(rr));

                rr = static_cast<BSQListTreeRepr*>(rr)->l;
                rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
            }

            this->lcurr = rr;
            this->lctype = static_cast<const BSQPartialVectorType*>(rt);

            this->icurr = 0;
            this->imax = BSQPartialVectorType::getPVCount(rr);
        }
    }
    
    virtual ~BSQListForwardIterator() {;}

    inline bool valid() const
    {
        return this->curr != this->lmax;
    }

    void advance_slow()
    {
        if(this->curr == this->lmax)
        {
            return;
        }

        void* rr = this->lcurr;
        while(static_cast<BSQListTreeRepr*>(this->iterstack.back())->r == rr)
        {
            rr = this->iterstack.back();
            this->iterstack.pop_back();
        }

        rr = static_cast<BSQListTreeRepr*>(this->iterstack.back())->r;
        const BSQListReprType* rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
        while(rt->lkind == ListReprKind::TreeElement)
        {
            this->iterstack.push_back(static_cast<BSQListTreeRepr*>(rr));

            rr = static_cast<BSQListTreeRepr*>(rr)->l;
            rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
        }

        this->lcurr = rr;
        this->lctype = static_cast<const BSQPartialVectorType*>(rt);

        this->icurr = 0;
        this->imax = BSQPartialVectorType::getPVCount(rr);
    }

    inline void advance()
    {
        assert(this->valid());

        this->curr++;
        this->icurr++;
        if(this->icurr >= this->imax)
        {
            this->advance_slow();
        }
    }

    inline StorageLocationPtr getlocation() const
    {
        assert(this->valid());

        return this->lctype->get(this->lcurr, this->icurr);
    }
};

class BSQListReverseIterator : public BSQCollectionIterator
{
public:
    int64_t curr;
    const BSQPartialVectorType* lctype;
    int16_t icurr;

    BSQListReverseIterator(const BSQType* lreprtype, void* lroot): BSQCollectionIterator(), curr(-1), lctype(nullptr), icurr(0)
    {
        if(lroot != nullptr) 
        {
            this->curr = (int64_t)dynamic_cast<const BSQListReprType*>(lreprtype)->getCount(lroot) - 1; 

            void* rr = lroot;
            const BSQListReprType* rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
            while(rt->lkind == ListReprKind::TreeElement)
            {
                this->iterstack.push_back(static_cast<BSQListTreeRepr*>(rr));

                rr = static_cast<BSQListTreeRepr*>(rr)->r;
                rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
            }

            this->lcurr = rr;
            this->lctype = static_cast<const BSQPartialVectorType*>(rt);

            this->icurr = BSQPartialVectorType::getPVCount(rr) - 1;
        }
    }
    
    virtual ~BSQListReverseIterator() {;}

    inline bool valid() const
    {
        return this->curr >= 0;
    }

    void advance_slow()
    {
        if(this->curr < 0)
        {
            return;
        }

        void* rr = this->lcurr;
        while(static_cast<BSQListTreeRepr*>(this->iterstack.back())->l == rr)
        {
            rr = this->iterstack.back();
            this->iterstack.pop_back();
        }

        rr = static_cast<BSQListTreeRepr*>(rr)->l;
        const BSQListReprType* rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
        while(rt->lkind == ListReprKind::TreeElement)
        {
            this->iterstack.push_back(static_cast<BSQListTreeRepr*>(rr));

            rr = static_cast<BSQListTreeRepr*>(rr)->r;
            rt = static_cast<const BSQListReprType*>(GET_TYPE_META_DATA(rr));
        }

        this->lcurr = rr;
        this->lctype = static_cast<const BSQPartialVectorType*>(rt);

        this->icurr = BSQPartialVectorType::getPVCount(rr) - 1;
    }

    inline void advance()
    {
        assert(this->valid());

        this->curr--;
        this->icurr--;
        if(this->icurr < 0)
        {
            this->advance_slow();
        }
    }

    inline StorageLocationPtr getlocation() const
    {
        assert(this->valid());

        return this->lctype->get(this->lcurr, this->icurr);
    }
};

class BSQListSpineIterator : public BSQCollectionIterator
{
public:
    BSQListSpineIterator(const BSQType* lreprtype, void* lroot): BSQCollectionIterator()
    {
        if(lroot != nullptr) 
        {
            this->lcurr = lroot;
        }
    }
    
    virtual ~BSQListSpineIterator() {;}

    inline bool valid() const
    {
        return this->lcurr != nullptr;
    }

    inline void moveLeft()
    {
        assert(this->valid());

        this->iterstack.push_back(this->lcurr);
        this->lcurr = static_cast<BSQListTreeRepr*>(this->lcurr)->l;
    }

    inline void moveRight()
    {
        assert(this->valid());

        this->iterstack.push_back(this->lcurr);
        this->lcurr = static_cast<BSQListTreeRepr*>(this->lcurr)->r;
    }

    inline void pop()
    {
        assert(this->valid());

        this->lcurr = this->iterstack.back();
        this->iterstack.pop_back();
    }
};

//List
class BSQListType : public BSQType
{
public:
    const BSQTypeID etype; //type of entries in the list

    BSQListType(BSQTypeID tid, DisplayFP fpDisplay, std::string name, BSQTypeID etype): 
        BSQType(tid, BSQTypeLayoutKind::Collection, {8, 8, 8, nullptr, "5"}, {gcProcessHeapOperator_collectionImpl, gcDecOperator_collectionImpl, gcEvacuateParentOperator_collectionImpl, gcEvacuateChildOperator_collectionImpl}, {}, EMPTY_KEY_CMP, fpDisplay, name),
        etype(etype)
    {;}

    virtual ~BSQListType() {;}

    void clearValue(StorageLocationPtr trgt) const override final
    {
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(trgt, nullptr);
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(trgt, SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(src));
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
        assert(false);
        return nullptr;
    }
};
