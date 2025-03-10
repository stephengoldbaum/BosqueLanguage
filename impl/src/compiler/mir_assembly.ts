//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

import { CodeFileInfo, SourceInfo } from "../ast/parser";
import { MIRBody, MIRResolvedTypeKey, MIRFieldKey, MIRInvokeKey, MIRVirtualMethodKey, MIRGlobalKey } from "./mir_ops";
import { BSQRegex } from "../ast/bsqregex";

import * as assert from "assert";

enum APIEmitTypeTag
{
    NoneTag = 0x0,
    NothingTag,
    BoolTag,
    NatTag,
    IntTag,
    BigNatTag,
    BigIntTag,
    RationalTag,
    FloatTag,
    DecimalTag,
    StringTag,
    StringOfTag,
    DataStringTag,
    ByteBufferTag,
    DataBufferTag,
    DateTimeTag,
    UTCDateTimeTag,
    CalendarDateTag,
    RelativeTimeTag,
    TickTimeTag,
    LogicalTimeTag,
    ISOTimeStampTag,
    UUID4Tag,
    UUID7Tag,
    SHAContentHashTag,
    LatLongCoordinateTag,
    ConstructableOfType,
    TupleTag,
    RecordTag,
    ContainerTTag,
    ContainerKVTag,
    EnumTag,
    EntityTag,
    UnionTag
};

enum ContainerCategory
{
    List = 0x0,
    Stack,
    Queue,
    Set
}

function jemitsinfo(sinfo: SourceInfo): object {
    return { line: sinfo.line, column: sinfo.column, pos: sinfo.pos, span: sinfo.span };
}

function jparsesinfo(jobj: any): SourceInfo {
    return new SourceInfo(jobj.line, jobj.column, jobj.pos, jobj.span);
}

class MIRFunctionParameter {
    readonly name: string;
    readonly type: MIRResolvedTypeKey;

    constructor(name: string, type: MIRResolvedTypeKey) {
        this.name = name;
        this.type = type;
    }

    jemit(): object {
        return { name: this.name, type: this.type };
    }

    static jparse(jobj: any): MIRFunctionParameter {
        return new MIRFunctionParameter(jobj.name, jobj.type);
    }
}

class MIRConstantDecl {
    readonly enclosingDecl: MIRResolvedTypeKey | undefined;

    readonly gkey: MIRGlobalKey;
    readonly shortname: string;

    readonly attributes: string[];

    readonly sourceLocation: SourceInfo;
    readonly srcFile: string;

    readonly declaredType: MIRResolvedTypeKey;
    readonly ivalue: MIRInvokeKey;

    constructor(enclosingDecl: MIRResolvedTypeKey | undefined, gkey: MIRGlobalKey, shortname: string, attributes: string[], sinfo: SourceInfo, srcFile: string, declaredType: MIRResolvedTypeKey, ivalue: MIRInvokeKey) {
        this.enclosingDecl = enclosingDecl;
        this.gkey = gkey;
        this.shortname = shortname;

        this.attributes = attributes;

        this.sourceLocation = sinfo;
        this.srcFile = srcFile;

        this.declaredType = declaredType;
        this.ivalue = ivalue;
    }

    jemit(): object {
        return { enclosingDecl: this.enclosingDecl, gkey: this.gkey, shortname: this.shortname, attributes: this.attributes, sinfo: jemitsinfo(this.sourceLocation), file: this.srcFile, declaredType: this.declaredType, ivalue: this.ivalue };
    }

    static jparse(jobj: any): MIRConstantDecl {
        return new MIRConstantDecl(jobj.enclosingDecl, jobj.gkey, jobj.shortname, jobj.attributes, jparsesinfo(jobj.sinfo), jobj.file, jobj.declaredType, jobj.ivalue);
    }
}

abstract class MIRInvokeDecl {
    readonly enclosingDecl: MIRResolvedTypeKey | undefined;
    readonly bodyID: string;
    readonly ikey: MIRInvokeKey;
    readonly shortname: string;

    readonly sinfoStart: SourceInfo;
    readonly sinfoEnd: SourceInfo;
    readonly srcFile: string;

    readonly attributes: string[];
    readonly recursive: boolean;

    readonly params: MIRFunctionParameter[];
    readonly resultType: MIRResolvedTypeKey;

    readonly preconditions: MIRInvokeKey[] | undefined;
    readonly postconditions: MIRInvokeKey[] | undefined;

    readonly isUserCode: boolean;

    constructor(enclosingDecl: MIRResolvedTypeKey | undefined, bodyID: string, ikey: MIRInvokeKey, shortname: string, attributes: string[], recursive: boolean, sinfoStart: SourceInfo, sinfoEnd: SourceInfo, srcFile: string, params: MIRFunctionParameter[], resultType: MIRResolvedTypeKey, preconds: MIRInvokeKey[] | undefined, postconds: MIRInvokeKey[] | undefined, isUserCode: boolean) {
        this.enclosingDecl = enclosingDecl;
        this.bodyID = bodyID;
        this.ikey = ikey;
        this.shortname = shortname;

        this.sinfoStart = sinfoStart;
        this.sinfoEnd = sinfoEnd;
        this.srcFile = srcFile;

        this.attributes = attributes;
        this.recursive = recursive;

        this.params = params;
        this.resultType = resultType;

        this.preconditions = preconds;
        this.postconditions = postconds;

        this.isUserCode = isUserCode;
    }

    abstract jemit(): object;

    static jparse(jobj: any): MIRInvokeDecl {
        if (jobj.body) {
            return new MIRInvokeBodyDecl(jobj.enclosingDecl, jobj.bodyID, jobj.ikey, jobj.shortname, jobj.attributes, jobj.recursive, jparsesinfo(jobj.sinfoStart), jparsesinfo(jobj.sinfoEnd), jobj.file, jobj.params.map((p: any) => MIRFunctionParameter.jparse(p)), jobj.masksize, jobj.resultType, jobj.preconditions || undefined, jobj.postconditions || undefined, jobj.isUserCode, MIRBody.jparse(jobj.body));
        }
        else {
            let binds = new Map<string, MIRResolvedTypeKey>();
            jobj.binds.forEach((bind: any) => binds.set(bind[0], bind[1]));

            let pcodes = new Map<string, MIRPCode>();
            jobj.pcodes.forEach((pc: any) => pcodes.set(pc[0], pc[1]));

            return new MIRInvokePrimitiveDecl(jobj.enclosingDecl, jobj.bodyID, jobj.ikey, jobj.shortname, jobj.attributes, jobj.recursive, jparsesinfo(jobj.sinfoStart), jparsesinfo(jobj.sinfoEnd), jobj.file, binds, jobj.params.map((p: any) => MIRFunctionParameter.jparse(p)), jobj.resultType, jobj.implkey, pcodes);
        }
    }
}

class MIRInvokeBodyDecl extends MIRInvokeDecl {
    readonly body: MIRBody;
    readonly masksize: number;

    constructor(enclosingDecl: MIRResolvedTypeKey | undefined, bodyID: string, ikey: MIRInvokeKey, shortname: string, attributes: string[], recursive: boolean, sinfoStart: SourceInfo, sinfoEnd: SourceInfo, srcFile: string, params: MIRFunctionParameter[], masksize: number, resultType: MIRResolvedTypeKey, preconds: MIRInvokeKey[] | undefined, postconds: MIRInvokeKey[] | undefined, isUserCode: boolean, body: MIRBody) {
        super(enclosingDecl, bodyID, ikey, shortname, attributes, recursive, sinfoStart, sinfoEnd, srcFile, params, resultType, preconds, postconds, isUserCode);

        this.body = body;
        this.masksize = masksize;
    }

    jemit(): object {
        return { enclosingDecl: this.enclosingDecl, bodyID: this.bodyID, ikey: this.ikey, shortname: this.shortname, sinfoStart: jemitsinfo(this.sinfoStart), sinfoEnd: jemitsinfo(this.sinfoEnd), file: this.srcFile, attributes: this.attributes, recursive: this.recursive, params: this.params.map((p) => p.jemit()), masksize: this.masksize, resultType: this.resultType, preconditions: this.preconditions, postconditions: this.postconditions, body: this.body.jemit(), isUserCode: this.isUserCode };
    }
}

type MIRPCode = {
    code: MIRInvokeKey,
    cargs: {cname: string, ctype: MIRResolvedTypeKey}[]
};

class MIRInvokePrimitiveDecl extends MIRInvokeDecl {
    readonly implkey: string;
    readonly binds: Map<string, MIRResolvedTypeKey>;
    readonly pcodes: Map<string, MIRPCode>;

    constructor(enclosingDecl: MIRResolvedTypeKey | undefined, bodyID: string, ikey: MIRInvokeKey, shortname: string, attributes: string[], recursive: boolean, sinfoStart: SourceInfo, sinfoEnd: SourceInfo, srcFile: string, binds: Map<string, MIRResolvedTypeKey>, params: MIRFunctionParameter[], resultType: MIRResolvedTypeKey, implkey: string, pcodes: Map<string, MIRPCode>) {
        super(enclosingDecl, bodyID, ikey, shortname, attributes, recursive, sinfoStart, sinfoEnd, srcFile, params, resultType, undefined, undefined, false);

        this.implkey = implkey;
        this.binds = binds;
        this.pcodes = pcodes;
    }

    jemit(): object {
        return {enclosingDecl: this.enclosingDecl, bodyID: this.bodyID, ikey: this.ikey, shortname: this.shortname, sinfoStart: jemitsinfo(this.sinfoStart), sinfoEnd: jemitsinfo(this.sinfoEnd), file: this.srcFile, attributes: this.attributes, recursive: this.recursive, params: this.params.map((p) => p.jemit()), resultType: this.resultType, implkey: this.implkey, binds: [...this.binds], pcodes: [... this.pcodes] };
    }
}

class MIRFieldDecl {
    readonly enclosingDecl: MIRResolvedTypeKey;
    readonly attributes: string[];

    readonly fkey: MIRFieldKey;
    readonly fname: string;

    readonly sourceLocation: SourceInfo;
    readonly srcFile: string;

    readonly isOptional: boolean;
    readonly declaredType: MIRResolvedTypeKey;

    constructor(enclosingDecl: MIRResolvedTypeKey, attributes: string[], srcInfo: SourceInfo, srcFile: string, fkey: MIRFieldKey, fname: string, isOptional: boolean, dtype: MIRResolvedTypeKey) {
        this.enclosingDecl = enclosingDecl;
        this.attributes = attributes;

        this.fkey = fkey;
        this.fname = fname;

        this.sourceLocation = srcInfo;
        this.srcFile = srcFile;

        this.isOptional = isOptional;
        this.declaredType = dtype;
    }

    jemit(): object {
        return { enclosingDecl: this.enclosingDecl, attributes: this.attributes, fkey: this.fkey, fname: this.fname, sinfo: jemitsinfo(this.sourceLocation), file: this.srcFile, isOptional: this.isOptional, declaredType: this.declaredType };
    }

    static jparse(jobj: any): MIRFieldDecl {
        return new MIRFieldDecl(jobj.enclosingDecl, jobj.attributes, jparsesinfo(jobj.sinfo), jobj.file, jobj.fkey, jobj.fname, jobj.isOptional, jobj.declaredType);
    }
}

abstract class MIROOTypeDecl {
    readonly tkey: MIRResolvedTypeKey;

    readonly sourceLocation: SourceInfo;
    readonly srcFile: string;

    readonly attributes: string[];

    readonly ns: string;
    readonly name: string;
    readonly terms: Map<string, MIRType>;
    readonly provides: MIRResolvedTypeKey[];

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[]) {
        this.tkey = tkey;

        this.sourceLocation = srcInfo;
        this.srcFile = srcFile;

        this.attributes = attributes;

        this.ns = ns;
        this.name = name;
        this.terms = terms;
        this.provides = provides;
    }

    abstract jemit(): object;

    jemitbase(): object {
        return { tkey: this.tkey, sinfo: jemitsinfo(this.sourceLocation), file: this.srcFile, attributes: this.attributes, ns: this.ns, name: this.name, terms: [...this.terms].map((t) => [t[0], t[1].jemit()]), provides: this.provides };
    }

    static jparsebase(jobj: any): [SourceInfo, string, MIRResolvedTypeKey, string[], string, string, Map<string, MIRType>, MIRResolvedTypeKey[]] {
        let terms = new Map<string, MIRType>();
            jobj.terms.forEach((t: any) => terms.set(t[0], MIRType.jparse(t[1])));

        return [jparsesinfo(jobj.sinfo), jobj.file, jobj.tkey, jobj.attributes, jobj.ns, jobj.name, terms, jobj.provides];
    }

    static jparse(jobj: any): MIROOTypeDecl {
        const tag = jobj.tag;
        switch (tag) {
            case "concept":
                return MIRConceptTypeDecl.jparse(jobj);
            case "std":
                return MIRObjectEntityTypeDecl.jparse(jobj);
            case "constructable":
                return MIRConstructableEntityTypeDecl.jparse(jobj);
            case "enum":
                return MIREnumEntityTypeDecl.jparse(jobj);
            case "primitive":
                return MIRPrimitiveInternalEntityTypeDecl.jparse(jobj);
            case "constructableinternal":
                return MIRConstructableInternalEntityTypeDecl.jparse(jobj);
            case "havocsequence":
                return MIRHavocEntityTypeDecl.jparse(jobj);
            case "list":
                return MIRPrimitiveListEntityTypeDecl.jparse(jobj);
            case "stack":
                return MIRPrimitiveStackEntityTypeDecl.jparse(jobj);
            case "queue":
                return MIRPrimitiveQueueEntityTypeDecl.jparse(jobj);
            case "set":
                return MIRPrimitiveSetEntityTypeDecl.jparse(jobj);
            default:
                assert(tag === "map");

                return MIRPrimitiveMapEntityTypeDecl.jparse(jobj);
        }
    }
}

class MIRConceptTypeDecl extends MIROOTypeDecl {
    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[]) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);
    }

    jemit(): object {
        return { tag: "concept", ...this.jemitbase() };
    }

    static jparse(jobj: any): MIRConceptTypeDecl {
        return new MIRConceptTypeDecl(...MIROOTypeDecl.jparsebase(jobj));
    }
}

abstract class MIREntityTypeDecl extends MIROOTypeDecl {
    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[]) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);
    }
}

class MIRObjectEntityTypeDecl extends MIREntityTypeDecl {
    readonly hasconsinvariants: boolean;
    readonly validatefunc: MIRInvokeKey | undefined;
    readonly consfunc: MIRInvokeKey;
    readonly consfuncfields: {cfkey: MIRFieldKey, isoptional: boolean}[];

    readonly fields: MIRFieldDecl[];
    readonly vcallMap: Map<MIRVirtualMethodKey, MIRInvokeKey> = new Map<string, MIRInvokeKey>();

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], hasconsinvariants: boolean, validatefunc: MIRInvokeKey | undefined, consfunc: MIRInvokeKey, consfuncfields: {cfkey: MIRFieldKey, isoptional: boolean}[], fields: MIRFieldDecl[]) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.hasconsinvariants = hasconsinvariants;
        this.validatefunc = validatefunc;
        this.consfunc = consfunc;

        this.consfuncfields = consfuncfields;
        this.fields = fields;
    }

    jemit(): object {
        return { tag: "std", ...this.jemitbase(), hasconsinvariants: this.hasconsinvariants, validatefunc: this.validatefunc, consfunc: this.consfunc, consfuncfields: this.consfuncfields, fields: this.fields.map((f) => f.jemit()), vcallMap: [...this.vcallMap] };
    }

    static jparse(jobj: any): MIRConceptTypeDecl {
        let entity = new MIRObjectEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj), jobj.hasconsinvariants, jobj.validatefunc || undefined, jobj.consfunc, jobj.consfuncfields, jobj.fields.map((jf: any) => MIRFieldDecl.jparse(jf)));
        
        jobj.vcallMap.forEach((vc: any) => entity.vcallMap.set(vc[0], vc[1]));
        return entity;
    }
} 

class MIRConstructableEntityTypeDecl extends MIREntityTypeDecl {
    readonly valuetype: MIRResolvedTypeKey;
    readonly validatefunc: MIRInvokeKey | undefined; 
    readonly usingcons: MIRInvokeKey | undefined;
    readonly basetype: MIRResolvedTypeKey;

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], fromtype: MIRResolvedTypeKey, validatefunc: MIRInvokeKey | undefined, usingcons: MIRInvokeKey | undefined, basetype: MIRResolvedTypeKey) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.valuetype = fromtype;
        this.validatefunc = validatefunc;
        this.usingcons = usingcons;
        this.basetype = basetype;
    }

    jemit(): object {
        return { tag: "constructable", ...this.jemitbase(), valuetype: this.valuetype, validatefunc: this.validatefunc, usingcons: this.usingcons, basetype: this.basetype };
    }

    static jparse(jobj: any): MIRConstructableEntityTypeDecl {
        return new MIRConstructableEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj), jobj.valuetype, jobj.validatefunc, jobj.usingcons, jobj.basetype);
    }
}

class MIREnumEntityTypeDecl extends MIREntityTypeDecl {
    readonly enums: string[];

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], enums: string[]) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.enums = enums;
    }

    jemit(): object {
        return { tag: "enum", ...this.jemitbase(), enums: this.enums };
    }

    static jparse(jobj: any): MIREnumEntityTypeDecl {
        return new MIREnumEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj), jobj.enums);
    }
}

abstract class MIRInternalEntityTypeDecl extends MIREntityTypeDecl {
    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[]) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);
    }
} 

class MIRPrimitiveInternalEntityTypeDecl extends MIRInternalEntityTypeDecl {
    //Should just be a special implemented value

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[]) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);
    }

    jemit(): object {
        return { tag: "primitive", ...this.jemitbase() };
    }

    static jparse(jobj: any): MIRPrimitiveInternalEntityTypeDecl {
        return new MIRPrimitiveInternalEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj));
    }
} 

class MIRStringOfInternalEntityTypeDecl extends MIRInternalEntityTypeDecl {
    readonly validatortype: MIRResolvedTypeKey;

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], validatortype: MIRResolvedTypeKey) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.validatortype = validatortype;
    }

    jemit(): object {
        return { tag: "stringofinternal", ...this.jemitbase(), validatortype: this.validatortype };
    }

    static jparse(jobj: any): MIRStringOfInternalEntityTypeDecl {
        return new MIRStringOfInternalEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj), jobj.validatortype);
    }
}

class MIRDataStringInternalEntityTypeDecl extends MIRInternalEntityTypeDecl {
    readonly fromtype: MIRResolvedTypeKey;
    readonly accepts: MIRInvokeKey;

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], fromtype: MIRResolvedTypeKey, accepts: MIRInvokeKey) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.fromtype = fromtype;
        this.accepts = accepts;
    }

    jemit(): object {
        return { tag: "datastringinternal", ...this.jemitbase(), fromtype: this.fromtype, optaccepts: this.accepts };
    }

    static jparse(jobj: any): MIRDataStringInternalEntityTypeDecl {
        return new MIRDataStringInternalEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj), jobj.fromtype, jobj.accepts);
    }
}

class MIRDataBufferInternalEntityTypeDecl extends MIRInternalEntityTypeDecl {
    readonly fromtype: MIRResolvedTypeKey;
    readonly accepts: MIRInvokeKey;

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], fromtype: MIRResolvedTypeKey, accepts: MIRInvokeKey) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.fromtype = fromtype;
        this.accepts = accepts;
    }

    jemit(): object {
        return { tag: "databufferinternal", ...this.jemitbase(), fromtype: this.fromtype, accepts: this.accepts };
    }

    static jparse(jobj: any): MIRDataBufferInternalEntityTypeDecl {
        return new MIRDataBufferInternalEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj), jobj.fromtype, jobj.accepts);
    }
}

class MIRConstructableInternalEntityTypeDecl extends MIRInternalEntityTypeDecl {
    readonly fromtype: MIRResolvedTypeKey;

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], fromtype: MIRResolvedTypeKey) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.fromtype = fromtype;
    }

    jemit(): object {
        return { tag: "constructableinternal", ...this.jemitbase(), fromtype: this.fromtype };
    }

    static jparse(jobj: any): MIRConstructableInternalEntityTypeDecl {
        return new MIRConstructableInternalEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj), jobj.fromtype);
    }
}

class MIRHavocEntityTypeDecl extends MIRInternalEntityTypeDecl {
    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[]) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);
    }

    jemit(): object {
        return { tag: "havocsequence", ...this.jemitbase() };
    }

    static jparse(jobj: any): MIRHavocEntityTypeDecl {
        return new MIRHavocEntityTypeDecl(...MIROOTypeDecl.jparsebase(jobj));
    }
}

abstract class MIRPrimitiveCollectionEntityTypeDecl extends MIRInternalEntityTypeDecl {
    readonly binds: Map<string, MIRType>;

    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], binds: Map<string, MIRType>) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides);

        this.binds = binds;
    }

    jemitcollection(): object {
        const fbinds = [...this.binds].sort((a, b) => a[0].localeCompare(b[0])).map((v) => [v[0], v[1].jemit()]);
        return { ...this.jemitbase(), binds: fbinds };
    }

    static jparsecollection(jobj: any): [SourceInfo, string, MIRResolvedTypeKey, string[], string, string, Map<string, MIRType>, MIRResolvedTypeKey[], Map<string, MIRType>]  {
        const bbinds = new Map<string, MIRType>();
        jobj.binds.foreach((v: [string, any]) => {
            bbinds.set(v[0], MIRType.jparse(v[1]));
        });

        return [...MIROOTypeDecl.jparsebase(jobj), bbinds];
    }
}

class MIRPrimitiveListEntityTypeDecl extends MIRPrimitiveCollectionEntityTypeDecl {
    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], binds: Map<string, MIRType>) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides, binds);
    }

    getTypeT(): MIRType {
        return this.binds.get("T") as MIRType;
    }

    jemit(): object {
        return { tag: "list", ...this.jemitcollection() };
    }

    static jparse(jobj: any): MIRPrimitiveListEntityTypeDecl {
        return new MIRPrimitiveListEntityTypeDecl(...MIRPrimitiveCollectionEntityTypeDecl.jparsecollection(jobj));
    }
}

class MIRPrimitiveStackEntityTypeDecl extends MIRPrimitiveCollectionEntityTypeDecl {
   constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], binds: Map<string, MIRType>) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides, binds);
    }

    getTypeT(): MIRType {
        return this.binds.get("T") as MIRType;
    }

    jemit(): object {
        return { tag: "stack", ...this.jemitcollection() };
    }

    static jparse(jobj: any): MIRPrimitiveStackEntityTypeDecl {
        return new MIRPrimitiveStackEntityTypeDecl(...MIRPrimitiveCollectionEntityTypeDecl.jparsecollection(jobj));
    }
}

class MIRPrimitiveQueueEntityTypeDecl extends MIRPrimitiveCollectionEntityTypeDecl {
   constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], binds: Map<string, MIRType>) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides, binds);
    }

    getTypeT(): MIRType {
        return this.binds.get("T") as MIRType;
    }

    jemit(): object {
        return { tag: "queue", ...this.jemitcollection() };
    }

    static jparse(jobj: any): MIRPrimitiveQueueEntityTypeDecl {
        return new MIRPrimitiveQueueEntityTypeDecl(...MIRPrimitiveCollectionEntityTypeDecl.jparsecollection(jobj));
    }
}

class MIRPrimitiveSetEntityTypeDecl extends MIRPrimitiveCollectionEntityTypeDecl {
    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], binds: Map<string, MIRType>) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides, binds);
    }

    getTypeT(): MIRType {
        return this.binds.get("T") as MIRType;
    }

    jemit(): object {
        return { tag: "set", ...this.jemitcollection() };
    }

    static jparse(jobj: any): MIRPrimitiveSetEntityTypeDecl {
        return new MIRPrimitiveSetEntityTypeDecl(...MIRPrimitiveCollectionEntityTypeDecl.jparsecollection(jobj));
    }
}

class MIRPrimitiveMapEntityTypeDecl extends MIRPrimitiveCollectionEntityTypeDecl {
    constructor(srcInfo: SourceInfo, srcFile: string, tkey: MIRResolvedTypeKey, attributes: string[], ns: string, name: string, terms: Map<string, MIRType>, provides: MIRResolvedTypeKey[], binds: Map<string, MIRType>) {
        super(srcInfo, srcFile, tkey, attributes, ns, name, terms, provides, binds);
    }

    getTypeK(): MIRType {
        return this.binds.get("K") as MIRType;
    }

    getTypeV(): MIRType {
        return this.binds.get("V") as MIRType;
    }

    jemit(): object {
        return { tag: "set", ...this.jemitcollection() };
    }

    static jparse(jobj: any): MIRPrimitiveMapEntityTypeDecl {
        return new MIRPrimitiveMapEntityTypeDecl(...MIRPrimitiveCollectionEntityTypeDecl.jparsecollection(jobj));
    }
}

abstract class MIRTypeOption {
    readonly typeID: MIRResolvedTypeKey;

    constructor(typeID: MIRResolvedTypeKey) {
        this.typeID = typeID;
    }

    abstract jemit(): object;

    static jparse(jobj: any): MIRTypeOption {
        switch (jobj.kind) {
            case "entity":
                return MIREntityType.jparse(jobj);
            case "concept":
                return MIRConceptType.jparse(jobj);
            case "tuple":
                return MIRTupleType.jparse(jobj);
            case "record":
                return MIRRecordType.jparse(jobj);
            default:
                assert(jobj.kind === "ephemeral"); 
                return MIREphemeralListType.jparse(jobj);
        }
    }
}

class MIREntityType extends MIRTypeOption {
    private constructor(typeID: MIRResolvedTypeKey) {
        super(typeID);
    }

    static create(typeID: MIRResolvedTypeKey): MIREntityType {
        return new MIREntityType(typeID);
    }

    jemit(): object {
        return {kind: "entity", typeID: this.typeID };
    }

    static jparse(jobj: any): MIREntityType {
        return MIREntityType.create(jobj.typeID);
    }
}

class MIRConceptType extends MIRTypeOption {
    readonly ckeys: MIRResolvedTypeKey[];

    private constructor(typeID: MIRResolvedTypeKey, ckeys: MIRResolvedTypeKey[]) {
        super(typeID);
        this.ckeys = ckeys;
    }

    static create(ckeys: MIRResolvedTypeKey[]): MIRConceptType {
        const skeys = ckeys.sort((a, b) => a[0].localeCompare(b[0]));
        return new MIRConceptType(skeys.join(" & "), skeys);
    }

    jemit(): object {
        return {kind: "concept", ckeys: this.ckeys };
    }

    static jparse(jobj: any): MIRConceptType {
        return MIRConceptType.create(jobj.ckeys);
    }
}

class MIRTupleType extends MIRTypeOption {
    readonly entries: MIRType[];

    private constructor(typeID: MIRResolvedTypeKey, entries: MIRType[]) {
        super(typeID);
        this.entries = entries;
    }

    static create(entries: MIRType[]): MIRTupleType {
        let cvalue = entries.map((entry) => entry.typeID).join(", ");
        return new MIRTupleType(`[${cvalue}]`, [...entries]);
    }

    jemit(): object {
        return { kind: "tuple", entries: this.entries };
    }

    static jparse(jobj: any): MIRTupleType {
        return MIRTupleType.create(jobj.entries);
    }
}

class MIRRecordType extends MIRTypeOption {
    readonly entries: {pname: string, ptype: MIRType}[];

    constructor(typeID: string, entries: {pname: string, ptype: MIRType}[]) {
        super(typeID);
        this.entries = entries;
    }

    static create(entries: {pname: string, ptype: MIRType}[]): MIRRecordType {
        let simplifiedEntries = [...entries].sort((a, b) => a.pname.localeCompare(b.pname));
        const name = simplifiedEntries.map((entry) => entry.pname + ": " + entry.ptype.typeID).join(", ");

        return new MIRRecordType("{" + name + "}", simplifiedEntries);
    }

    jemit(): object {
        return { kind: "record", entries: this.entries };
    }

    static jparse(jobj: any): MIRRecordType {
        return MIRRecordType.create(jobj.entries);
    }
}

class MIREphemeralListType extends MIRTypeOption {
    readonly entries: MIRType[];

    private constructor(typeID: string, entries: MIRType[]) {
        super(typeID);
        this.entries = entries;
    }

    static create(entries: MIRType[]): MIREphemeralListType {
        const name = entries.map((entry) => entry.typeID).join(", ");

        return new MIREphemeralListType("(|" + name + "|)", entries);
    }

    jemit(): object {
        return { kind: "ephemeral", entries: this.entries.map((e) => e.jemit()) };
    }

    static jparse(jobj: any): MIREphemeralListType {
        return MIREphemeralListType.create(jobj.entries.map((e: any) => MIRType.jparse(e)));
    }
}

class MIRType {
    readonly typeID: MIRResolvedTypeKey;
    readonly options: MIRTypeOption[];

    private constructor(typeID: MIRResolvedTypeKey, options: MIRTypeOption[]) {
        this.typeID = typeID;
        this.options = options;
    }

    static createSingle(option: MIRTypeOption): MIRType {
        return new MIRType(option.typeID, [option]);
    }

    static create(options: MIRTypeOption[]): MIRType {
        if (options.length === 1) {
            return MIRType.createSingle(options[0]);
        }
        else {
            const typeid = [...options].sort().map((tk) => tk.typeID).join(" | ");

            return new MIRType(typeid, options);
        }
    }

    jemit(): object {
        return { options: this.options.map((opt) => opt.jemit()) };
    }

    static jparse(jobj: any): MIRType {
        return MIRType.create(jobj.options.map((opt: any) => MIRTypeOption.jparse(opt)));
    }

    isTupleTargetType(): boolean {
        return this.options.every((opt) => opt instanceof MIRTupleType);
    }

    isUniqueTupleTargetType(): boolean {
        return this.isTupleTargetType() && this.options.length === 1;
    }

    getUniqueTupleTargetType(): MIRTupleType {
        return (this.options[0] as MIRTupleType);
    }

    isRecordTargetType(): boolean {
        return this.options.every((opt) => opt instanceof MIRRecordType);
    }

    isUniqueRecordTargetType(): boolean {
        return this.isRecordTargetType() && this.options.length === 1;
    }

    getUniqueRecordTargetType(): MIRRecordType {
        return (this.options[0] as MIRRecordType);
    }

    isUniqueCallTargetType(): boolean {
        if (this.options.length !== 1) {
            return false;
        }

        return this.options[0] instanceof MIREntityType;
    }

    getUniqueCallTargetType(): MIREntityType {
        return this.options[0] as MIREntityType;
    }
}

enum SymbolicActionMode {
    EvaluateSymbolic, //Inputs will be symbolically parsed and executed and set to extract result value -- single entrypoint assumed
    ErrTestSymbolic, //Inputs will symbolically generated and executed with failures reported (NO check on output) -- single entrypoint assumed
    ChkTestSymbolic //Inputs will be symbolically generated and executed with failures reported and check for "true" return value -- single entrypoint assumed
}

enum RuntimeActionMode {
    EvaluateConcrete, //Inputs will be parsed as concrete values and set to extract result value
    ErrTestConcrete, //Inputs will be parsed as concrete values and executed with failures reported (NO check on output)
    ChkTestConcrete, //Inputs will be parsed as concrete values and executed with failures reported and check for "true" return value
}

class PackageConfig {
    readonly macrodefs: string[]; 
    readonly src: CodeFileInfo[];

    constructor(macrodefs: string[], src: CodeFileInfo[]) {
        this.macrodefs = macrodefs;
        this.src = src;
    }

    jemit(): object {
        return { macrodefs: this.macrodefs, src: this.src };
    }

    static jparse(jobj: any): PackageConfig {
        return new PackageConfig(jobj.macrodefs, jobj.src);
    }
}

class MIRAssembly {
    readonly package: PackageConfig[];
    readonly srcFiles: { fpath: string, contents: string }[];
    readonly srcHash: string;

    readonly literalRegexs: BSQRegex[] = [];
    readonly validatorRegexs: Map<MIRResolvedTypeKey, BSQRegex> = new Map<MIRResolvedTypeKey, BSQRegex>();

    readonly constantDecls: Map<MIRGlobalKey, MIRConstantDecl> = new Map<MIRGlobalKey, MIRConstantDecl>();
    readonly fieldDecls: Map<MIRFieldKey, MIRFieldDecl> = new Map<MIRFieldKey, MIRFieldDecl>();

    readonly invokeDecls: Map<MIRInvokeKey, MIRInvokeBodyDecl> = new Map<MIRInvokeKey, MIRInvokeBodyDecl>();
    readonly primitiveInvokeDecls: Map<MIRInvokeKey, MIRInvokePrimitiveDecl> = new Map<MIRInvokeKey, MIRInvokePrimitiveDecl>();
    readonly virtualOperatorDecls: Map<MIRVirtualMethodKey, MIRInvokeKey[]> = new Map<MIRVirtualMethodKey, MIRInvokeKey[]>();

    readonly conceptDecls: Map<MIRResolvedTypeKey, MIRConceptTypeDecl> = new Map<MIRResolvedTypeKey, MIRConceptTypeDecl>();
    readonly entityDecls: Map<MIRResolvedTypeKey, MIREntityTypeDecl> = new Map<MIRResolvedTypeKey, MIREntityTypeDecl>();

    readonly tupleDecls: Map<MIRResolvedTypeKey, MIRTupleType> = new Map<MIRResolvedTypeKey, MIRTupleType>();
    readonly recordDecls: Map<MIRResolvedTypeKey, MIRRecordType> = new Map<MIRResolvedTypeKey, MIRRecordType>();

    readonly ephemeralListDecls: Map<MIRResolvedTypeKey, MIREphemeralListType> = new Map<MIRResolvedTypeKey, MIREphemeralListType>();

    readonly typeMap: Map<MIRResolvedTypeKey, MIRType> = new Map<MIRResolvedTypeKey, MIRType>();

    private m_subtypeRelationMemo: Map<string, Map<string, boolean>> = new Map<string, Map<string, boolean>>();
    private m_atomSubtypeRelationMemo: Map<string, Map<string, boolean>> = new Map<string, Map<string, boolean>>();

    entyremaps = {namespaceremap: new Map<string, string>(), entrytypedef: new Map<string, MIRType>()};

    private getConceptsProvidedByTuple(tt: MIRTupleType): MIRType {
        let tci: MIRResolvedTypeKey[] = ["Tuple"];
        if (tt.entries.every((ttype) => this.subtypeOf(ttype, this.typeMap.get("APIType") as MIRType))) {
            tci.push("APIType");
        }
        else {
            if (tt.entries.every((ttype) => this.subtypeOf(ttype, this.typeMap.get("TestableType") as MIRType))) {
                tci.push("TestableType");
            }
        } 

        return MIRType.createSingle(MIRConceptType.create(tci));
    }

    private getConceptsProvidedByRecord(rr: MIRRecordType): MIRType {
        let tci: MIRResolvedTypeKey[] = ["Record"];
        if (rr.entries.every((entry) => this.subtypeOf(entry.ptype, this.typeMap.get("APIType") as MIRType))) {
            tci.push("APIType");
        }
        else {
            if (rr.entries.every((entry) => this.subtypeOf(entry.ptype, this.typeMap.get("TestableType") as MIRType))) {
                tci.push("TestableType");
            }
        }

        return MIRType.createSingle(MIRConceptType.create(tci));
    }

    private atomSubtypeOf_EntityConcept(t1: MIREntityType, t2: MIRConceptType): boolean {
        const t2type = MIRType.createSingle(t2);

        if(t1.typeID === "Nothing" && this.subtypeOf(t2type, this.typeMap.get("IOption") as MIRType)) {
            return true;
        }
        else {
            const t1e = this.entityDecls.get(t1.typeID) as MIREntityTypeDecl;
            return t1e.provides.some((provide) => this.subtypeOf(this.typeMap.get(provide) as MIRType, t2type));
        }
    }

    private atomSubtypeOf_ConceptConcept(t1: MIRConceptType, t2: MIRConceptType): boolean {
        return t2.ckeys.every((c2t) => {
            return t1.ckeys.some((c1t) => {
                const c1 = this.conceptDecls.get(c1t) as MIRConceptTypeDecl;
                const c2 = this.conceptDecls.get(c2t) as MIRConceptTypeDecl;

                if (c1.ns === c2.ns && c1.name === c2.name) {
                    return true;
                }

                return c1.provides.some((provide) => this.subtypeOf(this.typeMap.get(provide) as MIRType, this.typeMap.get(c2t) as MIRType));
            });
        });
    }

    private atomSubtypeOf_TupleConcept(t1: MIRTupleType, t2: MIRConceptType): boolean {
        const t2type = this.typeMap.get(t2.typeID) as MIRType;
        const tcitype = this.getConceptsProvidedByTuple(t1);
        return this.subtypeOf(tcitype, t2type);
    }

    private atomSubtypeOf_RecordConcept(t1: MIRRecordType, t2: MIRConceptType): boolean {
        const t2type = this.typeMap.get(t2.typeID) as MIRType;
        const tcitype = this.getConceptsProvidedByRecord(t1);
        return this.subtypeOf(tcitype, t2type);
    }

    private atomSubtypeOf(t1: MIRTypeOption, t2: MIRTypeOption): boolean {
        let memores = this.m_atomSubtypeRelationMemo.get(t1.typeID);
        if (memores === undefined) {
            this.m_atomSubtypeRelationMemo.set(t1.typeID, new Map<string, boolean>());
            memores = this.m_atomSubtypeRelationMemo.get(t1.typeID) as Map<string, boolean>;
        }

        let memoval = memores.get(t2.typeID);
        if (memoval !== undefined) {
            return memoval;
        }

        let res = false;

        if (t1.typeID === t2.typeID) {
            res = true;
        }
        else if (t1 instanceof MIRConceptType && t2 instanceof MIRConceptType) {
            res = this.atomSubtypeOf_ConceptConcept(t1, t2);
        }
        else if (t2 instanceof MIRConceptType) {
            if (t1 instanceof MIREntityType) {
                res = this.atomSubtypeOf_EntityConcept(t1, t2);
            }
            else if (t1 instanceof MIRTupleType) {
                res = this.atomSubtypeOf_TupleConcept(t1, t2);
            }
            else if (t1 instanceof MIRRecordType) {
                res = this.atomSubtypeOf_RecordConcept(t1, t2);
            }
            else {
                //fall-through
            }
        }
        else {
            //fall-through
        }

        memores.set(t2.typeID, res);
        return res;
    }

    constructor(pckge: PackageConfig[], srcFiles: { fpath: string, contents: string }[], srcHash: string) {
        this.package = pckge;
        this.srcFiles = srcFiles;
        this.srcHash = srcHash;
    }

    subtypeOf(t1: MIRType, t2: MIRType): boolean {
        let memores = this.m_subtypeRelationMemo.get(t1.typeID);
        if (memores === undefined) {
            this.m_subtypeRelationMemo.set(t1.typeID, new Map<string, boolean>());
            memores = this.m_subtypeRelationMemo.get(t1.typeID) as Map<string, boolean>;
        }

        let memoval = memores.get(t2.typeID);
        if (memoval !== undefined) {
            return memoval;
        }

        const res = (t1.typeID === t2.typeID) || t1.options.every((t1opt) => t2.options.some((t2opt) => this.atomSubtypeOf(t1opt, t2opt)));

        memores.set(t2.typeID, res);
        return res;
    }

    jemit(): object {
        return {
            package: this.package.map((pc) => pc.jemit()),
            srcFiles: this.srcFiles,
            srcHash: this.srcHash,
            literalRegexs: [...this.literalRegexs].map((lre) => lre.jemit()),
            validatorRegexs: [...this.validatorRegexs].map((vre) => [vre[0], vre[1]]),
            constantDecls: [...this.constantDecls].map((cd) => [cd[0], cd[1].jemit()]),
            fieldDecls: [...this.fieldDecls].map((fd) => [fd[0], fd[1].jemit()]),
            invokeDecls: [...this.invokeDecls].map((id) => [id[0], id[1].jemit()]),
            primitiveInvokeDecls: [...this.primitiveInvokeDecls].map((id) => [id[0], id[1].jemit()]),
            virtualOperatorDecls: [...this.virtualOperatorDecls],
            conceptDecls: [...this.conceptDecls].map((cd) => [cd[0], cd[1].jemit()]),
            entityDecls: [...this.entityDecls].map((ed) => [ed[0], ed[1].jemit()]),
            typeMap: [...this.typeMap].map((t) => [t[0], t[1].jemit()]),

            entyremaps: {
                namespaceremap: [...this.entyremaps.namespaceremap].map((vv) => vv), 
                entrytypedef: [...this.entyremaps.entrytypedef].map((vv) => [vv[0], vv[1].jemit()])
            }
        };
    }

    static jparse(jobj: any): MIRAssembly {
        let masm = new MIRAssembly(jobj.package.map((pc: any) => PackageConfig.jparse(pc)), jobj.srcFiles, jobj.srcHash);

        jobj.literalRegexs.forEach((lre: any) => masm.literalRegexs.push(BSQRegex.jparse(lre)));
        jobj.validatorRegexs.forEach((vre: any) => masm.validatorRegexs.set(vre[0], vre[1]));
        jobj.constantDecls.forEach((cd: any) => masm.constantDecls.set(cd[0], MIRConstantDecl.jparse(cd[1])));
        jobj.fieldDecls.forEach((fd: any) => masm.fieldDecls.set(fd[0], MIRFieldDecl.jparse(fd[1])));
        jobj.invokeDecls.forEach((id: any) => masm.invokeDecls.set(id[0], MIRInvokeDecl.jparse(id[1]) as MIRInvokeBodyDecl));
        jobj.primitiveInvokeDecls.forEach((id: any) => masm.primitiveInvokeDecls.set(id[0], MIRInvokeDecl.jparse(id[1]) as MIRInvokePrimitiveDecl));
        jobj.virtualOperatorDecls.forEach((od: any) => masm.virtualOperatorDecls.set(od[0], od[1]));
        jobj.conceptDecls.forEach((cd: any) => masm.conceptDecls.set(cd[0], MIROOTypeDecl.jparse(cd[1]) as MIRConceptTypeDecl));
        jobj.entityDecls.forEach((id: any) => masm.entityDecls.set(id[0], MIROOTypeDecl.jparse(id[1]) as MIREntityTypeDecl));
        jobj.typeMap.forEach((t: any) => masm.typeMap.set(t[0], MIRType.jparse(t[1])));

        jobj.entyremaps.namespaceremap.forEach((vv: any) => masm.entyremaps.namespaceremap.set(vv[0], vv[1]));
        jobj.entyremaps.entrytypedef.forEach((vv: any) => masm.entyremaps.entrytypedef.set(vv[0], MIRType.jparse(vv[1])));

        return masm;
    }

    
    private getAPITypeForEntity(tt: MIRType, entity: MIREntityTypeDecl): object {
        if(entity instanceof MIRInternalEntityTypeDecl) {
            if(entity instanceof MIRPrimitiveInternalEntityTypeDecl) {
                if (tt.typeID === "None") {
                    return {tag: APIEmitTypeTag.NoneTag};
                }
                else if (tt.typeID === "Nothing") {
                    return {tag: APIEmitTypeTag.NothingTag};
                }
                else if (tt.typeID === "Bool") {
                    return {tag: APIEmitTypeTag.BoolTag};
                }
                else if (tt.typeID === "Int") {
                    return {tag: APIEmitTypeTag.IntTag};
                }
                else if (tt.typeID === "Nat") {
                    return {tag: APIEmitTypeTag.NatTag};
                }
                else if (tt.typeID ===  "BigInt") {
                    return {tag: APIEmitTypeTag.BigIntTag};
                }
                else if (tt.typeID ===  "BigNat") {
                    return {tag: APIEmitTypeTag.BigNatTag};
                }
                else if (tt.typeID ===  "Float") {
                    return {tag: APIEmitTypeTag.FloatTag};
                }
                else if (tt.typeID ===  "Decimal") {
                    return {tag: APIEmitTypeTag.DecimalTag};
                }
                else if (tt.typeID ===  "Rational") {
                    return {tag: APIEmitTypeTag.RationalTag};
                }
                else if (tt.typeID === "String") {
                    return {tag: APIEmitTypeTag.StringTag};
                }
                else if (tt.typeID ===  "ByteBuffer") {
                    return {tag: APIEmitTypeTag.ByteBufferTag};
                }
                else if(tt.typeID === "DateTime") {
                    return {tag: APIEmitTypeTag.DateTimeTag};
                }
                else if(tt.typeID === "UTCDateTime") {
                    return {tag: APIEmitTypeTag.UTCDateTimeTag};
                }
                else if(tt.typeID === "CalendarDate") {
                    return {tag: APIEmitTypeTag.CalendarDateTag};
                }
                else if(tt.typeID === "RelativeTime") {
                    return {tag: APIEmitTypeTag.RelativeTimeTag};
                }
                else if(tt.typeID === "TickTime") {
                    return {tag: APIEmitTypeTag.TickTimeTag};
                }
                else if(tt.typeID === "LogicalTime") {
                    return {tag: APIEmitTypeTag.LogicalTimeTag};
                }
                else if(tt.typeID === "ISOTimeStamp") {
                    return {tag: APIEmitTypeTag.ISOTimeStampTag};
                }
                else if(tt.typeID === "UUID4") {
                    return {tag: APIEmitTypeTag.UUID4Tag};
                }
                else if(tt.typeID === "UUID7") {
                    return {tag: APIEmitTypeTag.UUID7Tag};
                }
                else if(tt.typeID === "SHAContentHash") {
                    return {tag: APIEmitTypeTag.SHAContentHashTag};
                }
                else if(tt.typeID === "LatLongCoordinate") {
                    return {tag: APIEmitTypeTag.LatLongCoordinateTag};
                }
                else {
                    assert(false);
                    return {tag: APIEmitTypeTag.NoneTag, name: "[UNKNOWN API TYPE]"};
                }
            }
            else if (entity instanceof MIRConstructableInternalEntityTypeDecl) {
                if (tt.typeID.startsWith("Something")) {
                    return {tag: APIEmitTypeTag.ConstructableOfType, name: tt.typeID, oftype: (entity.fromtype as MIRResolvedTypeKey), validatefunc: null};
                }
                else if (tt.typeID.startsWith("Result::Ok")) {
                    return {tag: APIEmitTypeTag.ConstructableOfType, name: tt.typeID, oftype: (entity.fromtype as MIRResolvedTypeKey), validatefunc: null};
                }
                else {
                    assert(tt.typeID.startsWith("Result::Err"));
                    return {tag: APIEmitTypeTag.ConstructableOfType, name: tt.typeID, oftype: (entity.fromtype as MIRResolvedTypeKey), validatefunc: null};
                }
            }
            else if (entity instanceof MIRStringOfInternalEntityTypeDecl) {
                const validator = this.validatorRegexs.get(entity.validatortype) as BSQRegex;
                return {tag: APIEmitTypeTag.StringOfTag, name: tt.typeID, validator: validator.jemit()};
            }
            else if (entity instanceof MIRDataStringInternalEntityTypeDecl) {
                return {tag: APIEmitTypeTag.DataStringTag, name: tt.typeID, oftype: entity.fromtype, chkinv: entity.accepts}
            }
            else if (entity instanceof MIRDataBufferInternalEntityTypeDecl) {
                return {tag: APIEmitTypeTag.DataStringTag, name: tt.typeID, oftype: entity.fromtype, chkinv: entity.accepts}
            }
            else {
                assert(entity instanceof MIRPrimitiveCollectionEntityTypeDecl);

                if(entity instanceof MIRPrimitiveListEntityTypeDecl) {
                    return {tag: APIEmitTypeTag.ContainerTTag, name: tt.typeID, category: ContainerCategory.List, elemtype: entity.getTypeT().typeID};
                }
                else if(entity instanceof MIRPrimitiveStackEntityTypeDecl) {
                    return {tag: APIEmitTypeTag.ContainerTTag, name: tt.typeID, category: ContainerCategory.Stack, elemtype: entity.getTypeT().typeID};
                }
                else if(entity instanceof MIRPrimitiveQueueEntityTypeDecl) {
                    return {tag: APIEmitTypeTag.ContainerTTag, name: tt.typeID, category: ContainerCategory.Queue, elemtype: entity.getTypeT().typeID};
                }
                else if(entity instanceof MIRPrimitiveSetEntityTypeDecl) {
                    return {tag: APIEmitTypeTag.ContainerTTag, name: tt.typeID, category: ContainerCategory.Set, elemtype: entity.getTypeT().typeID};
                }
                else {
                    const mentity = entity as MIRPrimitiveMapEntityTypeDecl;
                    return {tag: APIEmitTypeTag.ContainerKVTag, name: tt.typeID, ktype: mentity.getTypeK(), vtype: mentity.getTypeV()};
                }
            }
        }
        else if(entity instanceof MIRConstructableEntityTypeDecl) {
            return {tag: APIEmitTypeTag.ConstructableOfType, name: tt.typeID, oftype: entity.valuetype, validatefunc: entity.validatefunc || null};
        }
        else if(entity instanceof MIREnumEntityTypeDecl) {
            return {tag: APIEmitTypeTag.EnumTag, name: tt.typeID, enums: entity.enums};
        }
        else {
            const oentity = entity as MIRObjectEntityTypeDecl;
            
            let consfields: {fkey: MIRFieldKey, fname: string}[] = [];
            let ttypes: {declaredType: MIRResolvedTypeKey, isOptional: boolean}[] = [];
            for(let i = 0; i < oentity.consfuncfields.length; ++i)
            {
                const ff = oentity.consfuncfields[i];
                const mirff = this.fieldDecls.get(ff.cfkey) as MIRFieldDecl;

                consfields.push({fkey: mirff.fkey, fname: mirff.fname});
                ttypes.push({declaredType: mirff.declaredType, isOptional: ff.isoptional});
            }

            return {tag: APIEmitTypeTag.EntityTag, name: tt.typeID, consfields: consfields, ttypes: ttypes, validatefunc: oentity.validatefunc || null, consfunc: oentity.consfunc || null};
        }
    }

    getAPITypeFor(tt: MIRType): object {
        if(tt.options.length === 1 && (tt.options[0] instanceof MIRTupleType)) {
            const tdecl = this.tupleDecls.get(tt.typeID) as MIRTupleType;

            let ttypes: string[] = [];
            for(let i = 0; i < tdecl.entries.length; ++i)
            {
                const mirtt = tdecl.entries[i];
                ttypes.push(mirtt.typeID);
            }

            return {tag: APIEmitTypeTag.TupleTag, name: tt.typeID, ttypes: ttypes};
        }
        else if(tt.options.length === 1 && (tt.options[0] instanceof MIRRecordType)) {
            const rdecl = this.recordDecls.get(tt.typeID) as MIRRecordType;

            let props: string[] = [];
            let ttypes: string[] = [];
            for(let i = 0; i < rdecl.entries.length; ++i)
            {
                const prop = rdecl.entries[i].pname;
                const mirtt = rdecl.entries[i].ptype;

                props.push(prop);
                ttypes.push(mirtt.typeID);
            }

            return {tag: APIEmitTypeTag.RecordTag, name: tt.typeID, props: props, ttypes: ttypes};
        }
        else if(tt.options.length === 1 && (tt.options[0] instanceof MIREntityType)) {
            return this.getAPITypeForEntity(tt, this.entityDecls.get(tt.typeID) as MIREntityTypeDecl);
        }
        else if(tt.options.length === 1 && (tt.options[0] instanceof MIRConceptType)) {
            const etypes = [...this.entityDecls].filter((edi) => this.subtypeOf(this.typeMap.get(edi[1].tkey) as MIRType, tt));
            const opts: string[] = etypes.map((opt) => opt[1].tkey).sort((a, b) => a.localeCompare(b));

            let ropts: string[] = [];
            for(let i = 0; i < opts.length; ++i) {
                const has = ropts.includes(opts[i]);
                if(!has) {
                    ropts.push(opts[i]);
                }
            }

            return {tag: APIEmitTypeTag.UnionTag, name: tt.typeID, opts: ropts};
        }
        else {
            const etypes = [...this.entityDecls].filter((edi) => this.subtypeOf(this.typeMap.get(edi[1].tkey) as MIRType, tt));
            const opts: string[] = etypes.map((opt) => opt[1].tkey).sort((a, b) => a.localeCompare(b));
            
            let ropts: string[] = [];
            for(let i = 0; i < opts.length; ++i) {
                const has = ropts.includes(opts[i]);
                if(!has) {
                    ropts.push(opts[i]);
                }
            }

            return {tag: APIEmitTypeTag.UnionTag, name: tt.typeID, opts: ropts};
        }
    }

    getAPIForInvoke(invk: MIRInvokeDecl): object {
        return { name: invk.ikey, argnames: invk.params.map((pp) => pp.name), argtypes: invk.params.map((pp) => pp.type), restype: invk.resultType };
    }

    emitAPIInfo(entrypoints: MIRInvokeKey[], istestbuild: boolean): any {
        const apitype = this.typeMap.get("APIType") as MIRType;
        const testtype = this.typeMap.get("TestableType") as MIRType;
        const apitypes = [...this.typeMap]
            .filter((tp) => this.subtypeOf(this.typeMap.get(tp[0]) as MIRType, apitype) || (istestbuild && this.subtypeOf(this.typeMap.get(tp[0]) as MIRType, testtype)))
            .sort((a, b) => a[0].localeCompare(b[0]))
            .map((tt) => this.getAPITypeFor(tt[1]));

        const typedecls = [...this.entyremaps.entrytypedef].sort((a, b) => a[0].localeCompare(b[0])).map((td) => {
            return {name: td[0], type: td[1].typeID};
        });

        const namespacemap = [...this.entyremaps.namespaceremap].sort((a, b) => a[0].localeCompare(b[0])).map((td) => {
            return {name: td[0], into: td[1]};
        });

        const sigs = entrypoints.sort().map((ep) => {
            const epdcl = this.invokeDecls.get(ep) as MIRInvokeBodyDecl;
            return this.getAPIForInvoke(epdcl);
        })

        return { apitypes: apitypes, typedecls: typedecls, namespacemap: namespacemap, apisig: sigs};
    }
}

export {
    MIRConstantDecl, MIRFunctionParameter, MIRPCode, MIRInvokeDecl, MIRInvokeBodyDecl, MIRInvokePrimitiveDecl, MIRFieldDecl,
    MIROOTypeDecl, MIRConceptTypeDecl, MIREntityTypeDecl,
    MIRType, MIRTypeOption, 
    MIREntityType, MIRObjectEntityTypeDecl, MIRConstructableEntityTypeDecl, MIREnumEntityTypeDecl, MIRInternalEntityTypeDecl, MIRPrimitiveInternalEntityTypeDecl, MIRStringOfInternalEntityTypeDecl, MIRDataStringInternalEntityTypeDecl, MIRDataBufferInternalEntityTypeDecl, MIRConstructableInternalEntityTypeDecl, 
    MIRHavocEntityTypeDecl,
    MIRPrimitiveCollectionEntityTypeDecl, MIRPrimitiveListEntityTypeDecl, MIRPrimitiveStackEntityTypeDecl, MIRPrimitiveQueueEntityTypeDecl, MIRPrimitiveSetEntityTypeDecl, MIRPrimitiveMapEntityTypeDecl,
    MIRConceptType, MIRTupleType, MIRRecordType, MIREphemeralListType,
    SymbolicActionMode, RuntimeActionMode, PackageConfig, MIRAssembly
};
