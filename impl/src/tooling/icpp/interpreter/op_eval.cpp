//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "op_eval.h"

//
//TODO: win32 add checked arith
//
//https://github.com/dcleblanc/SafeInt

#ifdef _WIN32
#define PrimitiveNegateOperatorMacroChecked(THIS, OP, TAG, REPRTYPE, ERROR) const PrimitiveNegateOperatorOp<TAG>* bop = static_cast<const PrimitiveNegateOperatorOp<TAG>*>(op); \
REPRTYPE res = -(SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->arg))); \
\
SLPTR_STORE_CONTENTS_AS(REPRTYPE, THIS->evalTargetVar(bop->trgt), res);

//Big Macro for generating code for primitive checked binary operations
#define PrimitiveBinaryOperatorMacroChecked(THIS, OP, TAG, REPRTYPE, OPERATORW, OPERATORP, ERROR) const PrimitiveBinaryOperatorOp<TAG>* bop = static_cast<const PrimitiveBinaryOperatorOp<TAG>*>(op); \
REPRTYPE res = SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->larg)) OPERATORW SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->rarg)); \
\
SLPTR_STORE_CONTENTS_AS(REPRTYPE, THIS->evalTargetVar(bop->trgt), res);
#else
///Big Macro for generating code for primitive checked negate operations
#define PrimitiveNegateOperatorMacroChecked(THIS, OP, TAG, REPRTYPE, ERROR) const PrimitiveNegateOperatorOp<TAG>* bop = static_cast<const PrimitiveNegateOperatorOp<TAG>*>(op); \
REPRTYPE res; \
bool err = __builtin_mul_overflow(SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->arg)), -1, &res); \
BSQ_LANGUAGE_ASSERT(!err, &(THIS->cframe->invoke->srcFile), THIS->cframe->dbg_currentline, ERROR); \
\
SLPTR_STORE_CONTENTS_AS(REPRTYPE, THIS->evalTargetVar(bop->trgt), res);

//Big Macro for generating code for primitive checked binary operations
#define PrimitiveBinaryOperatorMacroChecked(THIS, OP, TAG, REPRTYPE, OPERATORW, OPERATORP, ERROR) const PrimitiveBinaryOperatorOp<TAG>* bop = static_cast<const PrimitiveBinaryOperatorOp<TAG>*>(op); \
REPRTYPE res; \
bool err = OPERATORP(SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->larg)), SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->rarg)), &res); \
BSQ_LANGUAGE_ASSERT(!err, &(THIS->cframe->invoke->srcFile), THIS->cframe->dbg_currentline, ERROR); \
\
SLPTR_STORE_CONTENTS_AS(REPRTYPE, THIS->evalTargetVar(bop->trgt), res);
#endif

//Big Macro for generating code for primitive un-checked negate operations
#define PrimitiveNegateOperatorMacroSafe(THIS, OP, TAG, REPRTYPE) const PrimitiveNegateOperatorOp<TAG>* bop = static_cast<const PrimitiveNegateOperatorOp<TAG>*>(op); \
SLPTR_STORE_CONTENTS_AS(REPRTYPE, THIS->evalTargetVar(bop->trgt), -SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->arg)));

//Big Macro for generating code for primitive checked rhsarg is non-zero binary operations
#define PrimitiveBinaryOperatorMacroCheckedDiv(THIS, OP, TAG, REPRTYPE) const PrimitiveBinaryOperatorOp<TAG>* bop = static_cast<const PrimitiveBinaryOperatorOp<TAG>*>(op); \
REPRTYPE larg = SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->larg)); \
REPRTYPE rarg = SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->rarg)); \
\
BSQ_LANGUAGE_ASSERT(rarg != (REPRTYPE)0, &(THIS->cframe->invoke->srcFile), THIS->cframe->dbg_currentline, "Division by zero"); \
SLPTR_STORE_CONTENTS_AS(REPRTYPE, THIS->evalTargetVar(bop->trgt), larg / rarg);

//Big Macro for generating code for primitive un-checked infix binary operations
#define PrimitiveBinaryOperatorMacroSafe(THIS, OP, TAG, REPRTYPE, OPERATOR) const PrimitiveBinaryOperatorOp<TAG>* bop = static_cast<const PrimitiveBinaryOperatorOp<TAG>*>(op); \
SLPTR_STORE_CONTENTS_AS(REPRTYPE, THIS->evalTargetVar(bop->trgt), SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->larg)) OPERATOR SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->rarg)));

//Big Macro for generating code for primitive infix equality operations
#define PrimitiveBinaryComparatorMacroSafe(THIS, OP, TAG, REPRTYPE, OPERATOR) const PrimitiveBinaryOperatorOp<TAG>* bop = static_cast<const PrimitiveBinaryOperatorOp<TAG>*>(op); \
SLPTR_STORE_CONTENTS_AS(BSQBool, THIS->evalTargetVar(bop->trgt), SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->larg)) OPERATOR SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->rarg)));

//Big Macro for generating code for primitive infix equality operations
#define PrimitiveBinaryComparatorMacroFP(THIS, OP, TAG, REPRTYPE, ISNAN, ISINFINITE, OPERATOR) const PrimitiveBinaryOperatorOp<TAG>* bop = static_cast<const PrimitiveBinaryOperatorOp<TAG>*>(op); \
REPRTYPE larg = SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->larg)); \
REPRTYPE rarg = SLPTR_LOAD_CONTENTS_AS(REPRTYPE, THIS->evalArgument(bop->rarg)); \
\
BSQ_LANGUAGE_ASSERT(!ISNAN(rarg) & !ISNAN(larg), &(THIS->cframe->invoke->srcFile), THIS->cframe->dbg_currentline, "NaN cannot be ordered"); \
BSQ_LANGUAGE_ASSERT((!ISINFINITE(rarg) | !ISINFINITE(larg)) || ((rarg <= 0) & (0 <= larg)) || ((larg <= 0) & (0 <= rarg)), &(THIS->cframe->invoke->srcFile), THIS->cframe->dbg_currentline, "Infinte values cannot be ordered"); \
SLPTR_STORE_CONTENTS_AS(BSQBool, THIS->evalTargetVar(bop->trgt), larg OPERATOR rarg);

jmp_buf Evaluator::g_entrybuff;
EvaluatorFrame Evaluator::g_callstack[BSQ_MAX_STACK];
uint8_t* Evaluator::g_constantbuffer = nullptr;

std::map<std::string, const BSQRegex*> Evaluator::g_validators;
std::map<std::string, const BSQRegex*> Evaluator::g_regexs;

void Evaluator::evalDeadFlowOp()
{
    //This should be unreachable
    BSQ_INTERNAL_ASSERT(false);
}

void Evaluator::evalAbortOp(const AbortOp *op)
{
    if(this->debuggerattached)
    {
        throw DebuggerException::CreateErrorAbortRequest({-1, this->call_count, this->cframe->invoke, op, this->cframe->dbg_currentline});
    }
    else
    {
        BSQ_LANGUAGE_ABORT(op->msg.c_str(), &this->cframe->invoke->srcFile, this->cframe->dbg_currentline);
    }
}

void Evaluator::evalAssertCheckOp(const AssertOp *op)
{
    if (!SLPTR_LOAD_CONTENTS_AS(BSQBool, this->evalArgument(op->arg)))
    {
        if(this->debuggerattached)
        {
            throw DebuggerException::CreateErrorAbortRequest({-1, this->call_count, this->cframe->invoke, op, this->cframe->dbg_currentline});
        }
        else
        {
            BSQ_LANGUAGE_ABORT(op->msg.c_str(), &this->cframe->invoke->srcFile, this->cframe->dbg_currentline);
        }
    }
}

void Evaluator::evalDebugOp(const DebugOp* op)
{
    if(op->arg.kind == ArgumentTag::InvalidOp)
    {
        //TODO: debugger break here
        assert(false);
    }
    else
    {
        auto sl = this->evalArgument(op->arg);
        auto oftype = SLPTR_LOAD_UNION_INLINE_TYPE(sl);

        auto dval = oftype->fpDisplay(oftype, SLPTR_LOAD_UNION_INLINE_DATAPTR(sl), DisplayMode::Standard);

        printf("%s\n", dval.c_str());
        fflush(stdout);
    }
}

void Evaluator::evalLoadUnintVariableValueOp(const LoadUnintVariableValueOp* op)
{
    op->oftype->clearValue(this->evalTargetVar(op->trgt));
}

void Evaluator::evalNoneInitUnionOp(const NoneInitUnionOp* op)
{
    SLPTR_STORE_UNION_INLINE_TYPE(BSQWellKnownType::g_typeNone, this->evalTargetVar(op->trgt));
}

void Evaluator::evalStoreConstantMaskValueOp(const StoreConstantMaskValueOp* op)
{
    auto mask = this->evalMaskLocation(op->gmaskoffset);
    mask[op->gindex] = op->flag ? BSQTRUE : BSQFALSE;
}

template <>
void Evaluator::evalDirectAssignOp<true>(const DirectAssignOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, op->intotype, op->sguard))
    {
        op->intotype->storeValue(this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
    }
}

template <>
void Evaluator::evalDirectAssignOp<false>(const DirectAssignOp* op)
{
    op->intotype->storeValue(this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
}

template <>
void Evaluator::evalBoxOp<true>(const BoxOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, op->intotype, op->sguard))
    {
        coerce(op->fromtype, op->intotype, this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
    }
}

template <>
void Evaluator::evalBoxOp<false>(const BoxOp* op)
{
    coerce(op->fromtype, op->intotype, this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
}

template <>
void Evaluator::evalExtractOp<true>(const ExtractOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, op->intotype, op->sguard))
    {
        coerce(op->fromtype, op->intotype, this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
    }
}

template <>
void Evaluator::evalExtractOp<false>(const ExtractOp* op)
{
    coerce(op->fromtype, op->intotype, this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
}

void Evaluator::evalLoadConstOp(const LoadConstOp* op)
{
    op->oftype->storeValue(this->evalTargetVar(op->trgt), Evaluator::evalConstArgument(op->arg));
}

void Evaluator::processTupleDirectLoadAndStore(StorageLocationPtr src, const BSQType* srctype, size_t slotoffset, TargetVar dst, const BSQType* dsttype)
{
    dsttype->storeValue(this->evalTargetVar(dst), srctype->indexStorageLocationOffset(src, slotoffset));
}

void Evaluator::processTupleVirtualLoadAndStore(StorageLocationPtr src, const BSQUnionType* srctype, BSQTupleIndex idx, TargetVar dst, const BSQType* dsttype)
{
    const BSQType* ttype = srctype->getVType(src);
    StorageLocationPtr pp = srctype->getVData_StorageLocation(src);

    //
    //TODO: this is where it might be nice to do some mono/polymorphic inline caching vtable goodness
    //
    auto tinfo = dynamic_cast<const BSQTupleInfo*>(ttype);
    auto voffset = tinfo->idxoffsets[idx];

    this->processTupleDirectLoadAndStore(pp, ttype, voffset, dst, dsttype);
}

void Evaluator::processRecordDirectLoadAndStore(StorageLocationPtr src, const BSQType* srctype, size_t slotoffset, TargetVar dst, const BSQType* dsttype)
{
    dsttype->storeValue(this->evalTargetVar(dst), srctype->indexStorageLocationOffset(src, slotoffset));
}

void Evaluator::processRecordVirtualLoadAndStore(StorageLocationPtr src, const BSQUnionType* srctype, BSQRecordPropertyID propId, TargetVar dst, const BSQType* dsttype)
{
    const BSQType* rtype = srctype->getVType(src);
    StorageLocationPtr pp = srctype->getVData_StorageLocation(src);

    //
    //TODO: this is where it might be nice to do some mono/polymorphic inline caching vtable goodness
    //
    auto rinfo = dynamic_cast<const BSQRecordInfo*>(rtype);
    auto proppos = std::find(rinfo->properties.cbegin(), rinfo->properties.cend(), propId);
    assert(proppos != rinfo->properties.cend());

    auto propidx = (size_t)std::distance(rinfo->properties.cbegin(), proppos);
    auto voffset = rinfo->propertyoffsets[propidx];

    this->processRecordDirectLoadAndStore(pp, rtype, voffset, dst, dsttype);
}
    
void Evaluator::processEntityDirectLoadAndStore(StorageLocationPtr src, const BSQType* srctype, size_t slotoffset, TargetVar dst, const BSQType* dsttype)
{
    dsttype->storeValue(this->evalTargetVar(dst), srctype->indexStorageLocationOffset(src, slotoffset));
}

void Evaluator::processEntityVirtualLoadAndStore(StorageLocationPtr src, const BSQUnionType* srctype, BSQFieldID fldId, TargetVar dst, const BSQType* dsttype)
{
    const BSQType* etype = srctype->getVType(src);
    StorageLocationPtr pp = srctype->getVData_StorageLocation(src);

    //
    //TODO: this is where it might be nice to do some mono/polymorphic inline caching vtable goodness
    //
    auto einfo = dynamic_cast<const BSQEntityInfo*>(etype);
    auto fldpos = std::find(einfo->fields.cbegin(), einfo->fields.cend(), fldId);
    assert(fldpos != einfo->fields.cend());

    auto fldidx = (size_t)std::distance(einfo->fields.cbegin(), fldpos);
    auto voffset = einfo->fieldoffsets[fldidx];

    this->processEntityDirectLoadAndStore(pp, etype, voffset, dst, dsttype);
}

void Evaluator::processGuardVarStore(const BSQGuard& gv, BSQBool f)
{
    if(gv.gvaroffset == -1)
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalGuardVar(gv.gvaroffset), f);
    }
    else
    {
        auto mask = this->evalMaskLocation(gv.gmaskoffset);
        mask[gv.gindex] = f;
    }
}

void Evaluator::evalTupleHasIndexOp(const TupleHasIndexOp* op)
{
    auto sl = this->evalArgument(op->arg);

    const BSQType* ttype = op->layouttype->getVType(sl);
    auto tinfo = dynamic_cast<const BSQTupleInfo*>(ttype);
    BSQBool hasidx = op->idx < tinfo->maxIndex;

    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), hasidx);
}

void Evaluator::evalRecordHasPropertyOp(const RecordHasPropertyOp* op)
{
    auto sl = this->evalArgument(op->arg);

    const BSQType* rtype = op->layouttype->getVType(sl);
    auto rinfo = dynamic_cast<const BSQRecordInfo*>(rtype);
    BSQBool hasprop = std::find(rinfo->properties.cbegin(), rinfo->properties.cend(), op->propId) != rinfo->properties.cend();

    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), hasprop);
}

void Evaluator::evalLoadTupleIndexDirectOp(const LoadTupleIndexDirectOp* op)
{
    this->processTupleDirectLoadAndStore(this->evalArgument(op->arg), op->layouttype, op->slotoffset, op->trgt, op->trgttype);
}

void Evaluator::evalLoadTupleIndexVirtualOp(const LoadTupleIndexVirtualOp* op)
{
    auto sl = this->evalArgument(op->arg);
    this->processTupleVirtualLoadAndStore(sl, op->layouttype, op->idx, op->trgt, op->trgttype);
}

void Evaluator::evalLoadTupleIndexSetGuardDirectOp(const LoadTupleIndexSetGuardDirectOp* op)
{
    this->processTupleDirectLoadAndStore(this->evalArgument(op->arg), op->layouttype, op->slotoffset, op->trgt, op->trgttype);
    this->processGuardVarStore(op->guard, true);
}

void Evaluator::evalLoadTupleIndexSetGuardVirtualOp(const LoadTupleIndexSetGuardVirtualOp* op)
{
    auto sl = this->evalArgument(op->arg);

    const BSQType* ttype = op->layouttype->getVType(sl);
    StorageLocationPtr pp = op->layouttype->getVData_StorageLocation(sl);

    //
    //TODO: this is where it might be nice to do some mono/polymorphic inline caching vtable goodness
    //

    auto tinfo = dynamic_cast<const BSQTupleInfo*>(ttype);
    BSQBool loadsafe = op->idx < tinfo->maxIndex;
    if(loadsafe)
    {
        this->processTupleVirtualLoadAndStore(pp, op->layouttype, op->idx, op->trgt, op->trgttype);

    }
    this->processGuardVarStore(op->guard, loadsafe);
}

void Evaluator::evalLoadRecordPropertyDirectOp(const LoadRecordPropertyDirectOp* op)
{
    this->processRecordDirectLoadAndStore(this->evalArgument(op->arg), op->layouttype, op->slotoffset, op->trgt, op->trgttype);
}

void Evaluator::evalLoadRecordPropertyVirtualOp(const LoadRecordPropertyVirtualOp* op)
{
    auto sl = this->evalArgument(op->arg);
    this->processRecordVirtualLoadAndStore(sl, op->layouttype, op->propId, op->trgt, op->trgttype);
}

void Evaluator::evalLoadRecordPropertySetGuardDirectOp(const LoadRecordPropertySetGuardDirectOp* op)
{
    this->processRecordDirectLoadAndStore(this->evalArgument(op->arg), op->layouttype, op->slotoffset, op->trgt, op->trgttype);
    this->processGuardVarStore(op->guard, true);
}

void Evaluator::evalLoadRecordPropertySetGuardVirtualOp(const LoadRecordPropertySetGuardVirtualOp* op)
{
    auto sl = this->evalArgument(op->arg);
    
    const BSQType* rtype = op->layouttype->getVType(sl);
    StorageLocationPtr pp = op->layouttype->getVData_StorageLocation(sl);

    //
    //TODO: this is where it might be nice to do some mono/polymorphic inline caching vtable goodness
    //

    auto rinfo = dynamic_cast<const BSQRecordInfo*>(rtype);
    BSQBool loadsafe = std::find(rinfo->properties.cbegin(), rinfo->properties.cend(), op->propId) != rinfo->properties.cend();
    if(loadsafe)
    {
        this->processRecordVirtualLoadAndStore(pp, op->layouttype, op->propId, op->trgt, op->trgttype);
    }
    this->processGuardVarStore(op->guard, loadsafe);
}

void Evaluator::evalLoadDirectFieldOp(const LoadEntityFieldDirectOp* op)
{
    this->processEntityDirectLoadAndStore(this->evalArgument(op->arg), op->layouttype, op->slotoffset, op->trgt, op->trgttype);
}

void Evaluator::evalLoadVirtualFieldOp(const LoadEntityFieldVirtualOp* op)
{
    auto sl = this->evalArgument(op->arg);
    this->processEntityVirtualLoadAndStore(sl, op->layouttype, op->fieldId, op->trgt, op->trgttype);
}

void Evaluator::evalProjectTupleOp(const ProjectTupleOp* op)
{
    StorageLocationPtr src = this->evalArgument(op->arg);
    StorageLocationPtr trgtl = this->evalTargetVar(op->trgt);

    void* sl = nullptr;
    if(op->layouttype->tkind == BSQTypeLayoutKind::Struct)
    {
        sl = src;
    }
    else if(op->layouttype->tkind == BSQTypeLayoutKind::Ref)
    {
        sl = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(src);
    }
    else
    {
        sl = dynamic_cast<const BSQUnionType*>(op->layouttype)->getVData_Direct(src);
    }
    
    for(size_t i = 0; i < op->idxs.size(); ++i)
    {
        auto dst = SLPTR_INDEX_DATAPTR(trgtl, op->trgttype->idxoffsets[i]);
        auto src = SLPTR_INDEX_DATAPTR(sl, std::get<1>(op->idxs[i]));

        std::get<2>(op->idxs[i])->storeValue(dst, src);
    }
}

void Evaluator::evalProjectRecordOp(const ProjectRecordOp* op)
{
    StorageLocationPtr src = this->evalArgument(op->arg);

    void* sl = nullptr;
    if(op->layouttype->tkind == BSQTypeLayoutKind::Struct)
    {
        sl = src;
    }
    else if(op->layouttype->tkind == BSQTypeLayoutKind::Ref)
    {
        sl = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(src);
    }
    else
    {
        sl = dynamic_cast<const BSQUnionType*>(op->layouttype)->getVData_Direct(src);
    }

    auto trgtl = this->evalTargetVar(op->trgt);
    for(size_t i = 0; i < op->props.size(); ++i)
    {
        auto dst = SLPTR_INDEX_DATAPTR(trgtl, op->trgttype->idxoffsets[i]);
        auto src = SLPTR_INDEX_DATAPTR(sl, std::get<1>(op->props[i]));

        std::get<2>(op->props[i])->storeValue(dst, src);
    }
}

void Evaluator::evalProjectEntityOp(const ProjectEntityOp* op)
{
    StorageLocationPtr src = this->evalArgument(op->arg);

    void* sl = nullptr;
    if(op->layouttype->tkind == BSQTypeLayoutKind::Struct)
    {
        sl = src;
    }
    else if(op->layouttype->tkind == BSQTypeLayoutKind::Ref)
    {
        sl = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(src);
    }
    else
    {
        sl = dynamic_cast<const BSQUnionType*>(op->layouttype)->getVData_Direct(src);
    }

    auto trgtl = this->evalTargetVar(op->trgt);
    for(size_t i = 0; i < op->fields.size(); ++i)
    {
        auto dst = SLPTR_INDEX_DATAPTR(trgtl, op->trgttype->idxoffsets[i]);
        auto src = SLPTR_INDEX_DATAPTR(sl, std::get<1>(op->fields[i]));

        std::get<2>(op->fields[i])->storeValue(dst, src);
    }
}

void Evaluator::evalUpdateTupleOp(const UpdateTupleOp* op)
{
    StorageLocationPtr src = this->evalArgument(op->arg);
    void** stck = (void**)GCStack::allocFrame(sizeof(void*) * 2);

    if(op->layouttype->tkind == BSQTypeLayoutKind::Struct)
    {
        stck[0] = src;
    }
    else if(op->layouttype->tkind == BSQTypeLayoutKind::Ref)
    {
        stck[0] = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(src);
    }
    else
    {
        stck[0] = dynamic_cast<const BSQUnionType*>(op->layouttype)->getVData_Direct(src);
    }

    if(op->trgttype->tkind == BSQTypeLayoutKind::Struct)
    {
        stck[1] = this->evalTargetVar(op->trgt);
        GC_MEM_COPY(stck[1], stck[0], op->trgttype->allocinfo.heapsize);
    }
    else
    {
        stck[1] = Allocator::GlobalAllocator.allocateDynamic(op->trgttype);
        GC_MEM_COPY(stck[1], stck[0], op->trgttype->allocinfo.heapsize);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(this->evalTargetVar(op->trgt), stck[1]);
    }

    for(size_t i = 0; i < op->updates.size(); ++i)
    {
        auto dst = SLPTR_INDEX_DATAPTR(stck[1], std::get<1>(op->updates[i]));
        std::get<2>(op->updates[i])->storeValue(dst, this->evalArgument(std::get<3>(op->updates[i])));
    }

    GCStack::popFrame(sizeof(void*) * 2);
}

void Evaluator::evalUpdateRecordOp(const UpdateRecordOp* op)
{
    StorageLocationPtr src = this->evalArgument(op->arg);
    void** stck = (void**)GCStack::allocFrame(sizeof(void*) * 2);

    if(op->layouttype->tkind == BSQTypeLayoutKind::Struct)
    {
        stck[0] = src;
    }
    else if(op->layouttype->tkind == BSQTypeLayoutKind::Ref)
    {
        stck[0] = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(src);
    }
    else
    {
        stck[0] = dynamic_cast<const BSQUnionType*>(op->layouttype)->getVData_Direct(src);
    }

    if(op->trgttype->tkind == BSQTypeLayoutKind::Struct)
    {
        stck[1] = this->evalTargetVar(op->trgt);
        GC_MEM_COPY(stck[1], stck[0], op->trgttype->allocinfo.heapsize);
    }
    else
    {
        stck[1] = Allocator::GlobalAllocator.allocateDynamic(op->trgttype);
        GC_MEM_COPY(stck[1], stck[0], op->trgttype->allocinfo.heapsize);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(this->evalTargetVar(op->trgt), stck[1]);
    }

    for(size_t i = 0; i < op->updates.size(); ++i)
    {
        auto dst = SLPTR_INDEX_DATAPTR(stck[1], std::get<1>(op->updates[i]));
        std::get<2>(op->updates[i])->storeValue(dst, this->evalArgument(std::get<3>(op->updates[i])));
    }

    GCStack::popFrame(sizeof(void*) * 2);
}

void Evaluator::evalUpdateEntityOp(const UpdateEntityOp* op)
{
    StorageLocationPtr src = this->evalArgument(op->arg);
    void** stck = (void**)GCStack::allocFrame(sizeof(void*) * 2);

    if(op->layouttype->tkind == BSQTypeLayoutKind::Struct)
    {
        stck[0] = src;
    }
    else if(op->layouttype->tkind == BSQTypeLayoutKind::Ref)
    {
        stck[0] = SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(src);
    }
    else
    {
        stck[0] = dynamic_cast<const BSQUnionType*>(op->layouttype)->getVData_Direct(src);
    }

    if(op->trgttype->tkind == BSQTypeLayoutKind::Struct)
    {
        stck[1] = this->evalTargetVar(op->trgt);
        GC_MEM_COPY(stck[1], stck[0], op->trgttype->allocinfo.heapsize);
    }
    else
    {
        stck[1] = Allocator::GlobalAllocator.allocateDynamic(op->trgttype);
        GC_MEM_COPY(stck[1], stck[0], op->trgttype->allocinfo.heapsize);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(this->evalTargetVar(op->trgt), stck[1]);
    }

    for(size_t i = 0; i < op->updates.size(); ++i)
    {
        auto dst = SLPTR_INDEX_DATAPTR(stck[1], std::get<1>(op->updates[i]));
        std::get<2>(op->updates[i])->storeValue(dst, this->evalArgument(std::get<3>(op->updates[i])));
    }

    GCStack::popFrame(sizeof(void*) * 2);
}

void Evaluator::evalLoadFromEpehmeralListOp(const LoadFromEpehmeralListOp* op)
{
    auto sl = this->evalArgument(op->arg);
    op->trgttype->storeValue(this->evalTargetVar(op->trgt), SLPTR_INDEX_DATAPTR(sl, op->slotoffset));
}

void Evaluator::evalMultiLoadFromEpehmeralListOp(const MultiLoadFromEpehmeralListOp* op)
{
    auto sl = this->evalArgument(op->arg);
    for(size_t i = 0; i < op->trgts.size(); ++i)
    {
        op->trgttypes[i]->storeValue(this->evalTargetVar(op->trgts[i]), SLPTR_INDEX_DATAPTR(sl, op->slotoffsets[i]));
    }
}

void Evaluator::evalSliceEphemeralListOp(const SliceEphemeralListOp* op)
{
    auto sl = this->evalArgument(op->arg);
    auto tl = this->evalTargetVar(op->trgt);

    BSQ_MEM_COPY(tl, sl, op->slotoffsetend);
}

template <>
void Evaluator::evalInvokeFixedFunctionOp<true>(const InvokeFixedFunctionOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, op->trgttype, op->sguard))
    {
        StorageLocationPtr resl = this->evalTargetVar(op->trgt);
        this->invoke(BSQInvokeDecl::g_invokes[op->invokeId], op->args, resl, op->optmaskoffset != -1 ? this->cframe->masksbase + op->optmaskoffset : nullptr);
    }
}

template <>
void Evaluator::evalInvokeFixedFunctionOp<false>(const InvokeFixedFunctionOp* op)
{
    StorageLocationPtr resl = this->evalTargetVar(op->trgt);
    this->invoke(BSQInvokeDecl::g_invokes[op->invokeId], op->args, resl, op->optmaskoffset != -1 ? this->cframe->masksbase + op->optmaskoffset : nullptr);
}

void Evaluator::evalInvokeVirtualFunctionOp(const InvokeVirtualFunctionOp* op)
{
    auto sl = this->evalArgument(op->args[0]);

    const BSQType* etype = op->rcvrlayouttype->getVType(sl);
    StorageLocationPtr rcvrloc = op->rcvrlayouttype->getVData_StorageLocation(sl);

    StorageLocationPtr resl = this->evalTargetVar(op->trgt);
    this->vinvoke(dynamic_cast<const BSQInvokeBodyDecl*>(BSQInvokeDecl::g_invokes[etype->vtable.at(op->invokeId)]), rcvrloc, op->args, resl, op->optmaskoffset != -1 ? this->cframe->masksbase + op->optmaskoffset : nullptr);
}

void Evaluator::evalInvokeVirtualOperatorOp(const InvokeVirtualOperatorOp* op)
{
    //NOT IMPLEMENTED
    assert(false);
}

void Evaluator::evalConstructorTupleOp(const ConstructorTupleOp* op)
{
    StorageLocationPtr sl = this->evalTargetVar(op->trgt);
    StorageLocationPtr tcontents = nullptr;
    if(op->oftype->tkind == BSQTypeLayoutKind::Struct)
    {
        tcontents = sl;
    }
    else
    {
        tcontents = Allocator::GlobalAllocator.allocateDynamic(op->oftype);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(sl, tcontents);
    }

    auto tupinfo = dynamic_cast<const BSQTupleInfo*>(op->oftype);
    for(size_t i = 0; i < tupinfo->idxoffsets.size(); ++i)
    {
        BSQType::g_typetable[tupinfo->ttypes[i]]->storeValue(SLPTR_INDEX_DATAPTR(tcontents, tupinfo->idxoffsets[i]), this->evalArgument(op->args[i]));
    }
}

void Evaluator::evalConstructorTupleFromEphemeralListOp(const ConstructorTupleFromEphemeralListOp* op)
{
    StorageLocationPtr sl = this->evalTargetVar(op->trgt);
    StorageLocationPtr tcontents = nullptr;
    if(op->oftype->tkind == BSQTypeLayoutKind::Struct)
    {
        tcontents = sl;
    }
    else
    {
        tcontents = Allocator::GlobalAllocator.allocateDynamic(op->oftype);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(sl, tcontents);
    }

    StorageLocationPtr tsrc = this->evalArgument(op->arg);
    BSQ_MEM_COPY(tcontents, tsrc, op->oftype->allocinfo.assigndatasize);
}

void Evaluator::evalConstructorRecordOp(const ConstructorRecordOp* op)
{
    StorageLocationPtr sl = this->evalTargetVar(op->trgt);
    StorageLocationPtr tcontents = nullptr;
    if(op->oftype->tkind == BSQTypeLayoutKind::Struct)
    {
        tcontents = sl;
    }
    else
    {
        tcontents = Allocator::GlobalAllocator.allocateDynamic(op->oftype);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(sl, tcontents);
    }

    auto recinfo = dynamic_cast<const BSQRecordInfo*>(op->oftype);
    for(size_t i = 0; i < recinfo->propertyoffsets.size(); ++i)
    {
        BSQType::g_typetable[recinfo->rtypes[i]]->storeValue(SLPTR_INDEX_DATAPTR(tcontents, recinfo->propertyoffsets[i]), this->evalArgument(op->args[i]));
    }
}

void Evaluator::evalConstructorRecordFromEphemeralListOp(const ConstructorRecordFromEphemeralListOp* op)
{
    StorageLocationPtr sl = this->evalTargetVar(op->trgt);
    StorageLocationPtr tcontents = nullptr;
    if(op->oftype->tkind == BSQTypeLayoutKind::Struct)
    {
        tcontents = sl;
    }
    else
    {
        tcontents = Allocator::GlobalAllocator.allocateDynamic(op->oftype);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(sl, tcontents);
    }

    StorageLocationPtr tsrc = this->evalArgument(op->arg);
    if(op->proppositions.empty())
    {
        BSQ_MEM_COPY(tcontents, tsrc, op->oftype->allocinfo.assigndatasize);
    }
    else
    {
        auto recinfo = dynamic_cast<const BSQRecordInfo*>(op->oftype);
        for(size_t i = 0; i < op->proppositions.size(); ++i)
        {
            auto proppos = op->proppositions[i];
            BSQType::g_typetable[recinfo->rtypes[proppos]]->storeValue(SLPTR_INDEX_DATAPTR(tcontents, recinfo->propertyoffsets[proppos]), SLPTR_INDEX_DATAPTR(tsrc, op->argtype->idxoffsets[i]));
        }
    }
}

void Evaluator::evalEphemeralListExtendOp(const EphemeralListExtendOp* op)
{
    StorageLocationPtr tcontents = this->evalTargetVar(op->trgt);
    StorageLocationPtr acontents = this->evalArgument(op->arg);
    BSQ_MEM_COPY(tcontents, acontents, op->argtype->allocinfo.inlinedatasize);

    auto idxbase = op->argtype->idxoffsets.size();
    for(size_t i = 0; i < op->ext.size(); ++i)
    {
        BSQType::g_typetable[op->resultType->etypes[idxbase + i]]->storeValue(SLPTR_INDEX_DATAPTR(tcontents, op->argtype->idxoffsets[idxbase + i]), this->evalArgument(op->ext[i]));
    }
}

void Evaluator::evalConstructorEphemeralListOp(const ConstructorEphemeralListOp* op)
{
    StorageLocationPtr tcontents = this->evalTargetVar(op->trgt);

    for(size_t i = 0; i < op->oftype->idxoffsets.size(); ++i)
    {
        BSQType::g_typetable[op->oftype->etypes[i]]->storeValue(op->oftype->indexStorageLocationOffset(tcontents, op->oftype->idxoffsets[i]), this->evalArgument(op->args[i]));
    }
}

void Evaluator::evalConstructorEntityDirectOp(const ConstructorEntityDirectOp* op)
{
    StorageLocationPtr sl = this->evalTargetVar(op->trgt);

    StorageLocationPtr tcontents = nullptr;
    if(op->oftype->tkind == BSQTypeLayoutKind::Struct)
    {
        tcontents = sl;
    }
    else
    {
        tcontents = Allocator::GlobalAllocator.allocateDynamic(op->oftype);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(sl, tcontents);
    }

    auto einfo = dynamic_cast<const BSQEntityInfo*>(op->oftype);
    for(size_t i = 0; i < einfo->fieldoffsets.size(); ++i)
    {
        BSQType::g_typetable[einfo->ftypes[i]]->storeValue(SLPTR_INDEX_DATAPTR(tcontents, einfo->fieldoffsets[i]), this->evalArgument(op->args[i]));
    }
}

void Evaluator::evalPrefixNotOp(const PrefixNotOp* op)
{
    BSQBool sval = !SLPTR_LOAD_CONTENTS_AS(BSQBool, this->evalArgument(op->arg));
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), sval);
}

void Evaluator::evalAllTrueOp(const AllTrueOp* op)
{
    auto fpos = std::find_if(op->args.cbegin(), op->args.cend(), [this](Argument arg) {
        return !SLPTR_LOAD_CONTENTS_AS(BSQBool, this->evalArgument(arg));
    });

    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), fpos == op->args.cend());
}
    
void Evaluator::evalSomeTrueOp(const SomeTrueOp* op)
{
    auto tpos = std::find_if(op->args.cbegin(), op->args.cend(), [this](Argument arg) {
        return SLPTR_LOAD_CONTENTS_AS(BSQBool, this->evalArgument(arg));
    });

    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), tpos != op->args.cend());
}

template<>
void Evaluator::evalBinKeyEqFastOp<true>(const BinKeyEqFastOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), op->oftype->fpkeycmp(op->oftype, this->evalArgument(op->argl), this->evalArgument(op->argr)) == 0);
    }
}

template<>
void Evaluator::evalBinKeyEqFastOp<false>(const BinKeyEqFastOp* op)
{
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), op->oftype->fpkeycmp(op->oftype, this->evalArgument(op->argl), this->evalArgument(op->argr)) == 0);
}

template<>
void Evaluator::evalBinKeyEqStaticOp<true>(const BinKeyEqStaticOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        auto lleval = this->evalArgument(op->argl);
        auto rreval = this->evalArgument(op->argr);

        auto lldata = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVData_StorageLocation(lleval) : lleval; 
        auto rrdata = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVData_StorageLocation(rreval) : rreval; 
    
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), op->oftype->fpkeycmp(op->oftype, lldata, rrdata) == 0);
    }
}

template<>
void Evaluator::evalBinKeyEqStaticOp<false>(const BinKeyEqStaticOp* op)
{
    auto lleval = this->evalArgument(op->argl);
    auto rreval = this->evalArgument(op->argr);

    auto lldata = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVData_StorageLocation(lleval) : lleval; 
    auto rrdata = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVData_StorageLocation(rreval) : rreval;
    
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), op->oftype->fpkeycmp(op->oftype, lldata, rrdata) == 0);
}

template<>
void Evaluator::evalBinKeyEqVirtualOp<true>(const BinKeyEqVirtualOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        auto lleval = this->evalArgument(op->argl);
        auto rreval = this->evalArgument(op->argr);

        auto lltype = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVType(lleval) : op->argllayout; 
        auto rrtype = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVType(rreval) : op->argrlayout;

        if(lltype->tid != rrtype->tid)
        {
            SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), BSQFALSE);
        }
        else
        {
            auto lldata = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVData_StorageLocation(lleval) : lleval; 
            auto rrdata = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVData_StorageLocation(rreval) : rreval;

            SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), lltype->fpkeycmp(lltype, lldata, rrdata) == 0);
        }
    }
}

template<>
void Evaluator::evalBinKeyEqVirtualOp<false>(const BinKeyEqVirtualOp* op)
{
    auto lleval = this->evalArgument(op->argl);
    auto rreval = this->evalArgument(op->argr);

    auto lltype = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVType(lleval) : op->argllayout; 
    auto rrtype = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVType(rreval) : op->argrlayout;

    if(lltype->tid != rrtype->tid)
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), BSQFALSE);
    }
    else
    {
        auto lldata = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVData_StorageLocation(lleval) : lleval; 
        auto rrdata = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVData_StorageLocation(rreval) : rreval;
    
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), lltype->fpkeycmp(lltype, lldata, rrdata) == 0);
    }
}

void Evaluator::evalBinKeyLessFastOp(const BinKeyLessFastOp* op)
{
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), op->oftype->fpkeycmp(op->oftype, this->evalArgument(op->argl), this->evalArgument(op->argr)) < 0);
}

void Evaluator::evalBinKeyLessStaticOp(const BinKeyLessStaticOp* op)
{
    auto lleval = this->evalArgument(op->argl);
    auto rreval = this->evalArgument(op->argr);

    auto lldata = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVData_StorageLocation(lleval) : lleval; 
    auto rrdata = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVData_StorageLocation(rreval) : rreval; 
    
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), op->oftype->fpkeycmp(op->oftype, lldata, rrdata) < 0);
}

void Evaluator::evalBinKeyLessVirtualOp(const BinKeyLessVirtualOp* op)
{
    auto lleval = this->evalArgument(op->argl);
    auto rreval = this->evalArgument(op->argr);

    auto lltype = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVType(lleval) : op->argllayout; 
    auto rrtype = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVType(rreval) : op->argrlayout;

    if(lltype->tid != rrtype->tid)
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), lltype->tid < rrtype->tid);
    }
    else
    {
        auto lldata = op->argllayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argllayout)->getVData_StorageLocation(lleval) : lleval; 
        auto rrdata = op->argrlayout->isUnion() ? dynamic_cast<const BSQUnionType*>(op->argrlayout)->getVData_StorageLocation(rreval) : rreval;
    
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), lltype->fpkeycmp(lltype, lldata, rrdata) < 0);
    }
}

inline BSQBool isNoneTest(const BSQUnionType* tt, StorageLocationPtr chkl)
{
    return (BSQBool)(tt->getVType(chkl)->tid == BSQ_TYPE_ID_NONE);
}

inline BSQBool isNothingTest(const BSQUnionType* tt, StorageLocationPtr chkl)
{
    return (BSQBool)(tt->getVType(chkl)->tid == BSQ_TYPE_ID_NOTHING);
}

inline BSQTypeID getTypeIDForTypeOf(const BSQUnionType* tt, StorageLocationPtr chkl)
{
    return tt->getVType(chkl)->tid;
}

template <>
void Evaluator::evalIsNoneOp<true>(const TypeIsNoneOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), isNoneTest(op->arglayout, this->evalArgument(op->arg)));
    }
}

template <>
void Evaluator::evalIsNoneOp<false>(const TypeIsNoneOp* op)
{
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), isNoneTest(op->arglayout, this->evalArgument(op->arg)));
}

template <>
void Evaluator::evalIsSomeOp<true>(const TypeIsSomeOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), !isNoneTest(op->arglayout, this->evalArgument(op->arg)));
    }
}

template <>
void Evaluator::evalIsSomeOp<false>(const TypeIsSomeOp* op)
{
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), !isNoneTest(op->arglayout, this->evalArgument(op->arg)));
}

template <>
void Evaluator::evalIsNothingOp<true>(const TypeIsNothingOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), isNothingTest(op->arglayout, this->evalArgument(op->arg)));
    }
}

template <>
void Evaluator::evalIsNothingOp<false>(const TypeIsNothingOp* op)
{
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), isNothingTest(op->arglayout, this->evalArgument(op->arg)));
}

template <>
void Evaluator::evalTypeTagIsOp<true>(const TypeTagIsOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), getTypeIDForTypeOf(op->arglayout, this->evalArgument(op->arg)) == op->oftype->tid);
    }
}

template <>
void Evaluator::evalTypeTagIsOp<false>(const TypeTagIsOp* op)
{
    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), getTypeIDForTypeOf(op->arglayout, this->evalArgument(op->arg)) == op->oftype->tid);
}

template <>
void Evaluator::evalTypeTagSubtypeOfOp<true>(const TypeTagSubtypeOfOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, BSQWellKnownType::g_typeBool, op->sguard))
    {
        auto rtid = getTypeIDForTypeOf(op->arglayout, this->evalArgument(op->arg));
        auto subtype = std::find(op->oftype->subtypes.cbegin(), op->oftype->subtypes.cend(), rtid) != op->oftype->subtypes.cend();

        SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), subtype);
    }
}

template <>
void Evaluator::evalTypeTagSubtypeOfOp<false>(const TypeTagSubtypeOfOp* op)
{
    auto rtid = getTypeIDForTypeOf(op->arglayout, this->evalArgument(op->arg));
    auto subtype = std::find(op->oftype->subtypes.cbegin(), op->oftype->subtypes.cend(), rtid) != op->oftype->subtypes.cend();

    SLPTR_STORE_CONTENTS_AS(BSQBool, this->evalTargetVar(op->trgt), subtype);
}

InterpOp* Evaluator::evalJumpOp(const JumpOp* op)
{
    return this->advanceCurrentOp(op->offset);
}

InterpOp* Evaluator::evalJumpCondOp(const JumpCondOp* op)
{
    BSQBool jc = SLPTR_LOAD_CONTENTS_AS(BSQBool, this->evalArgument(op->arg));
    if(jc)
    {
        return this->advanceCurrentOp(op->toffset);
    }
    else
    {
        return this->advanceCurrentOp(op->foffset);
    }
}

InterpOp* Evaluator::evalJumpNoneOp(const JumpNoneOp* op)
{
    BSQBool isnone = isNoneTest(op->arglayout, this->evalArgument(op->arg));
    
    if(isnone)
    {
        return this->advanceCurrentOp(op->noffset);
    }
    else
    {
        return this->advanceCurrentOp(op->soffset);
    }
}

template <>
void Evaluator::evalRegisterAssignOp<true>(const RegisterAssignOp* op)
{
    if(this->tryProcessGuardStmt(op->trgt, op->oftype, op->sguard))
    {
        op->oftype->storeValue(this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
    }
}

template <>
void Evaluator::evalRegisterAssignOp<false>(const RegisterAssignOp* op)
{
    op->oftype->storeValue(this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
}

void Evaluator::evalReturnAssignOp(const ReturnAssignOp* op)
{
    op->oftype->storeValue(this->evalTargetVar(op->trgt), this->evalArgument(op->arg));
}

void Evaluator::evalReturnAssignOfConsOp(const ReturnAssignOfConsOp* op)
{
    StorageLocationPtr tcontents = nullptr;
    if(op->oftype->tkind == BSQTypeLayoutKind::Struct)
    {
        tcontents = this->evalTargetVar(op->trgt);
    }
    else
    {
        tcontents = Allocator::GlobalAllocator.allocateDynamic(op->oftype);
        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(this->evalTargetVar(op->trgt), tcontents);
    }

    const BSQEntityInfo* entityinfo = dynamic_cast<const BSQEntityInfo*>(op->oftype);
    for(size_t i = 0; i < entityinfo->fieldoffsets.size(); ++i)
    {
        BSQType::g_typetable[entityinfo->ftypes[i]]->storeValue(SLPTR_INDEX_DATAPTR(tcontents, entityinfo->fieldoffsets[i]), this->evalArgument(op->args[i]));
    }
}

void Evaluator::evalVarLifetimeStartOp(const VarLifetimeStartOp* op)
{
#ifdef BSQ_DEBUG_BUILD
    if(!this->cframe->dbg_locals.contains(op->name))
    {
        VariableHomeLocationInfo vhome{op->name, op->oftype, op->homelocation};
        this->cframe->dbg_locals[op->name] = vhome;
    }
#endif    
}

void Evaluator::evalVarLifetimeEndOp(const VarLifetimeEndOp* op)
{
#ifdef BSQ_DEBUG_BUILD
    this->cframe->dbg_locals.erase(op->name);
#endif    
} 

void Evaluator::evalVarHomeLocationValueUpdate(const VarHomeLocationValueUpdate* op)
{
#ifdef BSQ_DEBUG_BUILD 
    op->oftype->storeValue(this->evalTargetVar(op->homelocation), this->evalArgument(op->updatevar));
#endif    
} 

void Evaluator::evaluateOpCode(const InterpOp* op)
{    
    switch(op->tag)
    {
    case OpCodeTag::DeadFlowOp:
    {
        this->evalDeadFlowOp();
        break;
    }
    case OpCodeTag::AbortOp:
    {
        this->evalAbortOp(static_cast<const AbortOp*>(op));
        break;
    }
    case OpCodeTag::AssertOp:
    {
        this->evalAssertCheckOp(static_cast<const AssertOp*>(op));
        break;
    }
    case OpCodeTag::DebugOp:
    {
        this->evalDebugOp(static_cast<const DebugOp*>(op));
        break;
    }
    case OpCodeTag::LoadUnintVariableValueOp:
    {
        this->evalLoadUnintVariableValueOp(static_cast<const LoadUnintVariableValueOp*>(op));
        break;
    }
    case OpCodeTag::NoneInitUnionOp:
    {
        this->evalNoneInitUnionOp(static_cast<const NoneInitUnionOp*>(op));
        break;
    }
    case OpCodeTag::StoreConstantMaskValueOp:
    {
        this->evalStoreConstantMaskValueOp(static_cast<const StoreConstantMaskValueOp*>(op));
        break;
    }
    case OpCodeTag::DirectAssignOp:
    {
        auto daop = static_cast<const DirectAssignOp*>(op);
        if(daop->sguard.enabled)
        {
            this->evalDirectAssignOp<true>(daop);
        }
        else
        {
            this->evalDirectAssignOp<false>(daop);
        }
        break;
    }
    case OpCodeTag::BoxOp:
    {
        auto bop = static_cast<const BoxOp*>(op);
        if(bop->sguard.enabled)
        {
            this->evalBoxOp<true>(bop);
        }
        else
        {
            this->evalBoxOp<false>(bop);
        }
        break;
    }
    case OpCodeTag::ExtractOp:
    {
        auto exop = static_cast<const ExtractOp*>(op);
        if(exop->sguard.enabled)
        {
            this->evalExtractOp<true>(exop);
        }
        else
        {
            this->evalExtractOp<false>(exop);
        }
        break;
    }
    case OpCodeTag::LoadConstOp:
    {
        this->evalLoadConstOp(static_cast<const LoadConstOp*>(op));
        break;
    }
    case OpCodeTag::TupleHasIndexOp:
    {
        this->evalTupleHasIndexOp(static_cast<const TupleHasIndexOp*>(op));
        break;
    }
    case OpCodeTag::RecordHasPropertyOp:
    {
        this->evalRecordHasPropertyOp(static_cast<const RecordHasPropertyOp*>(op));
        break;
    }
    case OpCodeTag::LoadTupleIndexDirectOp:
    {
        this->evalLoadTupleIndexDirectOp(static_cast<const LoadTupleIndexDirectOp*>(op));
        break;
    }
    case OpCodeTag::LoadTupleIndexVirtualOp:
    {
        this->evalLoadTupleIndexVirtualOp(static_cast<const LoadTupleIndexVirtualOp*>(op));
        break;
    }
    case OpCodeTag::LoadTupleIndexSetGuardDirectOp:
    {
        this->evalLoadTupleIndexSetGuardDirectOp(static_cast<const LoadTupleIndexSetGuardDirectOp*>(op));
        break;
    }
    case OpCodeTag::LoadTupleIndexSetGuardVirtualOp:
    {
        this->evalLoadTupleIndexSetGuardVirtualOp(static_cast<const LoadTupleIndexSetGuardVirtualOp*>(op));
        break;
    }
    case OpCodeTag::LoadRecordPropertyDirectOp:
    {
        this->evalLoadRecordPropertyDirectOp(static_cast<const LoadRecordPropertyDirectOp*>(op));
        break;
    }
    case OpCodeTag::LoadRecordPropertyVirtualOp:
    {
        this->evalLoadRecordPropertyVirtualOp(static_cast<const LoadRecordPropertyVirtualOp*>(op));
        break;
    }
    case OpCodeTag::LoadRecordPropertySetGuardDirectOp:
    {
        this->evalLoadRecordPropertySetGuardDirectOp(static_cast<const LoadRecordPropertySetGuardDirectOp*>(op));
        break;
    }
    case OpCodeTag::LoadRecordPropertySetGuardVirtualOp:
    {
        this->evalLoadRecordPropertySetGuardVirtualOp(static_cast<const LoadRecordPropertySetGuardVirtualOp*>(op));
        break;
    }
    case OpCodeTag::LoadEntityFieldDirectOp:
    {
        this->evalLoadDirectFieldOp(static_cast<const LoadEntityFieldDirectOp*>(op));
        break;
    }
    case OpCodeTag::LoadEntityFieldVirtualOp:
    {
        this->evalLoadVirtualFieldOp(static_cast<const LoadEntityFieldVirtualOp*>(op));
        break;
    }
    case OpCodeTag::ProjectTupleOp:
    {
        this->evalProjectTupleOp(static_cast<const ProjectTupleOp*>(op));
        break;
    }
    case OpCodeTag::ProjectRecordOp:
    {
        this->evalProjectRecordOp(static_cast<const ProjectRecordOp*>(op));
        break;
    }
    case OpCodeTag::ProjectEntityOp:
    {
        this->evalProjectEntityOp(static_cast<const ProjectEntityOp*>(op));
        break;
    }
    case OpCodeTag::UpdateTupleOp:
    {
        this->evalUpdateTupleOp(static_cast<const UpdateTupleOp*>(op));
        break;
    }
    case OpCodeTag::UpdateRecordOp:
    {
        this->evalUpdateRecordOp(static_cast<const UpdateRecordOp*>(op));
        break;
    }
    case OpCodeTag::UpdateEntityOp:
    {
        this->evalUpdateEntityOp(static_cast<const UpdateEntityOp*>(op));
        break;
    }
    case OpCodeTag::LoadFromEpehmeralListOp:
    {
        this->evalLoadFromEpehmeralListOp(static_cast<const LoadFromEpehmeralListOp*>(op));
        break;
    }
    case OpCodeTag::MultiLoadFromEpehmeralListOp:
    {
        this->evalMultiLoadFromEpehmeralListOp(static_cast<const MultiLoadFromEpehmeralListOp*>(op));
        break;
    }
    case OpCodeTag::SliceEphemeralListOp:
    {
        this->evalSliceEphemeralListOp(static_cast<const SliceEphemeralListOp*>(op));
        break;
    }
    case OpCodeTag::InvokeFixedFunctionOp:
    {
        auto opc = static_cast<const InvokeFixedFunctionOp*>(op);
        if(opc->sguard.enabled)
        {
            this->evalInvokeFixedFunctionOp<true>(opc);
        }
        else
        {
            this->evalInvokeFixedFunctionOp<false>(opc);
        }
        break;
    }
    case OpCodeTag::InvokeVirtualFunctionOp:
    {
        this->evalInvokeVirtualFunctionOp(static_cast<const InvokeVirtualFunctionOp*>(op));
        break;
    }
    case OpCodeTag::InvokeVirtualOperatorOp:
    {
        this->evalInvokeVirtualOperatorOp(static_cast<const InvokeVirtualOperatorOp*>(op));
        break;
    }
    case OpCodeTag::ConstructorTupleOp:
    {
        this->evalConstructorTupleOp(static_cast<const ConstructorTupleOp*>(op));
        break;
    }
    case OpCodeTag::ConstructorTupleFromEphemeralListOp:
    {
        this->evalConstructorTupleFromEphemeralListOp(static_cast<const ConstructorTupleFromEphemeralListOp*>(op));
        break;
    }
    case OpCodeTag::ConstructorRecordOp:
    {
        this->evalConstructorRecordOp(static_cast<const ConstructorRecordOp*>(op));
        break;
    }
    case OpCodeTag::ConstructorRecordFromEphemeralListOp:
    {
        this->evalConstructorRecordFromEphemeralListOp(static_cast<const ConstructorRecordFromEphemeralListOp*>(op));
        break;
    }
    case OpCodeTag::EphemeralListExtendOp:
    {
        this->evalEphemeralListExtendOp(static_cast<const EphemeralListExtendOp*>(op));
        break;
    }
    case OpCodeTag::ConstructorEphemeralListOp:
    {
        this->evalConstructorEphemeralListOp(static_cast<const ConstructorEphemeralListOp*>(op));
        break;
    }
    case OpCodeTag::ConstructorEntityDirectOp:
    {
        this->evalConstructorEntityDirectOp(static_cast<const ConstructorEntityDirectOp*>(op));
        break;
    }
    case OpCodeTag::PrefixNotOp:
    {
        this->evalPrefixNotOp(static_cast<const PrefixNotOp*>(op));
        break;
    }
    case OpCodeTag::AllTrueOp:
    {
        this->evalAllTrueOp(static_cast<const AllTrueOp*>(op));
        break;
    }
    case OpCodeTag::SomeTrueOp:
    {
        this->evalSomeTrueOp(static_cast<const SomeTrueOp*>(op));
        break;
    }
    case OpCodeTag::BinKeyEqFastOp:
    {
        auto ope = static_cast<const BinKeyEqFastOp*>(op);
        if(ope->sguard.enabled)
        {
            this->evalBinKeyEqFastOp<true>(ope);
        }
        else
        {
            this->evalBinKeyEqFastOp<false>(ope);
        }
        break;
    }
    case OpCodeTag::BinKeyEqStaticOp:
    {
        auto ope = static_cast<const BinKeyEqStaticOp*>(op);
        if(ope->sguard.enabled)
        {
            this->evalBinKeyEqStaticOp<true>(ope);
        }
        else
        {
            this->evalBinKeyEqStaticOp<false>(ope);
        }
        break;
    }
    case OpCodeTag::BinKeyEqVirtualOp:
    {
        auto ope = static_cast<const BinKeyEqVirtualOp*>(op);
        if(ope->sguard.enabled)
        {
            this->evalBinKeyEqVirtualOp<true>(ope);
        }
        else
        {
            this->evalBinKeyEqVirtualOp<false>(ope);
        }
        break;
    }
    case OpCodeTag::BinKeyLessFastOp:
    {
        this->evalBinKeyLessFastOp(static_cast<const BinKeyLessFastOp*>(op));
        break;
    }
    case OpCodeTag::BinKeyLessStaticOp:
    {
        this->evalBinKeyLessStaticOp(static_cast<const BinKeyLessStaticOp*>(op));
        break;
    }
    case OpCodeTag::BinKeyLessVirtualOp:
    {
        this->evalBinKeyLessVirtualOp(static_cast<const BinKeyLessVirtualOp*>(op));
        break;
    }
    case OpCodeTag::TypeIsNoneOp:
    {
        auto opc = static_cast<const TypeIsNoneOp*>(op);
        if(opc->sguard.enabled)
        {
            this->evalIsNoneOp<true>(opc);
        }
        else
        {
            this->evalIsNoneOp<false>(opc);
        }
        break;
    }
    case OpCodeTag::TypeIsSomeOp:
    {
        auto opc = static_cast<const TypeIsSomeOp*>(op);
        if(opc->sguard.enabled)
        {
            this->evalIsSomeOp<true>(opc);
        }
        else
        {
            this->evalIsSomeOp<false>(opc);
        }
        break;
    }
    case OpCodeTag::TypeIsNothingOp:
    {
        auto opc = static_cast<const TypeIsNothingOp*>(op);
        if(opc->sguard.enabled)
        {
            this->evalIsNothingOp<true>(opc);
        }
        else
        {
            this->evalIsNothingOp<false>(opc);
        }
        break;
    }
    case OpCodeTag::TypeTagIsOp:
    {
        auto opc = static_cast<const TypeTagIsOp*>(op);
        if(opc->sguard.enabled)
        {
            this->evalTypeTagIsOp<true>(opc);
        }
        else
        {
            this->evalTypeTagIsOp<false>(opc);
        }
        break;
    }
    case OpCodeTag::TypeTagSubtypeOfOp:
    {
        auto opc = static_cast<const TypeTagSubtypeOfOp*>(op);
        if(opc->sguard.enabled)
        {
            this->evalTypeTagSubtypeOfOp<true>(opc);
        }
        else
        {
            this->evalTypeTagSubtypeOfOp<false>(opc);
        }
        break;
    }
    //
    // ---- jump operations are handled in the outer loop ----
    //
    case OpCodeTag::RegisterAssignOp:
    {
        auto opc = static_cast<const RegisterAssignOp*>(op);
        if(opc->sguard.enabled)
        {
            this->evalRegisterAssignOp<true>(opc);
        }
        else
        {
            this->evalRegisterAssignOp<false>(opc);
        }
        break;
    }
    case OpCodeTag::ReturnAssignOp:
    {
        this->evalReturnAssignOp(static_cast<const ReturnAssignOp*>(op));
        break;
    }
    case OpCodeTag::ReturnAssignOfConsOp:
    {
        this->evalReturnAssignOfConsOp(static_cast<const ReturnAssignOfConsOp*>(op));
        break;
    }
    case OpCodeTag::VarLifetimeStartOp:
    {
        this->evalVarLifetimeStartOp(static_cast<const VarLifetimeStartOp*>(op));
        break;
    }
    case OpCodeTag::VarLifetimeEndOp:
    {
        this->evalVarLifetimeEndOp(static_cast<const VarLifetimeEndOp*>(op));
        break;
    }
    case OpCodeTag::VarHomeLocationValueUpdate:
    {
        this->evalVarHomeLocationValueUpdate(static_cast<const VarHomeLocationValueUpdate*>(op));
        break;
    }
    case OpCodeTag::NegateIntOp:
    {
        PrimitiveNegateOperatorMacroChecked(this, op, OpCodeTag::NegateDecimalOp, BSQInt, "Int negation overflow/underflow");
        break;
    }
    case OpCodeTag::NegateBigIntOp:
    {
        PrimitiveNegateOperatorMacroSafe(this, op, OpCodeTag::NegateBigIntOp, BSQBigInt);
        break;
    }
    case OpCodeTag::NegateRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::NegateFloatOp:
    {
        PrimitiveNegateOperatorMacroSafe(this, op, OpCodeTag::NegateFloatOp, BSQFloat);
        break;
    }
    case OpCodeTag::NegateDecimalOp:
    {
        PrimitiveNegateOperatorMacroSafe(this, op, OpCodeTag::NegateDecimalOp, BSQDecimal);
        break;
    }
    case OpCodeTag::AddNatOp:
    {
        PrimitiveBinaryOperatorMacroChecked(this, op, OpCodeTag::AddNatOp, BSQNat, +, __builtin_add_overflow, "Nat addition overflow")
        break;
    }
    case OpCodeTag::AddIntOp:
    {
        PrimitiveBinaryOperatorMacroChecked(this, op, OpCodeTag::AddIntOp, BSQInt, +, __builtin_add_overflow, "Int addition overflow/underflow")
        break;
    }
    case OpCodeTag::AddBigNatOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::AddBigNatOp, BSQBigNat, +)
        break;
    }
    case OpCodeTag::AddBigIntOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::AddBigIntOp, BSQBigInt, +)
        break;
    }
    case OpCodeTag::AddRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::AddFloatOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::AddFloatOp, BSQFloat, +)
        break;
    }
    case OpCodeTag::AddDecimalOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::AddDecimalOp, BSQDecimal, +)
        break;
    }
    case OpCodeTag::SubNatOp:
    {
        PrimitiveBinaryOperatorMacroChecked(this, op, OpCodeTag::SubNatOp, BSQNat, -, __builtin_sub_overflow, "Nat subtraction overflow")
        break;
    }
    case OpCodeTag::SubIntOp:
    {
        PrimitiveBinaryOperatorMacroChecked(this, op, OpCodeTag::SubIntOp, BSQInt, -, __builtin_sub_overflow, "Int subtraction overflow/underflow")
        break;
    }
    case OpCodeTag::SubBigNatOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::SubBigNatOp, BSQBigNat, -)
        break;
    }
    case OpCodeTag::SubBigIntOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::SubBigIntOp, BSQBigInt, -)
        break;
    }
    case OpCodeTag::SubRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::SubFloatOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::SubFloatOp, BSQFloat, -)
        break;
    }
    case OpCodeTag::SubDecimalOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::SubDecimalOp, BSQDecimal, -)
        break;
    }
    case OpCodeTag::MultNatOp:
    {
        PrimitiveBinaryOperatorMacroChecked(this, op, OpCodeTag::MultNatOp, BSQNat, *, __builtin_mul_overflow, "Nat multiplication overflow")
        break;
    }
    case OpCodeTag::MultIntOp:
    {
        PrimitiveBinaryOperatorMacroChecked(this, op, OpCodeTag::MultIntOp, BSQInt, *, __builtin_mul_overflow, "Int multiplication underflow/overflow")
        break;
    }
    case OpCodeTag::MultBigNatOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::MultBigNatOp, BSQBigNat, *)
        break;
    }
    case OpCodeTag::MultBigIntOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::MultBigIntOp, BSQBigInt, *)
        break;
    }
    case OpCodeTag::MultRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::MultFloatOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::MultFloatOp, BSQFloat, *)
        break;
    }
    case OpCodeTag::MultDecimalOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::MultDecimalOp, BSQDecimal, *)
        break;
    }
    case OpCodeTag::DivNatOp:
    {
        PrimitiveBinaryOperatorMacroCheckedDiv(this, op, OpCodeTag::DivNatOp, BSQNat)
        break;
    }
    case OpCodeTag::DivIntOp:
    {
        PrimitiveBinaryOperatorMacroCheckedDiv(this, op, OpCodeTag::DivIntOp, BSQInt)
        break;
    }
    case OpCodeTag::DivBigNatOp:
    {
        PrimitiveBinaryOperatorMacroCheckedDiv(this, op, OpCodeTag::DivBigNatOp, BSQBigNat)
        break;
    }
    case OpCodeTag::DivBigIntOp:
    {
        PrimitiveBinaryOperatorMacroCheckedDiv(this, op, OpCodeTag::DivBigIntOp, BSQBigInt)
        break;
    }
    case OpCodeTag::DivRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::DivFloatOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::DivFloatOp, BSQFloat, /)
        break;
    }
    case OpCodeTag::DivDecimalOp:
    {
        PrimitiveBinaryOperatorMacroSafe(this, op, OpCodeTag::DivDecimalOp, BSQDecimal, /)
        break;
    }
    case OpCodeTag::EqNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::EqNatOp, BSQNat, ==)
        break;
    }
    case OpCodeTag::EqIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::EqIntOp, BSQInt, ==)
        break;
    }
    case OpCodeTag::EqBigNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::EqBigNatOp, BSQBigNat, ==)
        break;
    }
    case OpCodeTag::EqBigIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::EqBigIntOp, BSQBigInt, ==)
        break;
    }
    case OpCodeTag::EqRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::EqFloatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::EqFloatOp, BSQFloat, ==)
        break;
    }
    case OpCodeTag::EqDecimalOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::EqDecimalOp, BSQDecimal, ==)
        break;
    }
    case OpCodeTag::NeqNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::NeqNatOp, BSQNat, !=)
        break;
    }
    case OpCodeTag::NeqIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::NeqIntOp, BSQInt, !=)
        break;
    }
    case OpCodeTag::NeqBigNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::NeqBigNatOp, BSQBigNat, !=)
        break;
    }
    case OpCodeTag::NeqBigIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::NeqBigIntOp, BSQBigInt, !=)
        break;
    }
    case OpCodeTag::NeqRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::NeqFloatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::NeqFloatOp, BSQFloat, !=)
        break;
    }
    case OpCodeTag::NeqDecimalOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::NeqDecimalOp, BSQDecimal, !=)
        break;
    }
    case OpCodeTag::LtNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LtNatOp, BSQNat, <)
        break;
    }
    case OpCodeTag::LtIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LtIntOp, BSQInt, <)
        break;
    }
    case OpCodeTag::LtBigNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LtBigNatOp, BSQBigNat, <)
        break;
    }
    case OpCodeTag::LtBigIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LtBigIntOp, BSQBigInt, <)
        break;
    }
    case OpCodeTag::LtRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::LtFloatOp:
    {
        PrimitiveBinaryComparatorMacroFP(this, op, OpCodeTag::LtFloatOp, BSQFloat, std::isnan, std::isinf, <)
        break;
    }
    case OpCodeTag::LtDecimalOp:
    {
        PrimitiveBinaryComparatorMacroFP(this, op, OpCodeTag::LtDecimalOp, BSQDecimal, std::isnan, std::isinf, <)
        break;
    }
    case OpCodeTag::LeNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LeNatOp, BSQNat, <=)
        break;
    }
    case OpCodeTag::LeIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LeIntOp, BSQInt, <=)
        break;
    }
    case OpCodeTag::LeBigNatOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LeBigNatOp, BSQBigNat, <=)
        break;
    }
    case OpCodeTag::LeBigIntOp:
    {
        PrimitiveBinaryComparatorMacroSafe(this, op, OpCodeTag::LeBigIntOp, BSQBigInt, <=)
        break;
    }
    case OpCodeTag::LeRationalOp:
    {
        assert(false);
        break;
    }
    case OpCodeTag::LeFloatOp:
    {
        PrimitiveBinaryComparatorMacroFP(this, op, OpCodeTag::LeFloatOp, BSQFloat, std::isnan, std::isinf, <=)
        break;
    }
    case OpCodeTag::LeDecimalOp:
    {
        PrimitiveBinaryComparatorMacroFP(this, op, OpCodeTag::LeDecimalOp, BSQDecimal, std::isnan, std::isinf, <=)
        break;
    }
    default:
    {
        assert(false);
        break;
    }
    }
}

void Evaluator::evaluateOpCodeBlocks()
{
    InterpOp* op = this->getCurrentOp();
    do
    {
#ifdef BSQ_DEBUG_BUILD
        if(this->debuggerattached)
        {
            if(this->advanceOpAndProcsssBP(op))
            {
                Evaluator::fpDebuggerAction(this);
            }
        }
#endif

        switch(op->tag)
        {
        case OpCodeTag::JumpOp:
        {
            op = this->evalJumpOp(static_cast<const JumpOp*>(op));
            break;
        }
        case OpCodeTag::JumpCondOp:
        {
            op = this->evalJumpCondOp(static_cast<const JumpCondOp*>(op));
            break;
        }
        case OpCodeTag::JumpNoneOp:
        {
            op = this->evalJumpNoneOp(static_cast<const JumpNoneOp*>(op));
            break;
        }
        default:
        {
            this->evaluateOpCode(op);
            op = this->advanceCurrentOp();
            break;
        }
        }
    } while (this->hasMoreOps());
}
    
void Evaluator::evaluateBody(StorageLocationPtr resultsl, const BSQType* restype, Argument resarg)
{
    this->evaluateOpCodeBlocks();
    restype->storeValue(resultsl, this->evalArgument(resarg));
}

void Evaluator::invoke(const BSQInvokeDecl* call, const std::vector<Argument>& args, StorageLocationPtr resultsl, BSQBool* optmask)
{
    if(call->isPrimitive())
    {
        std::vector<StorageLocationPtr> pv(args.size(), (StorageLocationPtr)nullptr);
        std::transform(args.cbegin(), args.cend(), pv.begin(), [this](const Argument& arg) {
            return this->evalArgument(arg);
        });

        this->evaluatePrimitiveBody((const BSQInvokePrimitiveDecl*)call, pv, resultsl, call->resultType);
    }
    else
    {
        const BSQInvokeBodyDecl* idecl = (const BSQInvokeBodyDecl*)call;

        size_t cssize = idecl->stackBytes;
        uint8_t* cstack = GCStack::allocFrame(cssize);

        for(size_t i = 0; i < args.size(); ++i)
        {
            StorageLocationPtr argv = this->evalArgument(args[i]);
            StorageLocationPtr pv = Evaluator::evalParameterInfo(idecl->paraminfo[i], cstack);
            
            idecl->params[i].ptype->storeValue(pv, argv);
        }

        size_t maskslotbytes = idecl->maskSlots * sizeof(BSQBool);
        BSQBool* maskslots = (BSQBool*)BSQ_STACK_ALLOC(maskslotbytes);
        GC_MEM_ZERO(maskslots, maskslotbytes);

        this->invokePrelude(idecl, cstack, maskslots, optmask);
        this->evaluateBody(resultsl, idecl->resultType, idecl->resultArg);
        this->invokePostlude();

        GCStack::popFrame(cssize);
    }
}

void Evaluator::vinvoke(const BSQInvokeBodyDecl* idecl, StorageLocationPtr rcvr, const std::vector<Argument>& args, StorageLocationPtr resultsl, BSQBool* optmask)
{
    size_t cssize = idecl->stackBytes;
    uint8_t* cstack = GCStack::allocFrame(cssize);
    GC_MEM_ZERO(cstack, cssize);

    StorageLocationPtr pv = Evaluator::evalParameterInfo(idecl->paraminfo[0], cstack);
    idecl->params[0].ptype->storeValue(pv, rcvr);

    for(size_t i = 1; i < args.size(); ++i)
    {
        StorageLocationPtr argv = this->evalArgument(args[i]);
        StorageLocationPtr pv = Evaluator::evalParameterInfo(idecl->paraminfo[i], cstack);
            
        idecl->params[i].ptype->storeValue(pv, argv);
    }

    size_t maskslotbytes = idecl->maskSlots * sizeof(BSQBool);
    BSQBool* maskslots = (BSQBool*)BSQ_STACK_ALLOC(maskslotbytes);
    GC_MEM_ZERO(maskslots, maskslotbytes);

    this->invokePrelude(idecl, cstack, maskslots, optmask);
    this->evaluateBody(resultsl, idecl->resultType, idecl->resultArg);
    this->invokePostlude();

    GCStack::popFrame(cssize);
}

void Evaluator::invokePrelude(const BSQInvokeBodyDecl* invk, uint8_t* cstack, uint8_t* maskslots, BSQBool* optmask)
{
#ifdef BSQ_DEBUG_BUILD
    this->pushFrame(this->computeCallIntoStepMode(), this->computeCurrentBreakpoint(), invk, cstack, optmask, maskslots, &invk->body);
#else
    this->pushFrame(invk, cstack, optmask, maskslots, &invk->body);
#endif
}
    
void Evaluator::invokePostlude()
{
    this->popFrame();
}

void Evaluator::evaluatePrimitiveBody(const BSQInvokePrimitiveDecl* invk, const std::vector<StorageLocationPtr>& params, StorageLocationPtr resultsl, const BSQType* restype)
{
    LambdaEvalThunk eethunk(this);

    switch (invk->implkey)
    {
    case BSQPrimitiveImplTag::validator_accepts: {
        BSQString str = SLPTR_LOAD_CONTENTS_AS(BSQString, params[0]);
        BSQStringForwardIterator iter(&str, 0);

        const BSQRegex* re = Evaluator::g_validators.at(invk->enclosingtype);
        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, re->nfare->test(iter));
        break;
    }
    case BSQPrimitiveImplTag::number_nattoint: {
        BSQNat nn = SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]);
        BSQ_LANGUAGE_ASSERT(nn <= (BSQNat)UINT64_MAX, &invk->srcFile, 0, "Out-of-bounds Nat to Int");
        
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, (BSQInt)nn);
        break;
    }
    case BSQPrimitiveImplTag::number_inttonat: {
        BSQInt ii = SLPTR_LOAD_CONTENTS_AS(BSQInt, params[0]);
        BSQ_LANGUAGE_ASSERT(ii >= 0, &invk->srcFile, 0, "Out-of-bounds Int to Nat");
        
        SLPTR_STORE_CONTENTS_AS(BSQNat, resultsl, (BSQNat)ii);
        break;
    }
    case BSQPrimitiveImplTag::number_nattobignat: {
        SLPTR_STORE_CONTENTS_AS(BSQBigNat, resultsl, (BSQBigNat)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_inttobigint: {
        SLPTR_STORE_CONTENTS_AS(BSQBigInt, resultsl, (BSQBigInt)SLPTR_LOAD_CONTENTS_AS(BSQInt, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_bignattonat: {
        BSQBigNat nn = SLPTR_LOAD_CONTENTS_AS(BSQBigNat, params[0]);
        BSQ_LANGUAGE_ASSERT(nn <= (BSQBigNat)UINT64_MAX, &invk->srcFile, 0, "Out-of-bounds BigNat to Nat");
        
        SLPTR_STORE_CONTENTS_AS(BSQNat, resultsl, (BSQNat)nn);
        break;
    }
    case BSQPrimitiveImplTag::number_biginttoint: {
        BSQBigInt ii = SLPTR_LOAD_CONTENTS_AS(BSQBigInt, params[0]);
        BSQ_LANGUAGE_ASSERT((BSQBigInt)INT64_MIN <= ii && ii <= (BSQBigInt)INT64_MAX, &invk->srcFile, 0, "Out-of-bounds BigInt to Int");
        
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, (BSQInt)ii);
        break;
    }
    case BSQPrimitiveImplTag::number_bignattobigint: {
        SLPTR_STORE_CONTENTS_AS(BSQBigInt, resultsl, (BSQBigInt)SLPTR_LOAD_CONTENTS_AS(BSQBigNat, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_biginttobignat: {
        BSQBigInt ii = SLPTR_LOAD_CONTENTS_AS(BSQBigInt, params[0]);
        BSQ_LANGUAGE_ASSERT(ii >= 0, &invk->srcFile, 0, "Out-of-bounds Int to Nat");
        
        SLPTR_STORE_CONTENTS_AS(BSQBigNat, resultsl, (BSQBigNat)ii);
        break;
    }
    case BSQPrimitiveImplTag::number_bignattofloat: {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, (BSQFloat)SLPTR_LOAD_CONTENTS_AS(BSQBigNat, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_bignattodecimal: {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, (BSQDecimal)SLPTR_LOAD_CONTENTS_AS(BSQBigNat, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_bignattorational: {
        BSQRational rr = {(BSQBigInt)SLPTR_LOAD_CONTENTS_AS(BSQBigNat, params[0]), 1};
        SLPTR_STORE_CONTENTS_AS(BSQRational, resultsl, rr);
        break;
    }
    case BSQPrimitiveImplTag::number_biginttofloat: {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, (BSQFloat)SLPTR_LOAD_CONTENTS_AS(BSQBigInt, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_biginttodecimal: {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, (BSQDecimal)SLPTR_LOAD_CONTENTS_AS(BSQBigInt, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_biginttorational: {
        BSQRational rr = {SLPTR_LOAD_CONTENTS_AS(BSQBigInt, params[0]), 1};
        SLPTR_STORE_CONTENTS_AS(BSQRational, resultsl, rr);
        break;
    }
    case BSQPrimitiveImplTag::number_floattobigint: {
        SLPTR_STORE_CONTENTS_AS(BSQBigInt, resultsl, (BSQBigInt)SLPTR_LOAD_CONTENTS_AS(BSQFloat, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_decimaltobigint: {
        SLPTR_STORE_CONTENTS_AS(BSQBigInt, resultsl, (BSQBigInt)SLPTR_LOAD_CONTENTS_AS(BSQDecimal, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_rationaltobigint: {
        auto rr = SLPTR_LOAD_CONTENTS_AS(BSQRational, params[0]);
        SLPTR_STORE_CONTENTS_AS(BSQBigInt, resultsl, ((BSQBigInt)rr.numerator) / ((BSQBigInt)rr.denominator));
        break;
    }
    case BSQPrimitiveImplTag::number_floattodecimal: {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, (BSQDecimal)SLPTR_LOAD_CONTENTS_AS(BSQFloat, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_floattorational: {
        BSQ_INTERNAL_ASSERT(false);
        break;
    }
    case BSQPrimitiveImplTag::number_decimaltofloat: {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, (BSQFloat)SLPTR_LOAD_CONTENTS_AS(BSQDecimal, params[0]));
        break;
    }
    case BSQPrimitiveImplTag::number_decimaltorational: {
        BSQ_INTERNAL_ASSERT(false);
        break;
    }
    case BSQPrimitiveImplTag::number_rationaltofloat: {
        auto rr = SLPTR_LOAD_CONTENTS_AS(BSQRational, params[0]);
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, ((BSQFloat)rr.numerator) / ((BSQFloat)rr.denominator));
        break;
    }
    case BSQPrimitiveImplTag::number_rationaltodecimal: {
        auto rr = SLPTR_LOAD_CONTENTS_AS(BSQRational, params[0]);
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, ((BSQDecimal)rr.numerator) / ((BSQDecimal)rr.denominator));
        break;
    }
    case BSQPrimitiveImplTag::float_floor: {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, std::floor(SLPTR_LOAD_CONTENTS_AS(BSQFloat, params[0])));
        break;
    }
    case BSQPrimitiveImplTag::decimal_floor: {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, std::floor(SLPTR_LOAD_CONTENTS_AS(BSQDecimal, params[0])));
        break;
    }
    case BSQPrimitiveImplTag::float_ceil: {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, std::ceil(SLPTR_LOAD_CONTENTS_AS(BSQFloat, params[0])));
        break;
    }
    case BSQPrimitiveImplTag::decimal_ceil: {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, std::ceil(SLPTR_LOAD_CONTENTS_AS(BSQDecimal, params[0])));
        break;
    }
    case BSQPrimitiveImplTag::float_truncate: {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, std::trunc(SLPTR_LOAD_CONTENTS_AS(BSQFloat, params[0])));
        break;
    }
    case BSQPrimitiveImplTag::decimal_truncate: {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, std::trunc(SLPTR_LOAD_CONTENTS_AS(BSQDecimal, params[0])));
        break;
    }
    case BSQPrimitiveImplTag::float_power: {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, resultsl, std::pow(SLPTR_LOAD_CONTENTS_AS(BSQFloat, params[0]), SLPTR_LOAD_CONTENTS_AS(BSQFloat, params[1])));
        break;
    }
    case BSQPrimitiveImplTag::decimal_power: {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, resultsl, std::pow(SLPTR_LOAD_CONTENTS_AS(BSQDecimal, params[0]), SLPTR_LOAD_CONTENTS_AS(BSQDecimal, params[1])));
        break;
    }
    case BSQPrimitiveImplTag::nat_mod: {
        SLPTR_STORE_CONTENTS_AS(BSQNat, resultsl, SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]) % SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]));
        break;
    }
    case BSQPrimitiveImplTag::string_empty: {
        BSQString str = SLPTR_LOAD_CONTENTS_AS(BSQString, params[0]);

        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, BSQStringImplType::empty(str));
        break;
    }
    case BSQPrimitiveImplTag::string_append: {
        BSQString res = BSQStringImplType::concat2(params[0], params[1]);

        SLPTR_STORE_CONTENTS_AS(BSQString, resultsl, res);
        break;
    }
    case BSQPrimitiveImplTag::bytebuffer_getformat: {
        BSQByteBuffer bb;
        BSQWellKnownType::g_typeByteBuffer->storeValue(&bb, params[0]);

        SLPTR_STORE_CONTENTS_AS(BSQNat, resultsl, (BSQNat)bb.format);
        break;
    }
    case BSQPrimitiveImplTag::bytebuffer_getcompression: {
        BSQByteBuffer bb;
        BSQWellKnownType::g_typeByteBuffer->storeValue(&bb, params[0]);

        SLPTR_STORE_CONTENTS_AS(BSQNat, resultsl, (BSQNat)bb.compression);
        break;
    }
    case BSQPrimitiveImplTag::datetime_create: {
        BSQString sstr = SLPTR_LOAD_CONTENTS_AS(BSQString, params[5]);

        //TODO: we need to handle larger strings as well and should probably handle interning better
        assert(IS_INLINE_STRING(&sstr)); 
        auto tziter = APIModule::s_tzdata.insert(std::string((const char*)BSQInlineString::utf8Bytes(sstr.u_inlineString), (const char*)BSQInlineString::utf8BytesEnd(sstr.u_inlineString)));

        BSQDateTime dt = {
            (uint16_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[2]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[3]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[4]),
            0, //padding
            tziter.first->c_str()
        };

        SLPTR_STORE_CONTENTS_AS(BSQDateTime, resultsl, dt);
        break;
    }
    case BSQPrimitiveImplTag::utcdatetime_create: {
        BSQUTCDateTime dt = {
            (uint16_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[2]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[3]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[4]),
            0 //padding
        };

        SLPTR_STORE_CONTENTS_AS(BSQUTCDateTime, resultsl, dt);
        break;
    }
    case BSQPrimitiveImplTag::calendardate_create: {
        BSQCalendarDate dt = {
            (uint16_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[2]),
            0
        };

        SLPTR_STORE_CONTENTS_AS(BSQCalendarDate, resultsl, dt);
        break;
    }
    case BSQPrimitiveImplTag::relativetime_create: {
        BSQRelativeTime dt = {
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]),
            0
        };

        SLPTR_STORE_CONTENTS_AS(BSQRelativeTime, resultsl, dt);
        break;
    }
    case BSQPrimitiveImplTag::isotimestamp_create: {
        BSQISOTimeStamp its = {
            (uint16_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[0]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[2]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[3]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[4]),

            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[5]),
            (uint8_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, params[6]),

            0, //padding
            0 //more padding
        };

        SLPTR_STORE_CONTENTS_AS(BSQISOTimeStamp, resultsl, its);
        break;
    }
    case BSQPrimitiveImplTag::latlongcoordinate_create: {
        BSQLatLongCoordinate llc = {
            (float)SLPTR_LOAD_CONTENTS_AS(double, params[0]),
            (float)SLPTR_LOAD_CONTENTS_AS(double, params[1])
        };

        SLPTR_STORE_CONTENTS_AS(BSQLatLongCoordinate, resultsl, llc);
        break;
    }
    case BSQPrimitiveImplTag::s_list_build_empty: {
        LIST_STORE_RESULT_EMPTY(resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_build_k: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        auto rres = BSQListOps::list_cons(lflavor, params);
        
        LIST_STORE_RESULT_REPR(rres, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_empty: {
        void* ll = LIST_LOAD_DATA(params[0]);
        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, ll == nullptr); 
        break;
    }
    case BSQPrimitiveImplTag::s_list_size: {
        void* ll = LIST_LOAD_DATA(params[0]);
        auto count = SLPTR_LOAD_HEAP_TYPE_AS(BSQListReprType, params[0])->getCount(ll);
        SLPTR_STORE_CONTENTS_AS(BSQNat, resultsl, count);
        break;
    }
    case BSQPrimitiveImplTag::s_list_set: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        auto ii = SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]);
        
        auto rr = BSQListOps::s_set_ne(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), ii, params[2]);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_push_back: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        
        auto rr = BSQListOps::s_push_back_ne(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), params[1]);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_push_front: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        
        auto rr = BSQListOps::s_push_front_ne(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), params[1]);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_insert: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        auto ii = SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]);
        
        auto rr = BSQListOps::s_insert_ne(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), ii, params[2]);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_remove: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        auto ii = SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]);
        
        auto rr = BSQListOps::s_remove_ne(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), ii);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_pop_back: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        auto ii = LIST_LOAD_REPR_TYPE(params[0])->getCount(LIST_LOAD_DATA(params[0])) - 1;
        
        auto rr = BSQListOps::s_remove_ne(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), ii);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_pop_front: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        BSQNat ii = 0;
        
        auto rr = BSQListOps::s_remove_ne(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), ii);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_reduce: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        invk->binds.at("T")->storeValue(resultsl, params[1]); //store the initial acc in the result
        BSQListOps::s_reduce_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("f"), params, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_reduce_idx: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        invk->binds.at("T")->storeValue(resultsl, params[1]); //store the initial acc in the result
        BSQListOps::s_reduce_idx_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("f"), params, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_transduce: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        const BSQListTypeFlavor& uflavor = BSQListOps::g_flavormap.at(invk->binds.at("U")->tid);
        const BSQType* envtype = invk->binds.at("E");

        BSQListOps::s_transduce_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), uflavor, envtype, invk->pcodes.at("op"), params, dynamic_cast<const BSQEphemeralListType*>(invk->resultType), resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_transduce_idx: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        const BSQListTypeFlavor& uflavor = BSQListOps::g_flavormap.at(invk->binds.at("U")->tid);
        const BSQType* envtype = invk->binds.at("E");

        BSQListOps::s_transduce_idx_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), uflavor, envtype, invk->pcodes.at("op"), params, dynamic_cast<const BSQEphemeralListType*>(invk->resultType), resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_range: {
        BSQListOps::s_range_ne(invk->binds.at("T"), params[0], params[1], params[3], resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_fill: {
        BSQListOps::s_fill_ne(invk->binds.at("T"), params[1], params[0], resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_reverse: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        auto rr = BSQListOps::s_reverse_ne(lflavor, LIST_LOAD_DATA(params[0]));
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_append: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        auto rr = BSQListOps::list_append(lflavor, LIST_LOAD_DATA(params[0]), LIST_LOAD_DATA(params[1]));
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_slice_start: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        BSQListSpineIterator liter(LIST_LOAD_REPR_TYPE(params[0]), LIST_LOAD_DATA(params[0]));
        Allocator::GlobalAllocator.insertCollectionIter(&liter);

        auto rr = BSQListOps::s_slice_start(lflavor, liter, LIST_LOAD_REPR_TYPE(params[0]), SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]));
        LIST_STORE_RESULT_REPR(rr, resultsl);
        Allocator::GlobalAllocator.removeCollectionIter(&liter);
        break;
    }
    case BSQPrimitiveImplTag::s_list_slice_end: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        BSQListSpineIterator liter(LIST_LOAD_REPR_TYPE(params[0]), LIST_LOAD_DATA(params[0]));
        Allocator::GlobalAllocator.insertCollectionIter(&liter);

        auto rr = BSQListOps::s_slice_end(lflavor, liter, LIST_LOAD_REPR_TYPE(params[0]), SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]));
        LIST_STORE_RESULT_REPR(rr, resultsl);
        Allocator::GlobalAllocator.removeCollectionIter(&liter);
        break;
    }
    case BSQPrimitiveImplTag::s_list_slice: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        BSQListSpineIterator siter(LIST_LOAD_REPR_TYPE(params[0]), LIST_LOAD_DATA(params[0]));
        Allocator::GlobalAllocator.insertCollectionIter(&siter);

        auto rr = BSQListOps::s_slice(lflavor, siter, LIST_LOAD_REPR_TYPE(params[0]), SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]), SLPTR_LOAD_CONTENTS_AS(BSQNat, params[2]));
        LIST_STORE_RESULT_REPR(rr, resultsl);
        
        Allocator::GlobalAllocator.removeCollectionIter(&siter);
        break;
    }
    case BSQPrimitiveImplTag::s_list_get: {
        BSQListOps::s_safe_get(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), SLPTR_LOAD_CONTENTS_AS(BSQNat, params[1]), invk->binds.at("T"), resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_back: {
        auto ii = LIST_LOAD_REPR_TYPE(params[0])->getCount(LIST_LOAD_DATA(params[0])) - 1;
        BSQListOps::s_safe_get(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), ii, invk->binds.at("T"), resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_front: {
        BSQNat ii = 0;
        BSQListOps::s_safe_get(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), ii, invk->binds.at("T"), resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_has_pred: {
        auto pos = BSQListOps::s_find_pred_ne(eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, pos != -1);
        break;
    }
    case BSQPrimitiveImplTag::s_list_has_pred_idx: {
        auto pos = BSQListOps::s_find_pred_idx_ne(eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, pos != -1);
        break;
    }
    case BSQPrimitiveImplTag::s_list_find_pred: {
        auto pos = BSQListOps::s_find_pred_ne(eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, pos);
        break;
    }
    case BSQPrimitiveImplTag::s_list_find_pred_idx: {
        auto pos = BSQListOps::s_find_pred_idx_ne(eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, pos);
        break;
    }
    case BSQPrimitiveImplTag::s_list_find_pred_last: {
        auto pos = BSQListOps::s_find_pred_last_ne(eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, pos);
        break;
    }
    case BSQPrimitiveImplTag::s_list_find_pred_last_idx: {
        auto pos = BSQListOps::s_find_pred_last_idx_ne(eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, pos);
        break;
    }
    case BSQPrimitiveImplTag::s_list_single_index_of: {
        auto posfirst = BSQListOps::s_find_value_ne(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), params[1]);
        auto poslast = BSQListOps::s_find_value_last_ne(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), params[1]);
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, (posfirst == poslast ? posfirst : -1));
        break;
    }
    case BSQPrimitiveImplTag::s_list_has: {
        auto pos = BSQListOps::s_find_value_ne(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), params[1]);
        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, pos != -1);
        break;
    }
    case BSQPrimitiveImplTag::s_list_indexof: {
        auto pos = BSQListOps::s_find_value_ne(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), params[1]);
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, pos);
        break;
    }
    case BSQPrimitiveImplTag::s_list_last_indexof: {
        auto pos = BSQListOps::s_find_value_last_ne(LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), params[1]);
        SLPTR_STORE_CONTENTS_AS(BSQInt, resultsl, pos);
        break;
    }
    case BSQPrimitiveImplTag::s_list_filter_pred: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        auto rr = BSQListOps::s_filter_pred_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_filter_pred_idx: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);

        auto rr = BSQListOps::s_filter_pred_idx_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_map: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        const BSQListTypeFlavor& rflavor = BSQListOps::g_flavormap.at(invk->binds.at("U")->tid);

        auto rr = BSQListOps::s_map_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("f"), params, rflavor);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_map_idx: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        const BSQListTypeFlavor& rflavor = BSQListOps::g_flavormap.at(invk->binds.at("U")->tid);

        auto rr = BSQListOps::s_map_idx_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("f"), params, rflavor);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_map_sync: {
        const BSQListTypeFlavor& lflavor1 = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        const BSQListTypeFlavor& lflavor2 = BSQListOps::g_flavormap.at(invk->binds.at("U")->tid);
        const BSQListTypeFlavor& rflavor = BSQListOps::g_flavormap.at(invk->binds.at("V")->tid);

        auto rr = BSQListOps::s_map_sync_ne(lflavor1, lflavor2, eethunk, SLPTR_LOAD_CONTENTS_AS(BSQNat, params[3]), LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), LIST_LOAD_DATA(params[1]), LIST_LOAD_REPR_TYPE(params[1]), invk->pcodes.at("f"), params, rflavor);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_list_filter_map_fn: {
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->binds.at("T")->tid);
        const BSQListTypeFlavor& rflavor = BSQListOps::g_flavormap.at(invk->binds.at("U")->tid);

        auto rr = BSQListOps::s_filter_map_ne(lflavor, eethunk, LIST_LOAD_DATA(params[0]), LIST_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("f"), invk->pcodes.at("p"), params, rflavor);
        LIST_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_build_empty: {
        MAP_STORE_RESULT_EMPTY(resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_build_1: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));
        
        auto rr = BSQMapOps::map_cons_one_element(mflavor, invk->params[0].ptype, params);
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_empty: {
        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, MAP_LOAD_DATA(params[0]) == nullptr);
        break;
    }
    case BSQPrimitiveImplTag::s_map_count: {
        SLPTR_STORE_CONTENTS_AS(BSQNat, resultsl, ((BSQMapTreeRepr*)MAP_LOAD_DATA(params[0]))->tcount);
        break;
    }
    case BSQPrimitiveImplTag::s_map_entries: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(invk->resultType->tid);

        auto rr = BSQMapOps::s_entries_ne(mflavor, MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), lflavor);
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_min_key: {
        auto ttype = MAP_LOAD_REPR_TYPE(params[0]);
        auto rr = ttype->minElem(MAP_LOAD_DATA(params[0]));
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_max_key: {
        auto ttype = MAP_LOAD_REPR_TYPE(params[0]);
        auto rr = ttype->maxElem(MAP_LOAD_DATA(params[0]));
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_has: {
        auto rr = BSQMapOps::s_lookup_ne(MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), params[1], invk->binds.at("K"));
        SLPTR_STORE_CONTENTS_AS(BSQBool, resultsl, (BSQBool)(rr != nullptr));
        break;
    }
    case BSQPrimitiveImplTag::s_map_get: {
        auto ttype = MAP_LOAD_REPR_TYPE(params[0]);
        auto rr = BSQMapOps::s_lookup_ne(MAP_LOAD_DATA(params[0]), ttype, params[1], invk->binds.at("K"));
        BSQ_INTERNAL_ASSERT(rr != nullptr);

        invk->binds.at("V")->storeValue(resultsl, ttype->getValueLocation(rr));
        break;
    }
    case BSQPrimitiveImplTag::s_map_find: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));

        auto ttype = MAP_LOAD_REPR_TYPE(params[0]);
        auto rr = BSQMapOps::s_lookup_ne(MAP_LOAD_DATA(params[0]), ttype, params[1], invk->binds.at("K"));

        void* value = (void*)resultsl;
        BSQBool* flag = (BSQBool*)((uint8_t*)resultsl + mflavor.valuetype->allocinfo.inlinedatasize);
        if(rr == nullptr)
        {
            *flag = BSQFALSE;
        }
        else
        {
            *flag = BSQTRUE;
            GC_MEM_COPY(value, ttype->getValueLocation(rr), mflavor.valuetype->allocinfo.inlinedatasize);
        }
        break;
    }
    case BSQPrimitiveImplTag::s_map_union_fast: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));

        //TODO: we don't have a fast now 
        auto rr = BSQMapOps::s_fast_union_ne(mflavor, MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), MAP_LOAD_DATA(params[1]), MAP_LOAD_REPR_TYPE(params[1]));
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_submap: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));

        auto rr = BSQMapOps::s_submap_ne(mflavor, eethunk, MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("p"), params);
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_remap: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));
        const BSQMapTypeFlavor& rflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("U")->tid));

        auto rr = BSQMapOps::s_remap_ne(mflavor, eethunk, MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), invk->pcodes.at("f"), params, rflavor);
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_add: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));

        auto rr = BSQMapOps::s_add_ne(mflavor, MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), params[1], params[2]);
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_set: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));

        auto rr = BSQMapOps::s_set_ne(mflavor, MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), params[1], params[2]);
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    case BSQPrimitiveImplTag::s_map_remove: {
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(invk->binds.at("K")->tid, invk->binds.at("V")->tid));

        auto rr = BSQMapOps::s_remove_ne(mflavor, MAP_LOAD_DATA(params[0]), MAP_LOAD_REPR_TYPE(params[0]), params[1]);
        MAP_STORE_RESULT_REPR(rr, resultsl);
        break;
    }
    default: {
        assert(false);
        break;
    }
    }
}

void Evaluator::invokeGlobalCons(const BSQInvokeBodyDecl* invk, StorageLocationPtr resultsl, const BSQType* restype, Argument resarg)
{
    size_t cssize = invk->stackBytes;
    uint8_t* cstack = (uint8_t*)GCStack::allocFrame(cssize);
    GC_MEM_ZERO(cstack, cssize);

    size_t maskslotbytes = invk->maskSlots * sizeof(BSQBool);
    BSQBool* maskslots = (BSQBool*)BSQ_STACK_ALLOC(maskslotbytes);
    GC_MEM_ZERO(maskslots, maskslotbytes);


    this->invokePrelude(invk, cstack, maskslots, nullptr);
    this->evaluateBody(resultsl, restype, resarg);
    this->invokePostlude();

    GCStack::popFrame(cssize);
}

void Evaluator::invokeMain(const BSQInvokeBodyDecl* invk, uint8_t* istack, StorageLocationPtr resultsl, const BSQType* restype, Argument resarg)
{
    size_t cssize = invk->stackBytes;
    uint8_t* cstack = (uint8_t*)GCStack::allocFrame(cssize);
    GC_MEM_COPY(cstack, istack, cssize);

    size_t maskslotbytes = invk->maskSlots * sizeof(BSQBool);
    BSQBool* maskslots = (BSQBool*)BSQ_STACK_ALLOC(maskslotbytes);
    GC_MEM_ZERO(maskslots, maskslotbytes);

    this->invokePrelude(invk, cstack, maskslots, nullptr);
    this->evaluateBody(resultsl, restype, resarg);
    this->invokePostlude();

    GCStack::popFrame(cssize);
}

void Evaluator::linvoke(const BSQInvokeBodyDecl* call, const std::vector<StorageLocationPtr>& args, StorageLocationPtr resultsl)
{
    size_t cssize = call->stackBytes;
    uint8_t* cstack = (uint8_t*)GCStack::allocFrame(cssize);
    GC_MEM_ZERO(cstack, cssize);

    for(size_t i = 0; i < args.size(); ++i)
    {
        StorageLocationPtr pv = Evaluator::evalParameterInfo(call->paraminfo[i], cstack);
        call->params[i].ptype->storeValue(pv, args[i]);
    }

    size_t maskslotbytes = call->maskSlots * sizeof(BSQBool);
    BSQBool* maskslots = (BSQBool*)BSQ_STACK_ALLOC(maskslotbytes);
    GC_MEM_ZERO(maskslots, maskslotbytes);

    this->invokePrelude(call, cstack, maskslots, nullptr);
    this->evaluateBody(resultsl, call->resultType, call->resultArg);
    this->invokePostlude();

    GCStack::popFrame(cssize);
}

bool Evaluator::iinvoke(const BSQInvokeBodyDecl* call, const std::vector<StorageLocationPtr>& args, BSQBool* optmask)
{
    size_t cssize = call->stackBytes;
    uint8_t* cstack = (uint8_t*)GCStack::allocFrame(cssize);
    GC_MEM_ZERO(cstack, cssize);

    for(size_t i = 0; i < args.size(); ++i)
    {
        StorageLocationPtr pv = Evaluator::evalParameterInfo(call->paraminfo[i], cstack);
        call->params[i].ptype->storeValue(pv, args[i]);
    }

    size_t maskslotbytes = call->maskSlots * sizeof(BSQBool);
    BSQBool* maskslots = (BSQBool*)BSQ_STACK_ALLOC(maskslotbytes);
    GC_MEM_ZERO(maskslots, maskslotbytes);

    BSQBool ok = BSQFALSE;
    this->invokePrelude(call, cstack, maskslots, optmask);
    this->evaluateBody(&ok, call->resultType, call->resultArg);
    this->invokePostlude();

    GCStack::popFrame(cssize);
    return (bool)ok;
}

void Evaluator::cinvoke(const BSQInvokeBodyDecl* call, const std::vector<StorageLocationPtr>& args, BSQBool* optmask, StorageLocationPtr resultsl)
{
    size_t cssize = call->stackBytes;
    uint8_t* cstack = (uint8_t*)GCStack::allocFrame(cssize);
    GC_MEM_ZERO(cstack, cssize);

    for(size_t i = 0; i < args.size(); ++i)
    {
        StorageLocationPtr pv = Evaluator::evalParameterInfo(call->paraminfo[i], cstack);
        call->params[i].ptype->storeValue(pv, args[i]);
    }

    size_t maskslotbytes = call->maskSlots * sizeof(BSQBool);
    BSQBool* maskslots = (BSQBool*)BSQ_STACK_ALLOC(maskslotbytes);
    GC_MEM_ZERO(maskslots, maskslotbytes);

    this->invokePrelude(call, cstack, maskslots, optmask);
    this->evaluateBody(resultsl, call->resultType, call->resultArg);
    this->invokePostlude();

    GCStack::popFrame(cssize);
}

void LambdaEvalThunk::invoke(const BSQInvokeBodyDecl* call, const std::vector<StorageLocationPtr>& args, StorageLocationPtr resultsl)
{
    static_cast<Evaluator*>(this->ctx)->linvoke(call, args, resultsl);
}

bool ICPPParseJSON::checkInvokeOk(const std::string& checkinvoke, StorageLocationPtr value, Evaluator& ctx)
{
    auto invkid = MarshalEnvironment::g_invokeToIdMap.at(checkinvoke);
    auto invk = dynamic_cast<const BSQInvokeBodyDecl*>(BSQInvokeDecl::g_invokes[invkid]);

    std::vector<StorageLocationPtr> args = {value};
    BSQBool bb = BSQFALSE;
    ctx.linvoke(invk, args, &bb);

    return bb;
}

bool ICPPParseJSON::parseNoneImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    //location is aleady zero initialized
    return true;
}

bool ICPPParseJSON::parseNothingImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    //location is aleady zero initialized
    return true;
}

bool ICPPParseJSON::parseBoolImpl(const APIModule* apimodule, const IType* itype, bool b, StorageLocationPtr value, Evaluator& ctx)
{
    SLPTR_STORE_CONTENTS_AS(BSQBool, value, (BSQBool)(b ? BSQTRUE : BSQFALSE));
    return true;
}

bool ICPPParseJSON::parseNatImpl(const APIModule* apimodule, const IType* itype, uint64_t n, StorageLocationPtr value, Evaluator& ctx)
{
    SLPTR_STORE_CONTENTS_AS(BSQNat, value, (BSQNat)n);
    return true;
}

bool ICPPParseJSON::parseIntImpl(const APIModule* apimodule, const IType* itype, int64_t i, StorageLocationPtr value, Evaluator& ctx)
{
    SLPTR_STORE_CONTENTS_AS(BSQInt, value, (BSQInt)i);
    return true;
}

bool ICPPParseJSON::parseBigNatImpl(const APIModule* apimodule, const IType* itype, std::string n, StorageLocationPtr value, Evaluator& ctx)
{
    try
    {
        SLPTR_STORE_CONTENTS_AS(BSQBigNat, value, (BSQBigNat)std::stoull(n));
        return true;
    }
    catch(...)
    {
        ;
    }

    return false;
}

bool ICPPParseJSON::parseBigIntImpl(const APIModule* apimodule, const IType* itype, std::string i, StorageLocationPtr value, Evaluator& ctx)
{
    try
    {
        SLPTR_STORE_CONTENTS_AS(BSQBigInt, value, (BSQBigInt)std::stoll(i));
        return true;
    }
    catch(...)
    {
        ;
    }

    return false;
}

bool ICPPParseJSON::parseFloatImpl(const APIModule* apimodule, const IType* itype, std::string f, StorageLocationPtr value, Evaluator& ctx)
{
    try
    {
        SLPTR_STORE_CONTENTS_AS(BSQFloat, value, (BSQFloat)std::stod(f));
        return true;
    }
    catch(...)
    {
        ;
    }

    return false;
}

bool ICPPParseJSON::parseDecimalImpl(const APIModule* apimodule, const IType* itype, std::string d, StorageLocationPtr value, Evaluator& ctx)
{
    try
    {
        SLPTR_STORE_CONTENTS_AS(BSQDecimal, value, (BSQDecimal)std::stod(d));
        return true;
    }
    catch(...)
    {
        ;
    }

    return false;
}

bool ICPPParseJSON::parseRationalImpl(const APIModule* apimodule, const IType* itype, std::string n, uint64_t d, StorageLocationPtr value, Evaluator& ctx)
{
    try
    {
        BSQRational rv = {(BSQBigInt) std::stoll(n), (BSQNat)d};
        SLPTR_STORE_CONTENTS_AS(BSQRational, value, rv);
        return true;
    }
    catch(...)
    {
        ;
    }

    return false;
}

bool ICPPParseJSON::parseStringImpl(const APIModule* apimodule, const IType* itype, std::string s, StorageLocationPtr value, Evaluator& ctx)
{
    BSQString rstr = g_emptyString;
    if(s.empty())
    {
        //already empty
    }
    else if(s.size() < 16) 
    {
        rstr.u_inlineString = BSQInlineString::create((const uint8_t*)s.c_str(), s.size());
    }
    else if(s.size() <= 128)
    {
        auto stp = BSQStringKReprTypeAbstract::selectKReprForSize(s.size());

        rstr.u_data = Allocator::GlobalAllocator.allocateDynamic(stp);
        BSQ_MEM_COPY(rstr.u_data, s.c_str(), s.size());
    }
    else
    {
        //
        //TODO: split the string into multiple parts
        //
         assert(false);
    }
    
    SLPTR_STORE_CONTENTS_AS(BSQString, value, rstr);
    return true;
}

bool ICPPParseJSON::parseByteBufferImpl(const APIModule* apimodule, const IType* itype, uint8_t compress, uint8_t format, std::vector<uint8_t>& data, StorageLocationPtr value, Evaluator& ctx)
{
    assert(false);
    return false;

    /*
    Allocator::GlobalAllocator.ensureSpace(BSQWellKnownType::g_typeByteBufferLeaf->allocinfo.heapsize + BSQWellKnownType::g_typeByteBufferNode->allocinfo.heapsize + (2 * sizeof(GC_META_DATA_WORD)));
    BSQByteBufferNode* cnode = (BSQByteBufferNode*)Allocator::GlobalAllocator.allocateSafe(BSQWellKnownType::g_typeByteBufferNode);
    cnode->bytecount = std::min((uint64_t)data.size(), (uint64_t)256);
    cnode->bytes = (BSQByteBufferLeaf*)Allocator::GlobalAllocator.allocateSafe(BSQWellKnownType::g_typeByteBufferLeaf);
    cnode->next = nullptr;

    void* croot = cnode;
    GCStack::pushFrame(&croot, "2");

    size_t i = 0;
    while(i < data.size())
    {
        std::copy(data.cbegin() + i, data.cbegin() + i + cnode->bytecount, cnode->bytes->bytes);
        i += cnode->bytecount;

        if(i < data.size())
        {
            Allocator::GlobalAllocator.ensureSpace(BSQWellKnownType::g_typeByteBufferLeaf->allocinfo.heapsize + BSQWellKnownType::g_typeByteBufferNode->allocinfo.heapsize + (2 * sizeof(GC_META_DATA_WORD)));
            BSQByteBufferNode* cnode = (BSQByteBufferNode*)Allocator::GlobalAllocator.allocateSafe(BSQWellKnownType::g_typeByteBufferNode);
            cnode->bytecount = std::min((uint64_t)data.size(), (uint64_t)256);
            cnode->bytes = (BSQByteBufferLeaf*)Allocator::GlobalAllocator.allocateSafe(BSQWellKnownType::g_typeByteBufferLeaf);
            cnode->next = (BSQByteBufferNode*)croot;

            croot = cnode;
        }
    }

    BSQByteBufferNode* nnode = (BSQByteBufferNode*)croot;

    while(nnode->next != nullptr)
    {
        auto nn = nnode->next;
        nnode->next = nnode;
        nnode = nn;
    }

    Allocator::GlobalAllocator.ensureSpace(BSQWellKnownType::g_typeByteBuffer->allocinfo.heapsize + sizeof(GC_META_DATA_WORD));
    BSQByteBuffer* buff = (BSQByteBuffer*)Allocator::GlobalAllocator.allocateSafe(BSQWellKnownType::g_typeByteBufferNode);
    buff->bytecount = data.size();
    buff->bytes = nnode;
    buff->compression = (BufferCompression)compress;
    buff->format = (BufferFormat)format;

    SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(value, buff);

    GCStack::popFrame();
    return true;
    */
}

bool ICPPParseJSON::parseDateTimeImpl(const APIModule* apimodule, const IType* itype, APIDateTime t, StorageLocationPtr value, Evaluator& ctx)
{
    BSQDateTime dt = {0};
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;
    dt.hour = t.hour;
    dt.min = t.min;
    dt.tzdata = t.tzdata;

    SLPTR_STORE_CONTENTS_AS(BSQDateTime, value, dt);
    return true;
}

bool ICPPParseJSON::parseUTCDateTimeImpl(const APIModule* apimodule, const IType* itype, APIUTCDateTime t, StorageLocationPtr value, Evaluator& ctx)
{
    BSQUTCDateTime dt = {0};
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;
    dt.hour = t.hour;
    dt.min = t.min;

    SLPTR_STORE_CONTENTS_AS(BSQUTCDateTime, value, dt);
    return true;
}

bool ICPPParseJSON::parseCalendarDateImpl(const APIModule* apimodule, const IType* itype, APICalendarDate t, StorageLocationPtr value, Evaluator& ctx)
{
    BSQCalendarDate dt = {0};
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;

    SLPTR_STORE_CONTENTS_AS(BSQCalendarDate, value, dt);
    return true;
}

bool ICPPParseJSON::parseRelativeTimeImpl(const APIModule* apimodule, const IType* itype, APIRelativeTime t, StorageLocationPtr value, Evaluator& ctx)
{
    BSQRelativeTime dt = {0};
   
    dt.hour = t.hour;
    dt.min = t.min;

    SLPTR_STORE_CONTENTS_AS(BSQRelativeTime, value, dt);
    return true;
}

bool ICPPParseJSON::parseTickTimeImpl(const APIModule* apimodule, const IType* itype, uint64_t t, StorageLocationPtr value, Evaluator& ctx)
{
    SLPTR_STORE_CONTENTS_AS(BSQTickTime, value, (BSQTickTime)t);
    return true;
}

bool ICPPParseJSON::parseLogicalTimeImpl(const APIModule* apimodule, const IType* itype, uint64_t j, StorageLocationPtr value, Evaluator& ctx)
{
    SLPTR_STORE_CONTENTS_AS(BSQLogicalTime, value, (BSQLogicalTime)j);
    return true;
}

bool ICPPParseJSON::parseISOTimeStampImpl(const APIModule* apimodule, const IType* itype, APIISOTimeStamp t, StorageLocationPtr value, Evaluator& ctx)
{
    BSQISOTimeStamp dt = {0};
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;
    dt.hour = t.hour;
    dt.min = t.min;
    dt.seconds = t.sec;
    dt.millis = t.millis;

    SLPTR_STORE_CONTENTS_AS(BSQISOTimeStamp, value, dt);
    return true;
}

bool ICPPParseJSON::parseUUID4Impl(const APIModule* apimodule, const IType* itype, std::vector<uint8_t> v, StorageLocationPtr value, Evaluator& ctx)
{
    BSQUUID uuid;
    std::copy(v.cbegin(), v.cbegin() + 16, uuid.bytes);
    
    SLPTR_STORE_CONTENTS_AS(BSQUUID, value, uuid);
    return true;
}

bool ICPPParseJSON::parseUUID7Impl(const APIModule* apimodule, const IType* itype, std::vector<uint8_t> v, StorageLocationPtr value, Evaluator& ctx)
{
    BSQUUID uuid;
    std::copy(v.cbegin(), v.cbegin() + 16, uuid.bytes);
    
    SLPTR_STORE_CONTENTS_AS(BSQUUID, value, uuid);
    return true;
}

bool ICPPParseJSON::parseSHAContentHashImpl(const APIModule* apimodule, const IType* itype, std::vector<uint8_t> v, StorageLocationPtr value, Evaluator& ctx)
{
    BSQSHAContentHash* hash = (BSQSHAContentHash*)Allocator::GlobalAllocator.allocateDynamic(BSQWellKnownType::g_typeSHAContentHash);
    std::copy(v.cbegin(), v.cbegin() + 64, hash->bytes);
    
    SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(value, hash);
    return true;
}

bool ICPPParseJSON::parseLatLongCoordinateImpl(const APIModule* apimodule, const IType* itype, float latitude, float longitude, StorageLocationPtr value, Evaluator& ctx)
{
    BSQLatLongCoordinate llc = {0};
    llc.latitude = latitude;
    llc.longitude = longitude;

    SLPTR_STORE_CONTENTS_AS(BSQLatLongCoordinate, value, llc);
    return true;
}

void ICPPParseJSON::prepareParseTuple(const APIModule* apimodule, const IType* itype, Evaluator& ctx)
{
    BSQTypeID tupid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* tuptype = BSQType::g_typetable[tupid];

    void* tupmem = nullptr;
    if(tuptype->tkind == BSQTypeLayoutKind::Struct)
    {
        tupmem = GCStack::allocFrame(tuptype->allocinfo.inlinedatasize);
    }
    else
    {
        tupmem = GCStack::allocFrame(tuptype->allocinfo.heapsize);
    }
    
    
    this->tuplestack.push_back(std::make_pair(tupmem, tuptype));
}

StorageLocationPtr ICPPParseJSON::getValueForTupleIndex(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t i, Evaluator& ctx)
{
    void* tupmem = this->tuplestack.back().first;
    const BSQType* tuptype = this->tuplestack.back().second;

    return SLPTR_INDEX_DATAPTR(tupmem, dynamic_cast<const BSQTupleInfo*>(tuptype)->idxoffsets[i]);
}

void ICPPParseJSON::completeParseTuple(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    void* tupmem = this->tuplestack.back().first;
    const BSQType* tuptype = this->tuplestack.back().second;

    void* trgt = nullptr;
    uint64_t bytes = 0;
    if(tuptype->tkind == BSQTypeLayoutKind::Struct)
    {
        trgt = (void*)value;
        bytes = tuptype->allocinfo.inlinedatasize;
    }
    else
    {
        trgt = Allocator::GlobalAllocator.allocateDynamic(tuptype);
        bytes = tuptype->allocinfo.heapsize;

        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(value, trgt);
    }

    GC_MEM_COPY(trgt, tupmem, bytes);

    GCStack::popFrame(bytes);
    this->tuplestack.pop_back();
}

void ICPPParseJSON::prepareParseRecord(const APIModule* apimodule, const IType* itype, Evaluator& ctx)
{
    BSQTypeID recid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* rectype = BSQType::g_typetable[recid];

    void* recmem = nullptr;
    if(rectype->tkind == BSQTypeLayoutKind::Struct)
    {
        recmem = GCStack::allocFrame(rectype->allocinfo.inlinedatasize);
    }
    else
    {
        recmem = GCStack::allocFrame(rectype->allocinfo.heapsize);
    }
    
    this->recordstack.push_back(std::make_pair(recmem, rectype));
}

StorageLocationPtr ICPPParseJSON::getValueForRecordProperty(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, std::string pname, Evaluator& ctx)
{
    void* recmem = this->recordstack.back().first;
    const BSQType* rectype = this->recordstack.back().second;

    BSQRecordPropertyID pid = MarshalEnvironment::g_propertyToIdMap.at(pname);
    return SLPTR_INDEX_DATAPTR(recmem, dynamic_cast<const BSQRecordInfo*>(rectype)->propertyoffsets[pid]);
}

void ICPPParseJSON::completeParseRecord(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    void* recmem = this->recordstack.back().first;
    const BSQType* rectype = this->recordstack.back().second;

    void* trgt = nullptr;
    uint64_t bytes = 0;
    if(rectype->tkind == BSQTypeLayoutKind::Struct)
    {
        trgt = (void*)value;
        bytes = rectype->allocinfo.inlinedatasize;
    }
    else
    {
        trgt = Allocator::GlobalAllocator.allocateDynamic(rectype);
        bytes = rectype->allocinfo.heapsize;

        SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(value, trgt);
    }

    GC_MEM_COPY(trgt, recmem, bytes);

    GCStack::popFrame(bytes);
    this->tuplestack.pop_back();
}

void ICPPParseJSON::prepareParseContainer(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t count, Evaluator& ctx)
{
    BSQTypeID containertypeid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* collectiontype = BSQType::g_typetable[containertypeid];

    //TODO: right now we just assume we can stack alloc this space -- later we want to add special heap allocated frame support (like for global the object)
    uint8_t* recmem = nullptr;

    if(itype->tag == TypeTag::ContainerTTag)
    {
        auto ctype = dynamic_cast<const ContainerTType*>(itype);

        if(ctype->category == ContainerCategory::List)
        {
            auto ttype = BSQType::g_typetable[dynamic_cast<const BSQListType*>(collectiontype)->etype];
            recmem = GCStack::allocFrame(count * ttype->allocinfo.inlinedatasize);
        }
        else if(ctype->category == ContainerCategory::Stack)
        {
            BSQ_INTERNAL_ASSERT(false);
        }
        else if(ctype->category == ContainerCategory::Queue)
        {
            BSQ_INTERNAL_ASSERT(false);
        }
        else
        {
            BSQ_INTERNAL_ASSERT(false);
        }
    }
    else
    {
        auto ktype = BSQType::g_typetable[dynamic_cast<const BSQMapType*>(collectiontype)->ktype];
        auto vtype = BSQType::g_typetable[dynamic_cast<const BSQMapType*>(collectiontype)->vtype];

        recmem = GCStack::allocFrame(count * (ktype->allocinfo.inlinedatasize + vtype->allocinfo.inlinedatasize));
    }

    this->containerstack.push_back(std::make_pair(collectiontype, std::make_pair(recmem, (uint64_t)count)));
}

StorageLocationPtr ICPPParseJSON::getValueForContainerElementParse_T(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t i, Evaluator& ctx)
{
    const ContainerTType* ctype = dynamic_cast<const ContainerTType*>(itype);
    
    if(ctype->category == ContainerCategory::List)
    {
        const BSQListType* listtype = dynamic_cast<const BSQListType*>(this->containerstack.back().first);
        const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(listtype->etype);

        return (StorageLocationPtr)(this->containerstack.back().second.first + (i * lflavor.entrytype->allocinfo.inlinedatasize));
    }
    else if(ctype->category == ContainerCategory::Stack)
    {
        BSQ_INTERNAL_ASSERT(false);
        return nullptr;
    }
    else if(ctype->category == ContainerCategory::Queue)
    {
        BSQ_INTERNAL_ASSERT(false);
        return nullptr;
    }
    else
    {
        BSQ_INTERNAL_ASSERT(false);
        return nullptr;
    }
}

std::pair<StorageLocationPtr, StorageLocationPtr> ICPPParseJSON::getValueForContainerElementParse_KV(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t i, Evaluator& ctx)
{
    const BSQMapType* maptype = dynamic_cast<const BSQMapType*>(this->containerstack.back().first);
    const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(maptype->ktype, maptype->vtype));

    size_t ecount = (i * (mflavor.keytype->allocinfo.inlinedatasize + mflavor.valuetype->allocinfo.inlinedatasize));
    auto kloc = (StorageLocationPtr)(this->containerstack.back().second.first + ecount);
    auto vloc = (StorageLocationPtr)(this->containerstack.back().second.first + ecount + mflavor.keytype->allocinfo.inlinedatasize);
    
    return std::make_pair(kloc, vloc);
}

void ICPPParseJSON::completeParseContainer(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    if(itype->tag == TypeTag::ContainerTTag)
    {
        const ContainerTType* ctype = dynamic_cast<const ContainerTType*>(itype);

        if(ctype->category == ContainerCategory::List)
        {
            const BSQListType* listtype = dynamic_cast<const BSQListType*>(this->containerstack.back().first);
            const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(listtype->etype);

            if(this->containerstack.back().second.second == 0)
            {
                LIST_STORE_RESULT_EMPTY(value);
            }
            else
            {
                std::vector<StorageLocationPtr> params;
                for(size_t i = 0; i < this->containerstack.back().second.second; ++i)
                {
                    params.push_back(this->containerstack.back().second.first + (i * lflavor.entrytype->allocinfo.inlinedatasize));
                }

                void* rres = nullptr;
                if(params.size() <= 8)
                {
                    rres = BSQListOps::list_consk(lflavor, params);
                }
                else
                {
                    assert(false);
                    rres = nullptr;
                }

                LIST_STORE_RESULT_REPR(&rres, value);
            }

            GCStack::allocFrame(this->containerstack.back().second.second * lflavor.entrytype->allocinfo.inlinedatasize);
        }
        else if(ctype->category == ContainerCategory::Stack)
        {
            BSQ_INTERNAL_ASSERT(false);
        }
        else if(ctype->category == ContainerCategory::Queue)
        {
            BSQ_INTERNAL_ASSERT(false);
        }
        else if(ctype->category == ContainerCategory::Set)
        {
            BSQ_INTERNAL_ASSERT(false);
        }
    }
    else
    {
        const BSQMapType* maptype = dynamic_cast<const BSQMapType*>(this->containerstack.back().first);
        const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(maptype->ktype, maptype->vtype));

        if(this->containerstack.back().second.second == 0)
        {
            MAP_STORE_RESULT_EMPTY(value);
        }
        else if(this->containerstack.back().second.second == 1)
        {
            BSQMapTreeRepr* mtr = (BSQMapTreeRepr*)Allocator::GlobalAllocator.allocateDynamic(mflavor.treetype);
            mflavor.treetype->initializeLeaf(mtr, mflavor.treetype->getKeyLocation(mtr), mflavor.keytype, mflavor.treetype->getValueLocation(mtr), mflavor.valuetype);

            MAP_STORE_RESULT_REPR(mtr, value);
        }
        else
        {
            assert(false);
        }

        GCStack::allocFrame(this->containerstack.back().second.second * (mflavor.keytype->allocinfo.inlinedatasize + mflavor.keytype->allocinfo.inlinedatasize));
    }
    
    this->containerstack.pop_back();
}

void ICPPParseJSON::prepareParseEntity(const APIModule* apimodule, const IType* itype, Evaluator& ctx)
{
    BSQTypeID ooid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* ootype = BSQType::g_typetable[ooid];

    void* oomem = nullptr;
    if(ootype->tkind == BSQTypeLayoutKind::Struct)
    {
        oomem = GCStack::allocFrame(ootype->allocinfo.inlinedatasize);
    }
    else
    {
        oomem = GCStack::allocFrame(ootype->allocinfo.heapsize);
    }
    
    this->entitystack.push_back(std::make_pair(oomem, ootype));
}

void ICPPParseJSON::prepareParseEntityMask(const APIModule* apimodule, const IType* itype, Evaluator& ctx)
{
    BSQTypeID ooid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQEntityInfo* ootype = dynamic_cast<const BSQEntityInfo*>(BSQType::g_typetable[ooid]);

    auto mcount = std::count_if(ootype->fields.cbegin(), ootype->fields.cend(), [](BSQFieldID fid) {
        auto fdecl = BSQField::g_fieldtable[fid];
        return fdecl->isOptional;
    });

    BSQBool* mask = (BSQBool*)zxalloc(mcount * sizeof(BSQBool));
    this->entitymaskstack.push_back(mask);
}

StorageLocationPtr ICPPParseJSON::getValueForEntityField(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, std::pair<std::string, std::string> fnamefkey, Evaluator& ctx)
{
    void* oomem = this->entitystack.back().first;
    const BSQType* ootype = this->entitystack.back().second;

    BSQFieldID fid = MarshalEnvironment::g_fieldToIdMap.at(fnamefkey.second);
    return SLPTR_INDEX_DATAPTR(oomem, dynamic_cast<const BSQEntityInfo*>(ootype)->fieldoffsets[fid]);
}

void ICPPParseJSON::completeParseEntity(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    const EntityType* etype = dynamic_cast<const EntityType*>(itype);

    void* oomem = this->entitystack.back().first;
    const BSQType* ootype = this->entitystack.back().second;

    std::vector<StorageLocationPtr> cargs;
    std::transform(etype->consfields.cbegin(), etype->consfields.cend(), std::back_inserter(cargs), [&](const std::pair<std::string, std::string>& fnamefkey) {
        BSQFieldID fid = MarshalEnvironment::g_fieldToIdMap.at(fnamefkey.second);
        return SLPTR_INDEX_DATAPTR(oomem, dynamic_cast<const BSQEntityInfo*>(ootype)->fieldoffsets[fid]);
    });

    BSQBool* mask = this->entitymaskstack.back();

    if(etype->validatefunc.has_value())
    {
        auto chkid = MarshalEnvironment::g_invokeToIdMap.at(etype->validatefunc.value());
        auto chkcall = dynamic_cast<const BSQInvokeBodyDecl*>(BSQInvokeDecl::g_invokes[chkid]);

        std::string pfile = "[INPUT PARSE]";
        bool checkok = ctx.iinvoke(chkcall, cargs, mask);
        BSQ_LANGUAGE_ASSERT(checkok, (&pfile), -1, "Input Data Validation Failed");
    }

    uint64_t bytes = 0;
    if(ootype->tkind == BSQTypeLayoutKind::Struct)
    {
        bytes = ootype->allocinfo.inlinedatasize;
    }
    else
    {
        bytes = ootype->allocinfo.heapsize;
    }

    if(etype->consfunc.has_value())
    {
        auto consid = MarshalEnvironment::g_invokeToIdMap.at(etype->consfunc.value());
        auto conscall = dynamic_cast<const BSQInvokeBodyDecl*>(BSQInvokeDecl::g_invokes[consid]);

        std::string pfile = "[INPUT PARSE]";
        ctx.cinvoke(conscall, cargs, mask, value);
    }
    else
    {
        void* trgt = nullptr;
        uint64_t bytes = 0;
        if(ootype->tkind == BSQTypeLayoutKind::Struct)
        {
            trgt = (void*)value;
            bytes = ootype->allocinfo.inlinedatasize;
        }
        else
        {
            trgt = Allocator::GlobalAllocator.allocateDynamic(ootype);
            bytes = ootype->allocinfo.heapsize;

            SLPTR_STORE_CONTENTS_AS_GENERIC_HEAPOBJ(value, trgt);
        }

        GC_MEM_COPY(trgt, oomem, bytes);
    }

    xfree(mask);
    GCStack::popFrame(bytes);
    this->tuplestack.pop_back();
}

void ICPPParseJSON::setMaskFlag(const APIModule* apimodule, StorageLocationPtr flagloc, size_t i, bool flag, Evaluator& ctx)
{
    *(((BSQBool*)flagloc) + i) = (BSQBool)flag;
}

StorageLocationPtr ICPPParseJSON::parseUnionChoice(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t pick, const IType* picktype, Evaluator& ctx)
{
    auto bsqutypeid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    auto bsqutype = dynamic_cast<const BSQUnionType*>(BSQType::g_typetable[bsqutypeid]);
    if(bsqutype->tkind == BSQTypeLayoutKind::UnionRef)
    {
        return value;
    }
    else
    {
        auto ttypeid = MarshalEnvironment::g_typenameToIdMap.at(picktype->name);
        auto ttype = BSQType::g_typetable[ttypeid];

        SLPTR_STORE_UNION_INLINE_TYPE(ttype, value);
        return SLPTR_LOAD_UNION_INLINE_DATAPTR(value);
    }
}

std::optional<bool> ICPPParseJSON::extractBoolImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    return std::make_optional((bool)SLPTR_LOAD_CONTENTS_AS(BSQBool, value));
}

std::optional<uint64_t> ICPPParseJSON::extractNatImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    return std::make_optional((uint64_t)SLPTR_LOAD_CONTENTS_AS(BSQNat, value));
}

std::optional<int64_t> ICPPParseJSON::extractIntImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    return std::make_optional((int64_t)SLPTR_LOAD_CONTENTS_AS(BSQInt, value));
}

std::optional<std::string> ICPPParseJSON::extractBigNatImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto val = (uint64_t)SLPTR_LOAD_CONTENTS_AS(BSQBigNat, value);
    return std::make_optional(std::to_string(val));
}

std::optional<std::string> ICPPParseJSON::extractBigIntImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto val = (int64_t)SLPTR_LOAD_CONTENTS_AS(BSQBigInt, value);
    return std::make_optional(std::to_string(val));
}

std::optional<std::string> ICPPParseJSON::extractFloatImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    char cbuff[32];
    memset(cbuff, 0, sizeof(cbuff));
    snprintf(cbuff, sizeof(cbuff), "%.9g", SLPTR_LOAD_CONTENTS_AS(BSQFloat, value));

    return std::make_optional(std::string(cbuff));
}

std::optional<std::string> ICPPParseJSON::extractDecimalImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    char cbuff[32];
    memset(cbuff, 0, sizeof(cbuff));
    snprintf(cbuff, sizeof(cbuff), "%.9g", SLPTR_LOAD_CONTENTS_AS(BSQDecimal, value));

    return std::make_optional(std::string(cbuff));
}

std::optional<std::pair<std::string, uint64_t>> ICPPParseJSON::extractRationalImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto val = SLPTR_LOAD_CONTENTS_AS(BSQRational, value);
    return std::make_optional(std::make_pair(std::to_string(val.numerator), val.denominator));
}

std::optional<std::string> ICPPParseJSON::extractStringImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto val = SLPTR_LOAD_CONTENTS_AS(BSQString, value);

    std::string rstr;
    BSQStringForwardIterator iter(&val, 0);

    rstr.reserve(BSQStringImplType::utf8ByteCount(val));
    while(iter.valid())
    {
        rstr.push_back((char)iter.get_byte());
        iter.advance_byte();
    }

    return rstr;
}

std::optional<std::pair<std::vector<uint8_t>, std::pair<uint8_t, uint8_t>>> ICPPParseJSON::extractByteBufferImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    BSQByteBuffer* bb = (BSQByteBuffer*)SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(value);
    auto pprops = std::make_pair((uint8_t)bb->compression, (uint8_t)bb->format);

    std::vector<uint8_t> bytes;
    bytes.reserve(bb->bytecount);

    BSQByteBufferNode* bbn = bb->bytes;
    while(bbn != nullptr)
    {
        std::copy(bbn->bytes->bytes, bbn->bytes->bytes + bbn->bytecount, std::back_inserter(bytes));
        bbn = bbn->next;
    }

    return std::make_optional(std::make_pair(bytes, pprops));
}

std::optional<APIDateTime> ICPPParseJSON::extractDateTimeImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    BSQDateTime t = SLPTR_LOAD_CONTENTS_AS(BSQDateTime, value);

    APIDateTime dt;
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;
    dt.hour = t.hour;
    dt.min = t.min;
    dt.tzdata = t.tzdata;

    return std::make_optional(dt);
}

std::optional<APIUTCDateTime> ICPPParseJSON::extractUTCDateTimeImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    BSQUTCDateTime t = SLPTR_LOAD_CONTENTS_AS(BSQUTCDateTime, value);

    APIUTCDateTime dt;
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;
    dt.hour = t.hour;
    dt.min = t.min;

    return std::make_optional(dt);
}

std::optional<APICalendarDate> ICPPParseJSON::extractCalendarDateImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    BSQCalendarDate t = SLPTR_LOAD_CONTENTS_AS(BSQCalendarDate, value);

    APICalendarDate dt;
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;

    return std::make_optional(dt);
}

std::optional<APIRelativeTime> ICPPParseJSON::extractRelativeTimeImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    BSQRelativeTime t = SLPTR_LOAD_CONTENTS_AS(BSQRelativeTime, value);

    APIRelativeTime dt;
    dt.hour = t.hour;
    dt.min = t.min;

    return std::make_optional(dt);
}
    

std::optional<uint64_t> ICPPParseJSON::extractTickTimeImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    return std::make_optional((uint64_t)SLPTR_LOAD_CONTENTS_AS(BSQTickTime, value));
}

std::optional<uint64_t> ICPPParseJSON::extractLogicalTimeImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    return std::make_optional((uint64_t)SLPTR_LOAD_CONTENTS_AS(BSQLogicalTime, value));
}

std::optional<APIISOTimeStamp> ICPPParseJSON::extractISOTimeStampImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    BSQISOTimeStamp t = SLPTR_LOAD_CONTENTS_AS(BSQISOTimeStamp, value);

    APIISOTimeStamp dt;
    dt.year = t.year;
    dt.month = t.month;
    dt.day = t.day;
    dt.hour = t.hour;
    dt.min = t.min;
    dt.sec = t.seconds;
    dt.millis = t.millis;

    return std::make_optional(dt);
}

std::optional<std::vector<uint8_t>> ICPPParseJSON::extractUUID4Impl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto uuid = SLPTR_LOAD_CONTENTS_AS(BSQUUID, value);

    std::vector<uint8_t> vv;
    std::copy(uuid.bytes, uuid.bytes + 16, std::back_inserter(vv));

    return std::make_optional(vv);
}

std::optional<std::vector<uint8_t>> ICPPParseJSON::extractUUID7Impl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto uuid = SLPTR_LOAD_CONTENTS_AS(BSQUUID, value);

    std::vector<uint8_t> vv;
    std::copy(uuid.bytes, uuid.bytes + 16, std::back_inserter(vv));

    return std::make_optional(vv);
}

std::optional<std::vector<uint8_t>> ICPPParseJSON::extractSHAContentHashImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto hash = (BSQSHAContentHash*)SLPTR_LOAD_CONTENTS_AS_GENERIC_HEAPOBJ(value);

    std::vector<uint8_t> vv;
    std::copy(hash->bytes, hash->bytes + 64, std::back_inserter(vv));

    return std::make_optional(vv);
}

std::optional<std::pair<float, float>> ICPPParseJSON::extractLatLongCoordinateImpl(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto llcoord = SLPTR_LOAD_CONTENTS_AS(BSQLatLongCoordinate, value);

    return std::make_optional(std::make_pair(llcoord.latitude, llcoord.longitude));
}

StorageLocationPtr ICPPParseJSON::extractValueForTupleIndex(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t i, Evaluator& ctx)
{
    BSQTypeID tupid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* tuptype = BSQType::g_typetable[tupid];

    return tuptype->indexStorageLocationOffset(value, dynamic_cast<const BSQTupleInfo*>(tuptype)->idxoffsets[i]);
}

StorageLocationPtr ICPPParseJSON::extractValueForRecordProperty(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, std::string pname, Evaluator& ctx)
{
    BSQTypeID recid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* rectype = BSQType::g_typetable[recid];
    auto recinfo = dynamic_cast<const BSQRecordInfo*>(rectype);

    BSQRecordPropertyID pid = MarshalEnvironment::g_propertyToIdMap.at(itype->name);
    auto piter = std::find(recinfo->properties.cbegin(), recinfo->properties.cend(), pid);
    auto pidx = std::distance(recinfo->properties.cbegin(), piter);

    return rectype->indexStorageLocationOffset(value, recinfo->propertyoffsets[pidx]);
}

StorageLocationPtr ICPPParseJSON::extractValueForEntityField(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, std::pair<std::string, std::string> fnamefkey, Evaluator& ctx)
{
    BSQTypeID ooid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* ootype = BSQType::g_typetable[ooid];
    auto ooinfo = dynamic_cast<const BSQEntityInfo*>(ootype);

    BSQFieldID fid = MarshalEnvironment::g_fieldToIdMap.at(fnamefkey.second);
    auto fiter = std::find(ooinfo->fields.cbegin(), ooinfo->fields.cend(), fid);
    auto fidx = std::distance(ooinfo->fields.cbegin(), fiter);

    return ootype->indexStorageLocationOffset(value, ooinfo->fieldoffsets[fidx]);
}

void ICPPParseJSON::prepareExtractContainer(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    BSQTypeID containertypeid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    const BSQType* collectiontype = BSQType::g_typetable[containertypeid];

    this->parsecontainerstack.push_back({});

    if(itype->tag == TypeTag::ContainerTTag)
    {
        auto ctype = dynamic_cast<const ContainerTType*>(itype);
        if(ctype->category == ContainerCategory::List)
        {
            if(LIST_LOAD_DATA(value) != nullptr)
            {
                const BSQListType* listtype = dynamic_cast<const BSQListType*>(collectiontype);
                const BSQListTypeFlavor& lflavor = BSQListOps::g_flavormap.at(listtype->etype);

                BSQListOps::s_enumerate_for_extract(lflavor, LIST_LOAD_DATA(value), this->parsecontainerstack.back());
            }
        }
        else if(ctype->category == ContainerCategory::Stack)
        {
            BSQ_INTERNAL_ASSERT(false);
        }
        else if(ctype->category == ContainerCategory::Queue)
        {
            BSQ_INTERNAL_ASSERT(false);
        }
        else
        {
            BSQ_INTERNAL_ASSERT(false);
        }
    }
    else
    {
        if(MAP_LOAD_DATA(value) != nullptr)
        {
            const BSQMapType* maptype = dynamic_cast<const BSQMapType*>(this->containerstack.back().first);
            const BSQMapTypeFlavor& mflavor = BSQMapOps::g_flavormap.at(std::make_pair(maptype->ktype, maptype->vtype));

            BSQMapOps::s_enumerate_for_extract(mflavor, MAP_LOAD_DATA(value), this->parsecontainerstack.back());
        }
    }

    this->parsecontainerstackiter.push_back(this->parsecontainerstack.back().begin());
}

std::optional<size_t> ICPPParseJSON::extractLengthForContainer(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    if(itype->tag == TypeTag::ContainerTTag)
    {
        const ContainerTType* ctype = dynamic_cast<const ContainerTType*>(itype);

        if(ctype->category == ContainerCategory::List)
        {
            if(LIST_LOAD_DATA(value) == nullptr)
            {
                return std::make_optional(0);
            }
            else
            {
                auto ttype = LIST_LOAD_REPR_TYPE(value);
                return std::make_optional((size_t) ttype->getCount(LIST_LOAD_DATA(value)));
            }
        }
        else if(ctype->category == ContainerCategory::Stack)
        {
            BSQ_INTERNAL_ASSERT(false);
            return std::nullopt;
        }
        else if(ctype->category == ContainerCategory::Queue)
        {
            BSQ_INTERNAL_ASSERT(false);
            return std::nullopt;
        }
        else
        {
            BSQ_INTERNAL_ASSERT(false);
            return std::nullopt;
        }
    }
    else
    {
        if(MAP_LOAD_DATA(value) == nullptr)
        {
            return std::make_optional(0);
        }
        else
        {
            return std::make_optional((size_t) ((BSQMapTreeRepr*)(MAP_LOAD_DATA(value)))->tcount);
        }
    }
}

StorageLocationPtr ICPPParseJSON::extractValueForContainer_T(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t i, Evaluator& ctx)
{
    auto loc = *(this->parsecontainerstackiter.back());
    this->parsecontainerstackiter.back()++;

    return loc;
}

std::pair<StorageLocationPtr, StorageLocationPtr> ICPPParseJSON::extractValueForContainer_KV(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, size_t i, Evaluator& ctx)
{
    auto kloc = *(this->parsecontainerstackiter.back());
    this->parsecontainerstackiter.back()++;

    auto vloc = *(this->parsecontainerstackiter.back());
    this->parsecontainerstackiter.back()++;
    
    return std::make_pair(kloc, vloc);
}

void ICPPParseJSON::completeExtractContainer(const APIModule* apimodule, const IType* itype, Evaluator& ctx)
{
    this->parsecontainerstackiter.pop_back();
    this->parsecontainerstack.pop_back();
}

std::optional<size_t> ICPPParseJSON::extractUnionChoice(const APIModule* apimodule, const IType* itype, const std::vector<const IType*>& opttypes, StorageLocationPtr value, Evaluator& ctx)
{
    auto bsqutypeid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    auto bsqutype = dynamic_cast<const BSQUnionType*>(BSQType::g_typetable[bsqutypeid]);

    BSQTypeID uid;
    if(bsqutype->tkind == BSQTypeLayoutKind::UnionRef)
    {
        uid = SLPTR_LOAD_HEAP_TYPE(value)->tid;
    }
    else
    {
        uid = SLPTR_LOAD_UNION_INLINE_TYPE(value)->tid;
    }

    auto ppos = std::find_if(opttypes.cbegin(), opttypes.cend(), [uid](const IType* ttopt) {
        auto bsqoptid = MarshalEnvironment::g_typenameToIdMap.at(ttopt->name);
        return bsqoptid == uid;
    });

    auto tidx = std::distance(opttypes.cbegin(), ppos);
    return std::make_optional(tidx);
}

StorageLocationPtr ICPPParseJSON::extractUnionValue(const APIModule* apimodule, const IType* itype, StorageLocationPtr value, Evaluator& ctx)
{
    auto bsqutypeid = MarshalEnvironment::g_typenameToIdMap.at(itype->name);
    auto bsqutype = dynamic_cast<const BSQUnionType*>(BSQType::g_typetable[bsqutypeid]);

    if(bsqutype->tkind == BSQTypeLayoutKind::UnionRef)
    {
        return value;
    }
    else
    {
        return SLPTR_LOAD_UNION_INLINE_DATAPTR(value);
    }
}
