/*
** $Id: lundump.c,v 2.44 2015/11/02 16:09:30 roberto Exp $
** load precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#define lundump_cpp
#define LUA_CORE

#include <uvm/lprefix.h>


#include <string.h>

#include <uvm/lua.h>

#include <uvm/ldebug.h>
#include <uvm/ldo.h>
#include <uvm/lfunc.h>
#include <uvm/lmem.h>
#include <uvm/lobject.h>
#include <uvm/lstring.h>
#include <uvm/lundump.h>
#include <uvm/lzio.h>
#include <uvm/uvm_lib.h>

using uvm::lua::api::global_uvm_chain_api;


#if !defined(luai_verifycode)
#define luai_verifycode(L,b,f)  /* empty */
#endif


typedef struct {
    lua_State *L;
    ZIO *Z;
    const char *name;
} LoadState;


static l_noret error(LoadState *S, const char *why) {
    luaO_pushfstring(S->L, "%s: %s precompiled chunk", S->name, why);
	global_uvm_chain_api->throw_exception(S->L, UVM_API_SIMPLE_ERROR, "%s: %s precompiled chunk", S->name, why);
    luaD_throw(S->L, LUA_ERRSYNTAX);
}


/*
** All high-level loads go through LoadVector; you can change it to
** adapt to the endianness of the input
*/
#define LoadVector(S,b,n)	LoadBlock(S,b,(n)*sizeof((b)[0]))

static void LoadBlock(LoadState *S, void *b, size_t size) {
    if (luaZ_read(S->Z, b, size) != 0)
        error(S, "truncated");
}


#define LoadVar(S,x)		LoadVector(S,&x,1)


static lu_byte LoadByte(LoadState *S) {
    lu_byte x;
    LoadVar(S, x);
    return x;
}


static int LoadInt(LoadState *S) {
    int32_t x;
    LoadVar(S, x);
    return x;
}


static lua_Number LoadNumber(LoadState *S) {
    lua_Number x;
    LoadVar(S, x);
    return x;
}


static lua_Integer LoadInteger(LoadState *S) {
    lua_Integer x;
    LoadVar(S, x);
    return x;
}


static uvm_types::GcString *LoadString(LoadState *S) {
    uint64_t size = LoadByte(S);
    if (size == 0xFF)
        LoadVar(S, size);
    if (size == 0)
        return nullptr;
    else if (--size <= LUAI_MAXSHORTLEN) {  /* short string? */
        char buff[LUAI_MAXSHORTLEN];
        LoadVector(S, buff, size);
        return luaS_newlstr(S->L, buff, size);
    }
    else {  /* long string */
        auto ts = luaS_createlngstrobj(S->L, size);
        LoadVector(S, getstr(ts), size);  /* load directly in final place */
        return ts;
    }
}

const int g_sizelimit = 1024 * 1024 * 500;

static bool LoadCode(LoadState *S, uvm_types::GcProto *f) {
    int n = LoadInt(S);
	if (n > g_sizelimit)
	{
		return false;
	}
	f->codes.resize(n);
	for (auto i = 0; i < n; i++) {
		f->codes[i] = 0;
	}
    LoadVector(S, f->codes.data(), n);
	return true;
}


static void LoadFunction(LoadState *S, uvm_types::GcProto *f, uvm_types::GcString *psource);

// 500M
const int g_constantslimit = 1024 * 1024 * 500;

static bool LoadConstants(LoadState *S, uvm_types::GcProto *f) {
    int i;
    int n = LoadInt(S);
	if (n > g_constantslimit)
	{
		return false;
	}
	f->ks.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->ks[i], 0x0, sizeof(f->ks[i]));
	}
    for (i = 0; i < n; i++)
        setnilvalue(&f->ks[i]);
    for (i = 0; i < n; i++) {
        TValue *o = &f->ks[i];
        int t = LoadByte(S);
        switch (t) {
        case LUA_TNIL:
            setnilvalue(o);
            break;
        case LUA_TBOOLEAN:
            setbvalue(o, LoadByte(S));
            break;
        case LUA_TNUMFLT:
            setfltvalue(o, LoadNumber(S));
            break;
        case LUA_TNUMINT:
            setivalue(o, LoadInteger(S));
            break;
        case LUA_TSHRSTR:
        case LUA_TLNGSTR:
            setsvalue2n(S->L, o, LoadString(S));
            break;
        default:
            lua_assert(0);
        }
    }
	return true;
}


static bool LoadProtos(LoadState *S, uvm_types::GcProto *f) {
    int i;
    int n = LoadInt(S);
	if (n > g_sizelimit)
	{
		return false;
	}
	f->ps.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->ps[i], 0x0, sizeof(f->ps[i]));
	}
    for (i = 0; i < n; i++)
        f->ps[i] = nullptr;
    for (i = 0; i < n; i++) {
        f->ps[i] = luaF_newproto(S->L);
        LoadFunction(S, f->ps[i], f->source);
    }
	return true;
}


static bool LoadUpvalues(LoadState *S, uvm_types::GcProto *f) {
    int i, n;
    n = LoadInt(S);
	if (n > g_sizelimit)
	{
		return false;
	}
	f->upvalues.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->upvalues[i], 0x0, sizeof(f->upvalues[i]));
	}
    for (i = 0; i < n; i++)
        f->upvalues[i].name = nullptr;
    for (i = 0; i < n; i++) {
        f->upvalues[i].instack = LoadByte(S);
        f->upvalues[i].idx = LoadByte(S);
    }
	return true;
}


// 500M
const int g_lineslimit = 1024 * 1024 * 500;

static bool LoadDebug(LoadState *S, uvm_types::GcProto *f) {
    int i, n;
    n = LoadInt(S);
	if (n > g_lineslimit)
	{
		return false;
	}
	f->lineinfos.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->lineinfos[i], 0x0, sizeof(f->lineinfos[i]));
	}
    LoadVector(S, f->lineinfos.data(), n);
    n = LoadInt(S);
	f->locvars.resize(n);
	for (auto i = 0; i < n; i++) {
		memset(&f->locvars[i], 0x0, sizeof(f->locvars[i]));
	};
    for (i = 0; i < n; i++)
        f->locvars[i].varname = nullptr;
    for (i = 0; i < n; i++) {
        f->locvars[i].varname = LoadString(S);
        f->locvars[i].startpc = LoadInt(S);
        f->locvars[i].endpc = LoadInt(S);
    }
    n = LoadInt(S);
    for (i = 0; i < n; i++)
        f->upvalues[i].name = LoadString(S);
	return true;
}


static void LoadFunction(LoadState *S, uvm_types::GcProto *f, uvm_types::GcString *psource) {
    f->source = LoadString(S);
    if (f->source == nullptr)  /* no source in dump? */
        f->source = psource;  /* reuse parent's source */
    f->linedefined = LoadInt(S);
    f->lastlinedefined = LoadInt(S);
    f->numparams = LoadByte(S);
    f->is_vararg = LoadByte(S);
    f->maxstacksize = LoadByte(S);
	if (!LoadCode(S, f))
	{
		return;
	}
	if (!LoadConstants(S, f))
	{
		return;
	}
	if (!LoadUpvalues(S, f))
	{
		return;
	}
	if (!LoadProtos(S, f))
	{
		return;
	}
    LoadDebug(S, f);
}


static void checkliteral(LoadState *S, const char *s, const char *msg) {
    char buff[sizeof(LUA_SIGNATURE) + sizeof(LUAC_DATA)]; /* larger than both */
    size_t len = strlen(s);
    LoadVector(S, buff, len);
    if (memcmp(s, buff, len) != 0)
        error(S, msg);
}


static void fchecksize(LoadState *S, size_t size, const char *tname) {
    if (LoadByte(S) != size)
        error(S, luaO_pushfstring(S->L, "%s size mismatch in", tname));
}


#define checksize(S,t)	fchecksize(S,sizeof(t),#t)

static void checkHeader(LoadState *S) {
    checkliteral(S, LUA_SIGNATURE + 1, "not a");  /* 1st char already checked */
    if (LoadByte(S) != LUAC_VERSION)
        error(S, "version mismatch in");
    if (LoadByte(S) != LUAC_FORMAT)
        error(S, "format mismatch in");
    checkliteral(S, LUAC_DATA, "corrupted");
    checksize(S, int32_t);
    checksize(S, LUA_SIZE_T_TYPE);
    checksize(S, Instruction);
    checksize(S, lua_Integer);
    checksize(S, lua_Number);
    if (LoadInteger(S) != LUAC_INT)
        error(S, "endianness mismatch in");
    if (LoadNumber(S) != LUAC_NUM)
        error(S, "float format mismatch in");
}

//: vmgc::GcObject
struct InsSegment  {
	std::vector<int> fromSegIdxs;
	std::vector<int> toSegIdxs;
	std::vector<Instruction> instructions;
	OpCode endOp;
	bool isTest; //  opTest opTestSet opEqual opLessThan opLessOrEqual   (test类型指令后必跟随jmp, jmp指令不计入gas)
	bool isSubSegment;   //子分片
	int origStartCodeIdx;
	int	newStartCodeIdx;
	bool needModify;
	std::string	labelLocation;

	InsSegment() {
		isTest = false;
		isSubSegment = false;
		needModify = false;
		labelLocation = "";
		origStartCodeIdx = 0;
		newStartCodeIdx = 0;
	}


};
//UOP_TFORCALL后面总是紧随着UOP_TFORLOOP，2个只一次gas
std::vector<OpCode> branch_ops = { UOP_TEST,UOP_TESTSET,UOP_JMP,UOP_EQ,UOP_LT,UOP_LE,UOP_FORLOOP,UOP_FORPREP,UOP_TFORLOOP,UOP_LOADBOOL,UOP_RETURN,UOP_CALL,UOP_TAILCALL };

static int getSegmentIdx(std::vector<InsSegment*> segments , int codeIdx ) {
	for (int j = 0; j < segments.size(); j++ ){
		if (codeIdx == segments[j]->origStartCodeIdx) {
			return j;
		}
	}
	return -1;
}

static void setFromTo(std::vector<InsSegment*> segments, int segFromIdx , int segToIdx ){
	segments[segFromIdx]->toSegIdxs.push_back(segToIdx);
	segments[segToIdx]->fromSegIdxs.push_back(segFromIdx);
}
static std::string  preParseLabelLocations( uvm_types::GcProto * p, std::map<int, std::string>* plabelLocations)  {
	Instruction ins;
	int operand;
	int jmpDest;
	int codesize = p->codes.size();
	for (int i = 0; i < codesize; i++) {
		ins = p->codes[i];
		OpCode opcode = GET_OPCODE(ins);
		if (opcode == UOP_JMP || opcode == UOP_FORLOOP || opcode == UOP_FORPREP || opcode == UOP_TFORLOOP) {
			operand = GETARG_sBx(ins);
			jmpDest = operand + 1 + i;
			if ((jmpDest >= codesize) || (jmpDest < 0)) {
				return "bytecode jmp dest error";
			}
			lua_assert((jmpDest < codesize) && (jmpDest >= 0));
			std::string insLabel = "label_" + std::to_string(jmpDest);
			(*plabelLocations)[jmpDest] = insLabel;
		}				
	}
	return "";
}

static bool isBranchOp( Instruction ins) {
	auto op = GET_OPCODE(ins);	
	//UOP_LOADBOOL,/*	A B C	R(A) := (Bool)B; if (C) pc++			*/
	if ((op == UOP_LOADBOOL && GETARG_C(ins) != 0) ) {
		return true;
	}
	if (std::find(branch_ops.begin(), branch_ops.end(), op) != branch_ops.end()) {
		return true;
	}
	return false;
}

//计算gas 目前每条指令1gas
static int caculateGas(InsSegment* segment ) {
	auto insGasSum = (segment->instructions.size());
	if (GET_OPCODE(segment->instructions[insGasSum - 1]) == UOP_TFORLOOP) { //UOP_TFORCALL指令后面总是紧随着UOP_TFORLOOP，uvm运行时只计算一次指令gas
		insGasSum = insGasSum - 1;
	}
	else if (segment->isTest) { //test op随后必然跟随jmp指令,uvm运行时只计算一次指令gas
		insGasSum = insGasSum - 1;
	}
	
	return insGasSum;
}

static Instruction createAx( OpCode op, int a )
{ 
	return Instruction(op) << POS_OP | Instruction(a) << POS_Ax;
}

static void meterProto(lua_State* L, uvm_types::GcProto * p) {
	std::map<int,std::string> locationinfos;
	auto err = preParseLabelLocations(p,&locationinfos); //  code index => label
	if (err != "") {
		luaO_pushfstring(L, err.c_str());
		global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, err.c_str());
		luaD_throw(L, LUA_ERRSYNTAX);
		return;
	}

	if (p->codes.size() != p->lineinfos.size()) {
		luaO_pushfstring(L, "meter wrong, lineinfo size not match code size");
		global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, err.c_str());
		luaD_throw(L, LUA_ERRSYNTAX);
		return;
	}

	std::vector<InsSegment*> segments;
	auto segment = new InsSegment();
	int future_seg_idx = -1;

	for (int i = 0; i < p->codes.size(); i++) {
		Instruction ins = p->codes[i];
		OpCode ins_op = GET_OPCODE(ins);

		if (ins_op == UOP_METER) {

			for (int i = 0; i < (segments.size()); i++) {
				delete segments[i];
			}
			delete segment;

			luaO_pushfstring(L, "bytecode can't contain meter op");
			global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "bytecode can't contain meter op");
			luaD_throw(L, LUA_ERRSYNTAX);
			return;
		}
		if ((segment->isTest) && ins_op == UOP_JMP) { //is branch													//add to seg
			segment->instructions.push_back(ins);
			segment->endOp = GET_OPCODE(segment->instructions[segment->instructions.size() - 1]);
			segments.push_back(segment); 
			segment = new InsSegment();
			continue;
		}

		if (locationinfos.find(i) != locationinfos.end()) {
			//locations_idxs = append(locations_idxs,i)
			//add segments ， 再清空seg
			if ((segment->instructions.size()) > 0) {
				segment->endOp = GET_OPCODE(segment->instructions[(segment->instructions.size()) - 1]);
				segments.push_back(segment);
				segment = new InsSegment();
			}
			segment->labelLocation = locationinfos[i];
		}
		if (i == future_seg_idx) {
			if ((segment->instructions.size()) > 0) {
				segment->endOp = GET_OPCODE(segment->instructions[(segment->instructions.size()) - 1]);
				segments.push_back(segment);
				segment = new InsSegment();
			}
		}

		if ((segment->instructions.size()) == 0) {
			segment->origStartCodeIdx = i;
		}
		//add to seg
		segment->instructions.push_back(ins);

		//check branch
		//UOP_LOADBOOL,/*	A B C	R(A) := (Bool)B; if (C) pc++			*/
		if (isBranchOp(ins)) {
			if (ins_op == UOP_TEST || ins_op == UOP_TESTSET || ins_op == UOP_EQ || ins_op == UOP_LT || ins_op == UOP_LE) { //op PC++
				segment->isTest = true;
				future_seg_idx = i + 2;
			}
			else if (ins_op == UOP_LOADBOOL && GETARG_C(ins_op)!=0) {
				future_seg_idx = i + 2;
			}
			else if ((segment->instructions.size()) > 0) { //is branch , add segments ， 再清空seg
				segment->endOp = GET_OPCODE(segment->instructions[(segment->instructions.size()) - 1]);
				segments.push_back(segment);
				segment = new InsSegment();
			}
		}
	}
	if ((segment->instructions.size()) > 0) {
		segment->endOp = GET_OPCODE(segment->instructions[(segment->instructions.size()) - 1]);
		segments.push_back(segment);
	}
	else {
		delete segment;
	}

	for (int i = 0; i < (segments.size()); i++) {
		auto segment = segments[i];
		auto lastIns = segment->instructions[(segment->instructions.size()) - 1];
		auto ins_op = GET_OPCODE(lastIns);
		if (ins_op == UOP_JMP || ins_op == UOP_FORLOOP || ins_op == UOP_TFORLOOP || ins_op == UOP_FORPREP) {
			//test 类型op 其后都会跟随jmp
			//jmp to des

			auto operand = GETARG_sBx(lastIns);
			auto desInsIdx = segment->origStartCodeIdx + (segment->instructions.size()) + operand; //从下条指令起跳
			auto segToIdx = getSegmentIdx(segments, desInsIdx);
			lua_assert(segToIdx >= 0);

			segment->needModify = true;
			setFromTo(segments, i, segToIdx);

			//may go to next segment
			if (segment->isTest || ins_op == UOP_TFORLOOP || ins_op == UOP_FORLOOP) {
				desInsIdx = segment->origStartCodeIdx + (segment->instructions.size());
				auto segToIdx1 = getSegmentIdx(segments, desInsIdx);
				lua_assert(segToIdx1 >= 0);
				setFromTo(segments, i, segToIdx1);
			}
		}
		else if (ins_op == UOP_LOADBOOL && GETARG_C(lastIns) != 0) { 
			//next and next
			auto desInsIdx = segment->origStartCodeIdx + (segment->instructions.size()) + 1;			
			auto segToIdx2 = getSegmentIdx(segments, desInsIdx);
			lua_assert(segToIdx2 >= 0);
			setFromTo(segments, i, segToIdx2);		
		}
		else if (ins_op == UOP_RETURN) {
			segment->toSegIdxs.clear();
		}
		else { // no jump , just go next
			auto desInsIdx = segment->origStartCodeIdx + (segment->instructions.size());
			auto segToIdx1 = getSegmentIdx(segments, desInsIdx);
			lua_assert(segToIdx1 >= 0);
			setFromTo(segments, i, segToIdx1);
		}
	}

	//mark subsegment
	for (int i = 0; i < (segments.size()); i++) {
		auto segment = segments[i];
		if ((segment->fromSegIdxs.size()) == 1 && segments[segment->fromSegIdxs[0]]->endOp != UOP_CALL && segments[segment->fromSegIdxs[0]]->endOp != UOP_TAILCALL && (segments[segment->fromSegIdxs[0]]->toSegIdxs.size()) == 1) {
			segment->isSubSegment = true;
		}
	}

	//add meter ,
	for (int i = 0; i < (segments.size()); i++) {
		auto segment = segments[i];
		//modify start code idx
		if (i > 0) {
			segment->newStartCodeIdx = segments[i - 1]->newStartCodeIdx + (segments[i - 1]->instructions.size());
		}
		if (i != 0 && (segment->fromSegIdxs.size()) == 0) {
			continue;
		}
		else if (!segment->isSubSegment) {
			//add meter , meter op 没有gas， 生产环境不允许meter op
			auto insGasSum = caculateGas(segment);
			auto tempSegment = segment;
			for (; ;) {
				if ((tempSegment->toSegIdxs.size()) == 1 && segments[tempSegment->toSegIdxs[0]]->isSubSegment) {
					tempSegment = segments[tempSegment->toSegIdxs[0]];
					auto seg_gas = caculateGas(tempSegment);
					insGasSum = insGasSum + seg_gas;
				}
				else {
					break;
				}
			}
			auto meterIns = createAx(UOP_METER, insGasSum);
			segment->instructions.emplace(segment->instructions.begin(), meterIns);
			p->codes.insert(p->codes.begin() + segment->newStartCodeIdx, meterIns);
			p->lineinfos.insert(p->lineinfos.begin() + segment->newStartCodeIdx, p->lineinfos[segment->newStartCodeIdx]);// insert meter op line

		}
	}

	//modify jmp instruction and reset locationlabel ,  set new code instructions
	int insertMeterOpNum = 0;
	for (int i = 0; i < (segments.size()); i++) {
		auto segment = segments[i];
		if (segment->needModify) {
			auto lastIns = segment->instructions[(segment->instructions.size()) - 1];
			auto toSegIdx = segment->toSegIdxs[0];
			auto toInsIdx = segments[toSegIdx]->newStartCodeIdx;
			auto savedPc = segment->newStartCodeIdx + (segment->instructions.size()); // 从下条指令起跳
			auto operand = toInsIdx - savedPc; 
			SETARG_sBx(lastIns, operand);
			segment->instructions[(segment->instructions.size()) - 1] = lastIns;

			p->codes[segment->newStartCodeIdx + (segment->instructions.size()) - 1] = lastIns;

		}

	}
	for (int i = 0; i < (segments.size()); i++) {
		delete segments[i];
	}

	lua_assert(p->codes.size() == p->lineinfos.size());
	if (p->codes.size() != p->lineinfos.size()) {
		luaO_pushfstring(L, "meter wrong, lineinfo size not match code size");
		global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, err.c_str());
		luaD_throw(L, LUA_ERRSYNTAX);
		return;
	}
	
}


// 把prototype转成lua asm的字符串格式
bool luaU_addMeter(lua_State* L, uvm_types::GcProto * p, bool isRecurse )  {
	meterProto(L,p);
	if (strlen(L->runerror) > 0 || strlen(L->compile_error) > 0)
		return false;
	if (isRecurse) {
		//write subprotos   -----------------------------------------
		for(auto iter=p->ps.begin(); iter!=p->ps.end();iter++){
			if (!luaU_addMeter(L, *iter, true)) {
				return false;
			}
		}
	}
	return true;
}


/*
** load precompiled chunk
*/
uvm_types::GcLClosure *luaU_undump(lua_State *L, ZIO *Z, const char *name) {
	LoadState S;
	uvm_types::GcLClosure *cl;
	if (*name == '@' || *name == '=')
		S.name = name + 1;
	else if (*name == LUA_SIGNATURE[0])
		S.name = "binary string";
	else
		S.name = name;
	S.L = L;
	S.Z = Z;
	checkHeader(&S);
	if (strlen(L->runerror) > 0 || strlen(L->compile_error) > 0)
		return nullptr;
	cl = luaF_newLclosure(L, LoadByte(&S));
	setclLvalue(L, L->top, cl);
	luaD_inctop(L);
	cl->p = luaF_newproto(L);
	LoadFunction(&S, cl->p, nullptr);
	lua_assert(cl->nupvalues == cl->p->upvalues.size());
	luai_verifycode(L, buff, cl->p);
	if (L->is_metering) {
		auto r = luaU_addMeter(L, cl->p, true);
		if (!r)
			return nullptr;
	}	
	return cl;
}
