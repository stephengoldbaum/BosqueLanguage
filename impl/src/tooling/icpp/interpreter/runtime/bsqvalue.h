//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "../common.h"
#include "bsqmemory.h"

class BSQField
{
public:
    static const BSQField** g_fieldtable;

    const BSQFieldID fkey;
    const std::string fname;

    const BSQTypeID declaredType;
    const bool isOptional;

    BSQField(BSQFieldID fkey, std::string fname, BSQTypeID declaredType, bool isOptional): 
        fkey(fkey), fname(fname), declaredType(declaredType), isOptional(isOptional)
    {;}
};

class BSQWellKnownType
{
public:
    //Well known types
    static const BSQType* g_typeNone;
    static const BSQType* g_typeNothing;
    static const BSQType* g_typeBool;
    static const BSQType* g_typeNat;
    static const BSQType* g_typeInt;
    static const BSQType* g_typeBigNat;
    static const BSQType* g_typeBigInt;
    static const BSQType* g_typeFloat;
    static const BSQType* g_typeDecimal;
    static const BSQType* g_typeRational;

    static const BSQType* g_typeStringKRepr16;
    static const BSQType* g_typeStringKRepr32;
    static const BSQType* g_typeStringKRepr64;
    static const BSQType* g_typeStringKRepr96;
    static const BSQType* g_typeStringKRepr128;
    static const std::pair<size_t, const BSQType*> g_typeStringKCons[5];

    static const BSQType* g_typeStringTreeRepr;
    static const BSQType* g_typeStringSliceRepr;

    static const BSQType* g_typeString;

    static const BSQType* g_typeByteBufferLeaf;
    static const BSQType* g_typeByteBufferNode;
    static const BSQType* g_typeByteBuffer;
    static const BSQType* g_typeDateTime;
    static const BSQType* g_typeUTCDateTime;
    static const BSQType* g_typeCalendarDate;
    static const BSQType* g_typeRelativeTime;
    static const BSQType* g_typeTickTime;
    static const BSQType* g_typeLogicalTime;
    static const BSQType* g_typeISOTimeStamp;
    static const BSQType* g_typeUUID4;
    static const BSQType* g_typeUUID7;
    static const BSQType* g_typeSHAContentHash;
    static const BSQType* g_typeLatLongCoordinate;
    static const BSQType* g_typeRegex;
};

class BSQEmptyType : public BSQType
{
public:
    BSQEmptyType(BSQTypeID tid, KeyCmpFP fpkeycmp, DisplayFP fpDisplay, std::string name): 
        BSQType(tid, BSQTypeLayoutKind::Empty, { 0, 0, 0, nullptr, "" }, STRUCT_LEAF_GC_FUNCTOR_SET, {}, fpkeycmp, fpDisplay, name)
    {;}

    virtual ~BSQEmptyType() {;}

    void clearValue(StorageLocationPtr trgt) const override final
    {
        ;
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        ;
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
        assert(false);
        return nullptr;
    }
};

template <typename T>
class BSQRegisterType : public BSQType
{
public:
    BSQRegisterType(BSQTypeID tid, uint64_t datasize, const RefMask imask, KeyCmpFP fpkeycmp, DisplayFP fpDisplay, std::string name): 
        BSQType(tid, BSQTypeLayoutKind::Register, { BSQ_SIZE_ENSURE_ALIGN_MIN(datasize), BSQ_SIZE_ENSURE_ALIGN_MIN(datasize), datasize, nullptr, imask }, REGISTER_GC_FUNCTOR_SET, {}, fpkeycmp, fpDisplay, name)
    {;}

    virtual ~BSQRegisterType() {;}

    void storeValueDirect(StorageLocationPtr trgt, T v) const
    {
        SLPTR_STORE_CONTENTS_AS(T, trgt, v);
    }

    void clearValue(StorageLocationPtr trgt) const override final
    {
        GC_MEM_ZERO(trgt, sizeof(T));
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        SLPTR_STORE_CONTENTS_AS(T, trgt, SLPTR_LOAD_CONTENTS_AS(T, src));
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
        assert(false);
        return nullptr;
    }
};

class BSQEnumType : public BSQType
{
public:
    const std::vector<std::string> enumnames;

    BSQEnumType(BSQTypeID tid, KeyCmpFP fpkeycmp, DisplayFP fpDisplay, std::string name, const std::vector<std::string> enumnames): 
        BSQType(tid, BSQTypeLayoutKind::Register, { sizeof(uint64_t), sizeof(uint64_t), sizeof(uint64_t), nullptr, "1" }, REGISTER_GC_FUNCTOR_SET, {}, fpkeycmp, fpDisplay, name), enumnames(enumnames)
    {;}

    virtual ~BSQEnumType() {;}

    void storeValueDirect(StorageLocationPtr trgt, uint64_t v) const
    {
        SLPTR_STORE_CONTENTS_AS(uint64_t, trgt, v);
    }

    void clearValue(StorageLocationPtr trgt) const override final
    {
        GC_MEM_ZERO(trgt, sizeof(uint64_t));
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        SLPTR_STORE_CONTENTS_AS(uint64_t, trgt, SLPTR_LOAD_CONTENTS_AS(uint64_t, src));
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
        assert(false);
        return nullptr;
    }
};

class BSQRefType : public BSQType
{
public:
    BSQRefType(BSQTypeID tid, uint64_t heapsize, const RefMask heapmask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, KeyCmpFP fpkeycmp, DisplayFP fpDisplay, std::string name):  
        BSQType(tid, BSQTypeLayoutKind::Ref, { heapsize, sizeof(void*), sizeof(void*), heapmask, "2" }, REF_GC_FUNCTOR_SET, vtable, fpkeycmp, fpDisplay, name)
    {;}

    virtual ~BSQRefType() {;}

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
        return SLPTR_INDEX_DATAPTR(SLPTR_LOAD_HEAP_DATAPTR(src), offset);
    }
};

class BSQStructType : public BSQType
{
public:
    const BSQTypeID boxedtype; //value is 0 if this is not a boxable type

    BSQStructType(BSQTypeID tid, uint64_t datasize, const RefMask imask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, KeyCmpFP fpkeycmp, DisplayFP fpDisplay, std::string name, bool norefs, BSQTypeID boxedtype): 
        BSQType(tid, BSQTypeLayoutKind::Struct, { datasize, datasize, datasize, nullptr, imask }, norefs ? STRUCT_LEAF_GC_FUNCTOR_SET : STRUCT_STD_GC_FUNCTOR_SET, vtable, fpkeycmp, fpDisplay, name), boxedtype(boxedtype)
    {;}

    virtual ~BSQStructType() {;}

    void clearValue(StorageLocationPtr trgt) const override final
    {
        BSQ_MEM_ZERO(trgt, this->allocinfo.assigndatasize);
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        BSQ_MEM_COPY(trgt, src, this->allocinfo.assigndatasize);
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final 
    {
        return SLPTR_INDEX_DATAPTR(src, offset);
    }
};

class BSQBoxedStructType : public BSQType
{
public:
    const BSQStructType* oftype;

    BSQBoxedStructType(BSQTypeID tid, const BSQStructType* oftype, std::string name): 
        BSQType(tid, BSQTypeLayoutKind::BoxedStruct, { oftype->allocinfo.assigndatasize, sizeof(void*), sizeof(void*), oftype->allocinfo.inlinedmask, "2" }, REF_GC_FUNCTOR_SET, {}, EMPTY_KEY_CMP, oftype->fpDisplay, name)
    {;}

    virtual ~BSQBoxedStructType() {;}

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
        BSQ_INTERNAL_ASSERT(false);
        return nullptr;
    }
};

template <typename T>
class BSQBigNumType : public BSQType
{
public:
    BSQBigNumType(BSQTypeID tid, uint64_t datasize, const RefMask imask, KeyCmpFP fpkeycmp, DisplayFP fpDisplay, std::string name): 
        BSQType(tid, BSQTypeLayoutKind::BigNum, { datasize, datasize, datasize, nullptr, imask }, REGISTER_GC_FUNCTOR_SET, {}, fpkeycmp, fpDisplay, name)
    {;}

    virtual ~BSQBigNumType() {;}

    void storeValueDirect(StorageLocationPtr trgt, T v) const
    {
        SLPTR_STORE_CONTENTS_AS(T, trgt, v);
    }

    void clearValue(StorageLocationPtr trgt) const override final
    {
        GC_MEM_ZERO(trgt, sizeof(T));
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        SLPTR_STORE_CONTENTS_AS(T, trgt, SLPTR_LOAD_CONTENTS_AS(T, src));
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
        BSQ_INTERNAL_ASSERT(false);
        return nullptr;
    }
};

////
//Concrete types

std::string tupleDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQTupleInfo
{
public:
    BSQTupleIndex maxIndex;
    std::vector<BSQTypeID> ttypes;
    std::vector<size_t> idxoffsets;

    BSQTupleInfo(BSQTupleIndex maxIndex, std::vector<BSQTypeID> ttypes, std::vector<size_t> idxoffsets):
        maxIndex(maxIndex), ttypes(ttypes), idxoffsets(idxoffsets)
    {;}

    virtual ~BSQTupleInfo() {;}
};

class BSQTupleRefType : public BSQRefType, public BSQTupleInfo
{
public:
    BSQTupleRefType(BSQTypeID tid, uint64_t heapsize, const RefMask heapmask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, std::string name, BSQTupleIndex maxIndex, std::vector<BSQTypeID> ttypes, std::vector<size_t> idxoffsets):
        BSQRefType(tid, heapsize, heapmask, vtable, EMPTY_KEY_CMP, tupleDisplay_impl, name),
        BSQTupleInfo(maxIndex, ttypes, idxoffsets)
    {;}

    virtual ~BSQTupleRefType() {;}
};

class BSQTupleStructType : public BSQStructType, public BSQTupleInfo
{
public:
    BSQTupleStructType(BSQTypeID tid, uint64_t datasize, RefMask imask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, std::string name, bool norefs, BSQTypeID boxedtype, BSQTupleIndex maxIndex, std::vector<BSQTypeID> ttypes, std::vector<size_t> idxoffsets):
        BSQStructType(tid, datasize, imask, vtable, EMPTY_KEY_CMP, tupleDisplay_impl, name, norefs, boxedtype),
        BSQTupleInfo(maxIndex, ttypes, idxoffsets)
    {;}

    virtual ~BSQTupleStructType() {;}
};

std::string recordDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQRecordInfo
{
public:
    static std::map<BSQRecordPropertyID, std::string> g_propertynamemap;

    const std::vector<BSQRecordPropertyID> properties;
    const std::vector<BSQTypeID> rtypes;
    const std::vector<size_t> propertyoffsets;

    BSQRecordInfo(std::vector<BSQRecordPropertyID> properties, std::vector<BSQTypeID> rtypes, std::vector<size_t> propertyoffsets):
        properties(properties), rtypes(rtypes), propertyoffsets(propertyoffsets)
    {;}

    virtual ~BSQRecordInfo() {;}
};

class BSQRecordRefType : public BSQRefType, public BSQRecordInfo
{
public:
    BSQRecordRefType(BSQTypeID tid, uint64_t heapsize, const RefMask heapmask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, std::string name, std::vector<BSQRecordPropertyID> properties, std::vector<BSQTypeID> rtypes, std::vector<size_t> propertyoffsets):
        BSQRefType(tid, heapsize, heapmask, vtable, EMPTY_KEY_CMP, recordDisplay_impl, name),
        BSQRecordInfo(properties, rtypes, propertyoffsets)
    {;}

    virtual ~BSQRecordRefType() {;}
};

class BSQRecordStructType : public BSQStructType, public BSQRecordInfo
{
public:
    BSQRecordStructType(BSQTypeID tid, uint64_t datasize, const RefMask imask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, std::string name, bool norefs, BSQTypeID boxedtype, std::vector<BSQRecordPropertyID> properties, std::vector<BSQTypeID> rtypes, std::vector<size_t> propertyoffsets):
        BSQStructType(tid, datasize, imask, vtable, EMPTY_KEY_CMP, recordDisplay_impl, name, norefs, boxedtype),
        BSQRecordInfo(properties, rtypes, propertyoffsets)
    {;}

    virtual ~BSQRecordStructType() {;}
};

std::string entityDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQEntityInfo
{
public:
    const std::vector<BSQFieldID> fields;
    const std::vector<size_t> fieldoffsets;
    const std::vector<BSQTypeID> ftypes;

    BSQEntityInfo(std::vector<BSQFieldID> fields, std::vector<size_t> fieldoffsets, std::vector<BSQTypeID> ftypes):
        fields(fields), fieldoffsets(fieldoffsets), ftypes(ftypes)
    {;}

    virtual ~BSQEntityInfo() {;}
};

class BSQEntityRefType : public BSQRefType, public BSQEntityInfo
{
public:
    BSQEntityRefType(BSQTypeID tid, uint64_t heapsize, const RefMask heapmask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, std::string name, std::vector<BSQFieldID> fields, std::vector<size_t> fieldoffsets, std::vector<BSQTypeID> ftypes):
        BSQRefType(tid, heapsize, heapmask, vtable, EMPTY_KEY_CMP, entityDisplay_impl, name),
        BSQEntityInfo(fields, fieldoffsets, ftypes)
    {;}

    virtual ~BSQEntityRefType() {;}
};

class BSQEntityStructType : public BSQStructType, public BSQEntityInfo
{
public:
    BSQEntityStructType(BSQTypeID tid, uint64_t datasize, const RefMask imask, std::map<BSQVirtualInvokeID, BSQInvokeID> vtable, std::string name, bool norefs, BSQTypeID boxedtype, std::vector<BSQFieldID> fields, std::vector<size_t> fieldoffsets, std::vector<BSQTypeID> ftypes): 
        BSQStructType(tid, datasize, imask, vtable, EMPTY_KEY_CMP, entityDisplay_impl, name, norefs, boxedtype),
        BSQEntityInfo(fields, fieldoffsets, ftypes)
    {;}

    virtual ~BSQEntityStructType() {;}
};

//SOMETHING, RESULT_OK, and RESULT_ERR
std::string constructableEntityDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQConstructableEntityInfo
{
public:
    const BSQTypeID oftype;

    BSQConstructableEntityInfo(BSQTypeID oftype):
        oftype(oftype)
    {;}

    virtual ~BSQConstructableEntityInfo() {;}
};

class BSQEntityConstructableRefType : public BSQRefType, public BSQConstructableEntityInfo
{
public:
    BSQEntityConstructableRefType(BSQTypeID tid, uint64_t heapsize, const RefMask heapmask, std::string name, BSQTypeID oftype):
        BSQRefType(tid, heapsize, heapmask, {}, EMPTY_KEY_CMP, constructableEntityDisplay_impl, name), BSQConstructableEntityInfo(oftype)
    {;}

    virtual ~BSQEntityConstructableRefType() {;}
};

class BSQEntityConstructableStructType : public BSQStructType, public BSQConstructableEntityInfo
{
public:
    BSQEntityConstructableStructType(BSQTypeID tid, uint64_t datasize, const RefMask imask, std::string name, bool norefs, BSQTypeID boxedtype, BSQTypeID oftype): 
        BSQStructType(tid, datasize, imask, {}, EMPTY_KEY_CMP, constructableEntityDisplay_impl, name, norefs, boxedtype), BSQConstructableEntityInfo(oftype)
    {;}

    virtual ~BSQEntityConstructableStructType() {;}
};

std::string ephemeralDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQEphemeralListType : public BSQStructType
{
public:
    const std::vector<BSQTypeID> etypes;
    const std::vector<size_t> idxoffsets;

    BSQEphemeralListType(BSQTypeID tid, uint64_t datasize, const RefMask imask, std::string name, std::vector<BSQTypeID> etypes, std::vector<size_t> idxoffsets, bool norefs): 
        BSQStructType(tid, datasize, imask, {}, nullptr, ephemeralDisplay_impl, name, norefs, 0), etypes(etypes), idxoffsets(idxoffsets)
    {;}

    virtual ~BSQEphemeralListType() {;}
};

std::string globalObjectDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQGlobalObjectType : public BSQRefType
{
public:
    BSQGlobalObjectType(uint64_t heapsize, const RefMask heapmask, std::string name):
        BSQRefType(BSQ_TYPE_ID_INTERNAL, heapsize, heapmask, {}, EMPTY_KEY_CMP, globalObjectDisplay_impl, name)
    {;}

    virtual ~BSQGlobalObjectType() {;}
};

std::string unionDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int unionInlineKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);
int unionRefKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

class BSQUnionType : public BSQType
{
public:
    const std::vector<BSQTypeID> subtypes;

     BSQUnionType(BSQTypeID tid, BSQTypeLayoutKind tkind, BSQTypeSizeInfo allocinfo, KeyCmpFP fpkeycmp, std::string name, std::vector<BSQTypeID> subtypes): 
        BSQType(tid, tkind, allocinfo, {0}, {}, fpkeycmp, unionDisplay_impl, name), subtypes(subtypes)
    {;}

    virtual ~BSQUnionType() {;}

    virtual bool isUnion() const
    {
        return true;
    }

    virtual const BSQType* getVType(StorageLocationPtr trgt) const = 0;

    virtual StorageLocationPtr getVData_StorageLocation(StorageLocationPtr trgt) const = 0;
    virtual void* getVData_Direct(StorageLocationPtr trgt) const = 0;
};

class BSQUnionRefType : public BSQUnionType
{
public:
    BSQUnionRefType(BSQTypeID tid, std::string name, std::vector<BSQTypeID> subtypes): 
        BSQUnionType(tid, BSQTypeLayoutKind::UnionRef, { sizeof(void*), sizeof(void*), sizeof(void*), nullptr, "2" }, unionRefKeyCmp_impl, name, subtypes)
    {;}

    virtual ~BSQUnionRefType() {;}

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
        return SLPTR_INDEX_DATAPTR(SLPTR_LOAD_HEAP_DATAPTR(src), offset);
    }

    void coerceFromAtomic(const BSQType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        btype->storeValue(trgt, src);
    }

    void coerceFromUnionRef(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        this->storeValue(trgt, src);
    }

    void coerceFromUnionInline(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        this->storeValue(trgt, SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
    }

    void coerceFromUnionUniversal(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        this->storeValue(trgt, SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
    }

    void extractToAtomic(const BSQType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        btype->storeValue(trgt, src);
    }

    virtual const BSQType* getVType(StorageLocationPtr trgt) const override final
    {
        return SLPTR_LOAD_HEAP_TYPE(trgt);
    }

    virtual StorageLocationPtr getVData_StorageLocation(StorageLocationPtr trgt) const override final
    {
        return trgt;
    }

    virtual void* getVData_Direct(StorageLocationPtr trgt) const override final
    {
        return SLPTR_LOAD_HEAP_DATAPTR(trgt);
    }
};

class BSQUnionInlineType : public BSQUnionType
{
public:
    BSQUnionInlineType(BSQTypeID tid, uint64_t datasize, const RefMask imask, std::string name, std::vector<BSQTypeID> subtypes): 
        BSQUnionType(tid, BSQTypeLayoutKind::UnionInline, { datasize, datasize, datasize, nullptr, imask }, unionInlineKeyCmp_impl, name, subtypes)
    {;}

    virtual ~BSQUnionInlineType() {;}

    void clearValue(StorageLocationPtr trgt) const override final
    {
        GC_MEM_ZERO(trgt, this->allocinfo.assigndatasize);
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        BSQ_MEM_COPY(trgt, src, this->allocinfo.assigndatasize);
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
       return (SLPTR_LOAD_UNION_INLINE_TYPE(src))->indexStorageLocationOffset(SLPTR_LOAD_UNION_INLINE_DATAPTR(src), offset);
    }

    void coerceFromAtomic(const BSQType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        SLPTR_STORE_UNION_INLINE_TYPE(btype, trgt);
        btype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), src);
    }

    void coerceFromUnionRef(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        auto reprtype = SLPTR_LOAD_HEAP_TYPE(src);

        SLPTR_STORE_UNION_INLINE_TYPE(reprtype, trgt);
        reprtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), src);
    }

    void coerceFromUnionInline(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        auto reprtype = SLPTR_LOAD_UNION_INLINE_TYPE(src);

        SLPTR_STORE_UNION_INLINE_TYPE(reprtype, trgt);
        reprtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
    }

    void coerceFromUnionUniversal(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        auto reprtype = SLPTR_LOAD_UNION_INLINE_TYPE(src);
        if(reprtype->tkind != BSQTypeLayoutKind::BoxedStruct)
        {
            SLPTR_STORE_UNION_INLINE_TYPE(reprtype, trgt);
            reprtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
        }
        else
        {
            const BSQStructType* structtype = dynamic_cast<const BSQBoxedStructType*>(reprtype)->oftype;

            auto udata = SLPTR_LOAD_UNION_INLINE_DATAPTR(src);
            auto boxeddata = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(udata);

            SLPTR_STORE_UNION_INLINE_TYPE(structtype, trgt);
            structtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), boxeddata);
        }
    }

    void extractToAtomic(const BSQType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        btype->storeValue(trgt, SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
    }

    virtual const BSQType* getVType(StorageLocationPtr trgt) const override final
    {
        return SLPTR_LOAD_UNION_INLINE_TYPE(trgt);
    }

    virtual StorageLocationPtr getVData_StorageLocation(StorageLocationPtr trgt) const override final
    {
        return SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt);
    }

    virtual void* getVData_Direct(StorageLocationPtr trgt) const override final
    {
        auto oftype = SLPTR_LOAD_UNION_INLINE_TYPE(trgt);
        auto sl = SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt);

        if(oftype->tkind == BSQTypeLayoutKind::Struct)
        {
            return (void*)sl;
        }
        else
        {
            return SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(sl);
        }
    }
};

class BSQUnionUniversalType : public BSQUnionType
{
public:
    BSQUnionUniversalType(BSQTypeID tid, std::string name, std::vector<BSQTypeID> subtypes): 
        BSQUnionType(tid, BSQTypeLayoutKind::UnionUniversal, { UNION_UNIVERSAL_SIZE, UNION_UNIVERSAL_SIZE, UNION_UNIVERSAL_SIZE, nullptr, UNION_UNIVERSAL_MASK }, EMPTY_KEY_CMP, name, subtypes)
    {;}

    virtual ~BSQUnionUniversalType() {;}

    void clearValue(StorageLocationPtr trgt) const override final
    {
        GC_MEM_ZERO(trgt, this->allocinfo.assigndatasize);
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        BSQ_MEM_COPY(trgt, src, this->allocinfo.assigndatasize);
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
       return (SLPTR_LOAD_UNION_INLINE_TYPE(src))->indexStorageLocationOffset(SLPTR_LOAD_UNION_INLINE_DATAPTR(src), offset);
    }

    void coerceFromAtomic(const BSQType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        if(btype->tkind != BSQTypeLayoutKind::Struct)
        {
            SLPTR_STORE_UNION_INLINE_TYPE(btype, trgt);
            btype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), src);
        }
        else
        {
            auto structbtype = dynamic_cast<const BSQStructType*>(btype);
            if(structbtype == nullptr || structbtype->boxedtype == 0)
            {
                SLPTR_STORE_UNION_INLINE_TYPE(btype, trgt);
                btype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), src);
            }
            else
            {
                const BSQBoxedStructType* boxedtype = dynamic_cast<const BSQBoxedStructType*>(BSQType::g_typetable[structbtype->boxedtype]);
                
                auto obj = Allocator::GlobalAllocator.allocateDynamic(boxedtype);
                btype->storeValue(obj, src);

                SLPTR_STORE_UNION_INLINE_TYPE(boxedtype, trgt);
                boxedtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), obj);
            }
        }
    }

    void coerceFromUnionRef(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        auto reprtype = SLPTR_LOAD_HEAP_TYPE(src);

        SLPTR_STORE_UNION_INLINE_TYPE(reprtype, trgt);
        reprtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), src);
    }

    void coerceFromUnionInline(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        auto reprtype = SLPTR_LOAD_UNION_INLINE_TYPE(src);
        if(reprtype->tkind != BSQTypeLayoutKind::Struct)
        {
            SLPTR_STORE_UNION_INLINE_TYPE(reprtype, trgt);
            reprtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
        }
        else
        {
            auto structrtype = dynamic_cast<const BSQStructType*>(reprtype);
            if(structrtype == nullptr || structrtype->boxedtype == 0)
            {
                SLPTR_STORE_UNION_INLINE_TYPE(reprtype, trgt);
                reprtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
            }
            else
            {
                const BSQBoxedStructType* boxedtype = dynamic_cast<const BSQBoxedStructType*>(BSQType::g_typetable[structrtype->boxedtype]);

                auto obj = Allocator::GlobalAllocator.allocateDynamic(boxedtype);
                reprtype->storeValue(obj, SLPTR_LOAD_UNION_INLINE_DATAPTR(src));

                SLPTR_STORE_UNION_INLINE_TYPE(boxedtype, trgt);
                boxedtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), obj);
            }
        }
    }

    void coerceFromUnionUniversal(const BSQUnionType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        auto reprtype = SLPTR_LOAD_UNION_INLINE_TYPE(src);

        SLPTR_STORE_UNION_INLINE_TYPE(reprtype, trgt);
        reprtype->storeValue(SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt), SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
    }

    void extractToAtomic(const BSQType* btype, StorageLocationPtr trgt, StorageLocationPtr src) const
    {
        if(btype->tkind != BSQTypeLayoutKind::Struct)
        {
            btype->storeValue(trgt, SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
        }
        else
        {
            const BSQBoxedStructType* boxedtype = dynamic_cast<const BSQBoxedStructType*>(BSQType::g_typetable[dynamic_cast<const BSQStructType*>(btype)->boxedtype]);
            if(boxedtype == nullptr)
            {
                btype->storeValue(trgt, SLPTR_LOAD_UNION_INLINE_DATAPTR(src));
            }
            else
            {
                auto udata = SLPTR_LOAD_UNION_INLINE_DATAPTR(src);
                auto boxeddata = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(udata);

                btype->storeValue(trgt, boxeddata);
            }
        }
    }

    virtual const BSQType* getVType(StorageLocationPtr trgt) const override final
    {
        auto btype = SLPTR_LOAD_UNION_INLINE_TYPE(trgt);
        if(btype->tkind != BSQTypeLayoutKind::BoxedStruct)
        {
            return btype;
        }
        else
        {
            return dynamic_cast<const BSQBoxedStructType*>(btype)->oftype;
        }
    }

    virtual StorageLocationPtr getVData_StorageLocation(StorageLocationPtr trgt) const override final
    {
        auto btype = SLPTR_LOAD_UNION_INLINE_TYPE(trgt);
        if(btype->tkind != BSQTypeLayoutKind::BoxedStruct)
        {
            return SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt);
        }
        else
        {
            auto udata = SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt);
            return SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(udata);
        }
    }

    virtual void* getVData_Direct(StorageLocationPtr trgt) const override final
    {
        auto oftype = SLPTR_LOAD_UNION_INLINE_TYPE(trgt);
        auto sl = SLPTR_LOAD_UNION_INLINE_DATAPTR(trgt);

        if(oftype->tkind == BSQTypeLayoutKind::Struct)
        {
            return (void*)sl;
        }
        else
        {
            return (void*)SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(sl);
        }
    }
};

void coerce(const BSQType* from, const BSQType* into, StorageLocationPtr trgt, StorageLocationPtr src);
std::pair<const BSQType*, StorageLocationPtr> extractFromUnionVCall(const BSQUnionType* fromlayout, const BSQType* intoflow, StorageLocationPtr trgt, StorageLocationPtr src);

////
//Primitive value representations

////
//None
std::string entityNoneDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityNoneKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_NONE_TYPE() (new BSQEmptyType(BSQ_TYPE_ID_NONE, entityNoneKeyCmp_impl, entityNoneDisplay_impl, "None"))

////
//Nothing
std::string entityNothingDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_NOTHING_TYPE() (new BSQEmptyType(BSQ_TYPE_ID_NOTHING, EMPTY_KEY_CMP, entityNothingDisplay_impl, "Nothing"))

////
//Bool

typedef uint8_t BSQBool;
#define BSQTRUE ((BSQBool)1)
#define BSQFALSE ((BSQBool)0)

std::string entityBoolDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityBoolKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_BOOL_TYPE() (new BSQRegisterType<BSQBool>(BSQ_TYPE_ID_BOOL, sizeof(BSQBool), "1", entityBoolKeyCmp_impl, entityBoolDisplay_impl, "Bool"))

////
//Nat
typedef uint64_t BSQNat;

std::string entityNatDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityNatKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_NAT_TYPE(TID, NAME) (new BSQRegisterType<BSQNat>(TID, sizeof(BSQNat), "1", entityNatKeyCmp_impl, entityNatDisplay_impl, NAME))
    
////
//Int
typedef int64_t BSQInt;

std::string entityIntDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityIntKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_INT_TYPE(TID, NAME) (new BSQRegisterType<BSQInt>(TID, sizeof(BSQInt), "1", entityIntKeyCmp_impl, entityIntDisplay_impl, NAME))

////
//BigNat
typedef uint64_t BSQBigNat;

//
//TODO: change impl to something like for strings (an inline verison with maybe 128 or 196 bits + some heap allocated support)
//

std::string entityBigNatDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityBigNatKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_BIG_NAT_TYPE(TID, NAME) (new BSQBigNumType<BSQBigNat>(TID, sizeof(BSQBigNat), "4", entityBigNatKeyCmp_impl, entityBigNatDisplay_impl, NAME))

////
//BigInt
typedef int64_t BSQBigInt;

//
//TODO: change impl to something like for strings (an inline verison with maybe 128 or 196 bits + some heap allocated support)
//

std::string entityBigIntDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityBigIntKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_BIG_INT_TYPE(TID, NAME) (new BSQBigNumType<BSQBigInt>(TID, sizeof(BSQBigInt), "4", entityBigIntKeyCmp_impl, entityBigIntDisplay_impl, NAME))

////
//Float
typedef double BSQFloat;

std::string entityFloatDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_FLOAT_TYPE(TID, NAME) (new BSQRegisterType<BSQFloat>(TID, sizeof(BSQFloat), "1", EMPTY_KEY_CMP, entityFloatDisplay_impl, NAME))

////
//Decimal
typedef double BSQDecimal;

//
//TODO: we need to get a better library later maybe use something like -- /https://github.com/dotnet/runtime/blob/main/src/libraries/System.Private.CoreLib/src/System/Decimal.cs
//

std::string entityDecimalDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_DECIMAL_TYPE(TID, NAME) (new BSQRegisterType<BSQDecimal>(TID, sizeof(BSQDecimal), "1", EMPTY_KEY_CMP, entityDecimalDisplay_impl, NAME))

////
//Rational
struct BSQRational
{
    BSQBigInt numerator;
    BSQNat denominator;
};

std::string entityRationalDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_RATIONAL_TYPE(TID, NAME) (new BSQBigNumType<BSQRational>(TID, sizeof(BSQRational), "41", EMPTY_KEY_CMP, entityRationalDisplay_impl, NAME))

////
//String
struct BSQInlineString
{
    uint8_t utf8bytes[16];

    inline static BSQInlineString create(const uint8_t* chars, uint64_t len)
    {
        BSQInlineString istr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (uint8_t)len};
        assert(IS_INLINE_STRING(&istr));

        std::copy(chars, chars + len, istr.utf8bytes);
        return istr;
    }

    inline static uint64_t utf8ByteCount(const BSQInlineString& istr)
    {
        return istr.utf8bytes[15];
    }

    static void utf8ByteCount_Initialize(BSQInlineString& istr, uint64_t len)
    {
        istr.utf8bytes[15] = (uint8_t)len;
    }

    inline static uint8_t* utf8Bytes(BSQInlineString& istr)
    {
        return istr.utf8bytes;
    }

    inline static const uint8_t* utf8Bytes(const BSQInlineString& istr)
    {
        return istr.utf8bytes;
    }

    inline static uint8_t* utf8BytesEnd(BSQInlineString& istr)
    {
        return istr.utf8bytes + istr.utf8bytes[15];
    }

    inline static const uint8_t* utf8BytesEnd(const BSQInlineString& istr)
    {
        return istr.utf8bytes + istr.utf8bytes[15];
    }
};
constexpr BSQInlineString g_emptyInlineString = {0};

std::string entityStringReprDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

class BSQStringReprType : public BSQRefType
{
public:
    BSQStringReprType(BSQTypeID tid, uint64_t allocsize, RefMask heapmask, std::string name):
        BSQRefType(tid, allocsize, heapmask, {}, EMPTY_KEY_CMP, entityStringReprDisplay_impl, name) 
    {;}

    virtual ~BSQStringReprType() {;}

    virtual bool isKReprNode() const = 0;

    virtual uint64_t utf8ByteCount(void* repr) const = 0;
    virtual void* slice(void* data, uint64_t nstart, uint64_t nend) const = 0;
};

class BSQStringKReprTypeAbstract : public BSQStringReprType
{
public:
    BSQStringKReprTypeAbstract(BSQTypeID tid, uint64_t allocsize, std::string name) 
    : BSQStringReprType(tid, allocsize, nullptr, name) 
    {;}

    virtual ~BSQStringKReprTypeAbstract() {;}

    virtual bool isKReprNode() const override final { return true; }

    static uint64_t getUTF8ByteCount(void* repr)
    {
        return *((uint8_t*)repr);
    }

    static uint8_t* getUTF8Bytes(void* repr)
    {
        return ((uint8_t*)repr) + 1;
    }

    virtual uint64_t utf8ByteCount(void* repr) const override
    {
        return BSQStringKReprTypeAbstract::getUTF8ByteCount(repr);
    }

    inline static const BSQStringKReprTypeAbstract* selectKReprForSize(size_t k)
    {
        auto stp = std::find_if(BSQWellKnownType::g_typeStringKCons, BSQWellKnownType::g_typeStringKCons + sizeof(BSQWellKnownType::g_typeStringKCons), [&k](const std::pair<size_t, const BSQType*>& cc) {
            return cc.first > k;
        });
    
        assert(stp != BSQWellKnownType::g_typeStringKCons + sizeof(BSQWellKnownType::g_typeStringKCons));
        return static_cast<const BSQStringKReprTypeAbstract*>(stp->second);
    }

    virtual void* slice(void* data, uint64_t nstart, uint64_t nend) const override;
};

template <uint64_t k>
class BSQStringKReprType : public BSQStringKReprTypeAbstract
{
public:
    BSQStringKReprType(): BSQStringKReprTypeAbstract(BSQ_TYPE_ID_INTERNAL, k, "[Internal::StringKRepr]") 
    {;}

    virtual ~BSQStringKReprType() {;}
};

struct BSQStringTreeRepr
{
    void* srepr1;
    void* srepr2;
    uint64_t size;
    //TODO: stash perfect hashing bit if in tree to allow for fast order compare on strings! Maybe de-dup on GC in this tree too and use as Key value.
    //Then we need to sweep this tree periodically for release OR do some special case for count 1 and a string check
};

class BSQStringTreeReprType : public BSQStringReprType
{
public:
    BSQStringTreeReprType(): BSQStringReprType(BSQ_TYPE_ID_INTERNAL, sizeof(BSQStringTreeRepr), "22", "[Internal::StringConcatRepr]") 
    {;}

    virtual ~BSQStringTreeReprType() {;}

    virtual bool isKReprNode() const override final { return false; }

    uint64_t utf8ByteCount(void* repr) const override final
    {
        return ((BSQStringTreeRepr*)repr)->size;
    }

    virtual void* slice(void* data, uint64_t nstart, uint64_t nend) const override;
};

struct BSQString
{
    //TODO: should we make the reprs use this instead of void* -- makes a tree node can store much more 
    //      also we can then use this as a place to stash a pointer for the perfect tree hash for less compare and GC de-dupe
    union { void* u_data; BSQInlineString u_inlineString; };
};
constexpr BSQString g_emptyString = {0};

class BSQStringForwardIterator : public CharCodeIterator
{
private:
    void initializeIteratorPosition(int64_t curr);

    void increment_utf8byte();

public:
    BSQString* sstr;
    size_t curr;
    size_t strmax;
    uint8_t* cbuff;
    uint16_t cpos;
    uint16_t maxpos;

    BSQStringForwardIterator(BSQString* sstr, int64_t curr) : CharCodeIterator(), sstr(sstr), curr(curr), strmax(0), cbuff(nullptr), cpos(0), maxpos(0) 
    {
        if(IS_INLINE_STRING(sstr))
        {
            this->strmax = BSQInlineString::utf8ByteCount(sstr->u_inlineString);
        }
        else
        {
            this->strmax = GET_TYPE_META_DATA_AS(BSQStringReprType, sstr->u_data)->utf8ByteCount(sstr->u_data);
        }

        this->initializeIteratorPosition(curr);
    }
    
    virtual ~BSQStringForwardIterator() {;}

    virtual bool valid() const override final
    {
        return this->curr != this->strmax;
    }

    virtual void advance() override final
    {
        assert(this->valid());

        auto utfbyte = this->cbuff[this->cpos];
        if((utfbyte & (uint8_t)0x80) == 0)
        {
            this->increment_utf8byte();
        }
        else
        {
            //not implemented
            assert(false);
        }
    }

    virtual CharCode get() const override final
    {
        assert(this->valid());

        auto utfbyte = this->cbuff[this->cpos];
        if((utfbyte & (uint8_t)0x80) == 0)
        {
            return (CharCode)utfbyte;
        }
        else
        {
            //not implemented
            assert(false);
            return (CharCode)0;
        }
    }

    virtual size_t distance() const override final
    {
        return this->curr;
    }

    virtual void resetTo(size_t distance) override final
    {
        this->initializeIteratorPosition(distance);
    }

    void advance_byte()
    {
        assert(this->valid());
        this->increment_utf8byte();
    } 

    uint8_t get_byte() const
    {
        assert(this->valid());
        return this->cbuff[this->cpos];
    }
};

class BSQStringReverseIterator : public CharCodeIterator
{
private:
    void initializeIteratorPosition(int64_t curr);
    
    void increment_utf8byte();
public:
    BSQString* sstr;
    int64_t curr;
    int64_t strmax;
    uint8_t* cbuff;
    int16_t cpos;

    BSQStringReverseIterator(BSQString* sstr, int64_t curr) : CharCodeIterator(), sstr(sstr), curr(curr), strmax(0), cbuff(nullptr), cpos(0) 
    {
        if(IS_INLINE_STRING(sstr))
        {
            this->strmax = (int64_t)BSQInlineString::utf8ByteCount(sstr->u_inlineString);
        }
        else
        {
            this->strmax = (int64_t)GET_TYPE_META_DATA_AS(BSQStringReprType, sstr->u_data)->utf8ByteCount(sstr->u_data);
        }

        this->initializeIteratorPosition(curr);
        if(curr == strmax - 1)
        {
            auto utfbyte = this->cbuff[this->cpos];
            if((utfbyte & (uint8_t)0x80) == (uint8_t)0x80)
            {
                //not implemented
                assert(false);
            }
        }
    }

    virtual ~BSQStringReverseIterator() {;}

    virtual bool valid() const override final
    {
        return this->curr != -1;
    }

    virtual void advance() override final
    {
        assert(this->valid());
        this->increment_utf8byte();

        if(this->valid())
        {
            auto utfbyte = this->cbuff[this->cpos];
            if((utfbyte & (uint8_t)0x80) == (uint8_t)0x80)
            {
                //not implemented
                assert(false);
            }
        }
    }

    virtual CharCode get() const override final
    {
        assert(this->valid());

        auto utfbyte = this->cbuff[this->cpos];
        if((utfbyte & (uint8_t)0x80) == 0)
        {
            return (CharCode)utfbyte;
        }
        else
        {
            //not implemented
            assert(false);
        }
    }

    virtual size_t distance() const override final
    {
        return this->strmax - (this->curr + 1);
    }

    virtual void resetTo(size_t distance) override final
    {
        this->initializeIteratorPosition(this->strmax - (distance + 1));
    }

    void advance_byte()
    {
        assert(this->valid());
        this->increment_utf8byte();
    } 

    uint8_t get_byte() const
    {
        assert(this->valid());
        return this->cbuff[this->cpos];
    } 
};

std::string entityStringDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityStringKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

class BSQStringImplType : public BSQType
{
private:
    static uint8_t* boxInlineString(BSQInlineString istr);

public:
    BSQStringImplType(BSQTypeID tid, std::string name) 
    : BSQType(tid, BSQTypeLayoutKind::String, {sizeof(BSQString), sizeof(BSQString), sizeof(BSQString), "31", "31"}, { gcProcessHeapOperator_stringImpl, gcDecOperator_stringImpl, gcEvacuateParentOperator_stringImpl, gcEvacuateChildOperator_stringImpl  }, {}, entityStringKeyCmp_impl, entityStringDisplay_impl, name)
    {
        static_assert(sizeof(BSQString) == 16);
    }

    virtual ~BSQStringImplType() {;}

    void storeValueDirect(StorageLocationPtr trgt, BSQString s) const
    {
        SLPTR_STORE_CONTENTS_AS(BSQString, trgt, s);
    }

    void clearValue(StorageLocationPtr trgt) const override final
    {
        SLPTR_STORE_CONTENTS_AS(BSQString, trgt, {0});
    }

    void storeValue(StorageLocationPtr trgt, StorageLocationPtr src) const override final
    {
        SLPTR_STORE_CONTENTS_AS(BSQString, trgt, SLPTR_LOAD_CONTENTS_AS(BSQString, src));
    }

    StorageLocationPtr indexStorageLocationOffset(StorageLocationPtr src, size_t offset) const override final
    {
        assert(false);
        return nullptr;
    }

    static int keycmp(BSQString v1, BSQString v2);

    inline static int64_t utf8ByteCount(const BSQString& s)
    {
        if(IS_INLINE_STRING(&s))
        {
            return (int64_t)BSQInlineString::utf8ByteCount(s.u_inlineString);
        }
        else
        {
            return (int64_t)GET_TYPE_META_DATA_AS(BSQStringReprType, s.u_data)->utf8ByteCount(s.u_data);
        }
    }

    static inline BSQBool empty(const BSQString& s)
    {
        return (BSQBool)(s.u_data == nullptr);
    }

    static BSQString concat2(StorageLocationPtr s1, StorageLocationPtr s2);
    static BSQString slice(StorageLocationPtr str, int64_t startpos, int64_t endpos);
};

#define CONS_BSQ_STRING_TYPE(TID, NAME) (new BSQStringImplType(TID, NAME))

////
//ByteBuffer
struct BSQByteBufferLeaf
{
    uint8_t bytes[256];
};

struct BSQByteBufferNode
{
    BSQByteBufferNode* next;
    BSQByteBufferLeaf* bytes;
    uint64_t bytecount;
};

enum class BufferFormat {
    utf8 = 0x0,
    binary,
    bosque,
    json
};

enum class BufferCompression {
    off = 0x0,
    deflate
};

struct BSQByteBuffer
{
    BSQByteBufferNode* bytes;
    uint64_t bytecount;
    BufferFormat format;
    BufferCompression compression;
};

std::string entityByteBufferLeafDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
std::string entityByteBufferNodeDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
std::string entityByteBufferDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_BYTE_BUFFER_LEAF_TYPE() (new BSQRefType(BSQ_TYPE_ID_INTERNAL, sizeof(BSQByteBufferLeaf), nullptr, {}, EMPTY_KEY_CMP, entityByteBufferLeafDisplay_impl, "ByteBufferLeaf"))
#define CONS_BSQ_BYTE_BUFFER_NODE_TYPE() (new BSQRefType(BSQ_TYPE_ID_INTERNAL, sizeof(BSQByteBufferNode), "22", {}, EMPTY_KEY_CMP, entityByteBufferNodeDisplay_impl, "ByteBufferNode"))
#define CONS_BSQ_BYTE_BUFFER_TYPE(TID, NAME) (new BSQRefType(TID, sizeof(BSQByteBuffer), "2", {}, EMPTY_KEY_CMP, entityByteBufferDisplay_impl, NAME))

////
//DateTime

struct BSQDateTime
{
    uint16_t year;   // Year since 1900
    uint8_t month;   // 0-11
    uint8_t day;     // 1-31
    uint8_t hour;    // 0-23
    uint8_t min;     // 0-59

    uint16_t padding;

    const char* tzdata; //timezone name in tzdata database
};

std::string entityDateTimeDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_DATE_TIME_TYPE(TID, NAME) (new BSQRegisterType<BSQDateTime>(TID, sizeof(BSQDateTime), "11", EMPTY_KEY_CMP, entityDateTimeDisplay_impl, NAME))

////
//UTCDateTime

struct BSQUTCDateTime
{
    uint16_t year;   // Year since 1900
    uint8_t month;   // 0-11
    uint8_t day;     // 1-31
    uint8_t hour;    // 0-23
    uint8_t min;     // 0-59

    uint16_t padding;
};

std::string entityUTCDateTimeDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityUTCDateTimeKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_UTC_DATE_TIME_TYPE(TID, NAME) (new BSQRegisterType<BSQUTCDateTime>(TID, sizeof(BSQUTCDateTime), "1", entityUTCDateTimeKeyCmp_impl, entityUTCDateTimeDisplay_impl, NAME))

////
//CalendarDate

struct BSQCalendarDate
{
    uint16_t year;   // Year since 1900
    uint8_t month;   // 0-11
    uint8_t day;     // 1-31

    uint32_t padding;
};

std::string entityCalendarDateDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityCalendarDateKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_CALENDAR_DATE_TYPE(TID, NAME) (new BSQRegisterType<BSQCalendarDate>(TID, sizeof(BSQCalendarDate), "1", entityCalendarDateKeyCmp_impl, entityCalendarDateDisplay_impl, NAME))

////
//UTCDateTime

struct BSQRelativeTime
{
    uint8_t hour;    // 0-23
    uint8_t min;     // 0-59

    uint16_t padding;
    uint32_t padding2;
};

std::string entityRelativeTimeDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityRelativeTimeKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_RELATIVE_TIME_TYPE(TID, NAME) (new BSQRegisterType<BSQRelativeTime>(TID, sizeof(BSQRelativeTime), "1", entityRelativeTimeKeyCmp_impl, entityRelativeTimeDisplay_impl, NAME))

////
//TickTime
typedef uint64_t BSQTickTime;

std::string entityTickTimeDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityTickTimeKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_TICK_TIME_TYPE(TID, NAME) (new BSQRegisterType<BSQTickTime>(TID, sizeof(BSQTickTime), "1", entityTickTimeKeyCmp_impl, entityTickTimeDisplay_impl, NAME))

////
//LogicalTime
typedef uint64_t BSQLogicalTime;

std::string entityLogicalTimeDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityLogicalTimeKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_LOGICAL_TIME_TYPE(TID, NAME) (new BSQRegisterType<BSQTickTime>(TID, sizeof(BSQLogicalTime), "1", entityLogicalTimeKeyCmp_impl, entityLogicalTimeDisplay_impl, NAME))

////
//ISOTimeStamp

struct BSQISOTimeStamp
{
    uint16_t year;   // Year since 1900
    uint8_t month;   // 0-11
    uint8_t day;     // 1-31
    uint8_t hour;    // 0-23
    uint8_t min;     // 0-59

    uint16_t seconds; //0-61
    uint16_t millis; //0-999

    int16_t padding1;
    uint32_t padding2;
};

std::string entityISOTimeStampDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityISOTimeStampKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_ISO_TIME_STAMP_TYPE(TID, NAME) (new BSQRegisterType<BSQISOTimeStamp>(TID, sizeof(BSQISOTimeStamp), "11", entityISOTimeStampKeyCmp_impl, entityISOTimeStampDisplay_impl, NAME))

////
//UUID
typedef struct { uint8_t bytes[16]; } BSQUUID;

std::string entityUUIDDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entityUUIDKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_UUID4_TYPE(TID, NAME) (new BSQRegisterType<BSQUUID>(TID, sizeof(BSQUUID), "11", entityUUIDKeyCmp_impl, entityUUIDDisplay_impl, NAME))
#define CONS_BSQ_UUID7_TYPE(TID, NAME) (new BSQRegisterType<BSQUUID>(TID, sizeof(BSQUUID), "11", entityUUIDKeyCmp_impl, entityUUIDDisplay_impl, NAME))

////
//ContentHash
typedef struct { uint8_t bytes[64]; } BSQSHAContentHash;

std::string entitySHAContentHashDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);
int entitySHAContentHashKeyCmp_impl(const BSQType* btype, StorageLocationPtr data1, StorageLocationPtr data2);

#define CONS_BSQ_SHA_CONTENT_HASH_TYPE(TID, NAME) (new BSQRefType(TID, sizeof(BSQSHAContentHash), nullptr, {}, entitySHAContentHashKeyCmp_impl, entitySHAContentHashDisplay_impl, NAME))

////
//GeoCoordinate

struct BSQLatLongCoordinate
{
    float latitude;
    float longitude;
};

std::string entityLatLongCoordinateDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_LAT_LONG_COORDINATE_TYPE(TID, NAME) (new BSQRegisterType<BSQLatLongCoordinate>(TID, sizeof(BSQLatLongCoordinate), "1", EMPTY_KEY_CMP, entityLatLongCoordinateDisplay_impl, NAME))

////
//Regex

std::string entityRegexDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_REGEX_TYPE() (new BSQRegisterType<void*>(BSQ_TYPE_ID_REGEX, sizeof(void*), "1", EMPTY_KEY_CMP, entityRegexDisplay_impl, "Regex"))

////
//ENUM

std::string entityEnumDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_ENUM_TYPE(TID, NAME, ENAMES) (new BSQEnumType(TID, entityNatKeyCmp_impl, entityEnumDisplay_impl, NAME, ENAMES))

////
//List

std::string entityListDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_LIST_TYPE(TID, NAME, CTYPE) (new BSQListType(TID, entityListDisplay_impl, NAME, CTYPE))

#define CONS_BSQ_PARTIAL_VECTOR_TYPE(TID, HEAP_SIZE, HEAP_MASK, NAME, ELEMTYPE, ELEMSIZE, PVTAG) (new BSQPartialVectorType(TID, HEAP_SIZE, HEAP_MASK, NAME, ELEMTYPE, PVTAG, ELEMSIZE))
#define CONS_BSQ_TREE_LIST_TYPE(TID, NAME, ELEMTYPE) (new BSQListTreeType(TID, NAME, ELEMTYPE))

////
//Map

std::string entityMapDisplay_impl(const BSQType* btype, StorageLocationPtr data, DisplayMode mode);

#define CONS_BSQ_MAP_TYPE(TID, NAME, KTYPE, VTYPE) (new BSQMapType(TID, entityMapDisplay_impl, NAME, KTYPE, VTYPE))
#define CONS_BSQ_MAP_TREE_TYPE(TID, HEAP_SIZE, HEAP_MASK, NAME, KTYPE, KOFFSET, VTYPE, VOFFSET) (new BSQMapTreeType(TID, HEAP_SIZE, HEAP_MASK, NAME, KTYPE, KOFFSET, VTYPE, VOFFSET))
