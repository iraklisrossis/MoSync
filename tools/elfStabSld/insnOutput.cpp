#include "elfStabSld.h"

struct CCore {
	unsigned printInstruction(unsigned ip);
	void checkFunctionPointer(unsigned ip);
	void streamReturnType1(ReturnType returnType);
	void streamReturnType2(ReturnType returnType);
	const char* REG(int r);

	SIData& data;
	ostream& os;
	const Function& f;
	const byte* textBytes;
	const byte* dataBytes;
	const char* out;
};

void streamFunctionInstructions(SIData& data, const Function& f) {
	//printf("printInstructions(0x%x => 0x%x)\n", f.start, f.end);
	DEBUG_ASSERT(f.end >= f.start);
	CCore core = {
		data, data.stream, f, data.textBytes, data.dataBytes,
		data.cs ? "out " : "",
	};
	unsigned ip = f.start;
	while(ip <= f.end) {
		ip = core.printInstruction(ip);
	}
	if(ip != f.end + 1) {
		printf("f.start %x, f.end %x, ip %x\n", f.start, f.end, ip);
	}
	DEBUG_ASSERT(ip == f.end + 1);
	// in case of noreturn
	if(data.cs && f.ci.returnType != eVoid)
		core.os << "\tthrow new System.Exception();\n";
}

#include "build/gen-regnames.h"

const char* getIntRegName(size_t r) {
	if(r < ARRAY_SIZE(mapip2_register_names)) {
		return mapip2_register_names[r];
	} else {
		DEBIG_PHAT_ERROR;
	}
};

const char* getFloatRegName(size_t r) {
	if(r < ARRAY_SIZE(mapip2_float_register_names)) {
		return mapip2_float_register_names[r];
	} else {
		printf("Bad float reg: %" PRIuPTR "\n", r);
		DEBIG_PHAT_ERROR;
	}
};

const size_t nIntRegs = ARRAY_SIZE(mapip2_register_names), nFloatRegs = ARRAY_SIZE(mapip2_float_register_names);

#include "../../runtimes/cpp/core/core_helpers.h"
#include "../../runtimes/cpp/core/gen-opcodes.h"

#define IB ((int)(textBytes[ip++]))

#define USE_I(r) data.regUsage.i |= (1 << (r))
#define USE_F(r) data.regUsage.f |= (1 << (r))

#define FETCH_RD rd = IB; USE_I(rd);
#define FETCH_RS rs = IB; USE_I(rs);
#define FETCH_FRD rd = IB; USE_F(rd);
#define FETCH_FRS rs = IB; USE_F(rs);

#define FETCH_RD_RS_UNUSED rd = IB; rs = IB;

#define FETCH_FRD_RS FETCH_FRD FETCH_RS
#define FETCH_FRD_CONST FETCH_FRD FETCH_CONST
#define FETCH_FRD_RS_CONST FETCH_FRD FETCH_RS FETCH_CONST

#define FETCH_CONST if(op != OP_CALLI) checkFunctionPointer(ip); FETCH_INT

#define IMM imm32
#define IMMU "(uint)" << imm32

#define RS REG(rs)
#define RD REG(rd)
#define RSU "(uint)" << RS
#define RDU "(uint)" << RD

#define ARITH(rdst, a, oper, b) os << REG(rdst) << " = (int)(" << a << " " #oper " " << b << ");"

#define DIVIDE(rdst, a, b) ARITH(rdst, a, /, b)

#define OPC(a) case OP_##a: os << "\t";
#define EOP os << "\n"; break

#define WRITE_REG(rd, src) USE_I(rd); os << REG(rd) << " = " << src << ";"

#define FREG(r) getFloatRegName(r)
#define FRD FREG(rd)
#define FRS FREG(rs)

class label {
public:
	unsigned mIp;
	label(unsigned ip) : mIp(ip) {}
	template<class charT, class Traits>
  friend std::basic_ostream<charT, Traits>&
		operator<<(std::basic_ostream<charT, Traits >& os, const label& l);
};

template<class charT, class Traits>
std::basic_ostream<charT, Traits>&
	operator<<(std::basic_ostream<charT, Traits >& os, const label& l)
{
	os << "L" << l.mIp;
	return os;
}

void CCore::streamReturnType1(ReturnType returnType) {
	switch(returnType) {
	case eVoid: break;
	case eInt: os << "r0 = "; USE_I(REG_r0); break;
	case eFloat: os << "f8 = "; USE_F(8); break;
	case eLong: os << "MOV_DI("<<out<<"r0, "<<out<<"r1, "; USE_I(REG_r0); USE_I(REG_r1); break;
	case eComplexFloat: os << "MOV_CF("<<out<<"f8, "<<out<<"f9, "; USE_F(8); USE_F(9); break;
	case eComplexInt: os << "MOV_CI("<<out<<"r0, "<<out<<"r1, "; USE_I(REG_r0); USE_I(REG_r1); break;
	}
}
void CCore::streamReturnType2(ReturnType returnType) {
	switch(returnType) {
	case eLong:
	case eComplexFloat:
	case eComplexInt:
		os << ")";
	default:
		break;
	}
}

const char* CCore::REG(int r) {
	USE_I(r);
	return getIntRegName(r);
}


void streamCallRegName(ostream& os, const CallInfo& ci) {
	os << "callReg";
	switch(ci.returnType) {
	case eVoid: os << "V"; break;
	case eInt: os << "I"; break;
	case eFloat: os << "F"; break;
	case eLong: os << "L"; break;
	case eComplexFloat: os << "CF"; break;
	case eComplexInt: os << "CI"; break;
	}
	os << noshowbase;
	os << ci.intParams << ci.floatParams;
	os << showbase;
}

static void streamParameters(ostream& os, const CallInfo& ci, bool first = true) {
	for(unsigned i=0; i<ci.intParams; i++) {
		if(first)
			first = false;
		else
			os << ", ";
		os << getIntRegName(REG_p0 + i);
	}
	for(unsigned i=0; i<ci.floatParams; i++) {
		if(first)
			first = false;
		else
			os << ", ";
		os << FREG(8 + i);
	}
	os << ")";
}

void streamFunctionCall(ostream& os, bool cs, const Function& cf, bool syscall) {
	streamFunctionName(os, cs, cf, syscall);
	os << "(";
	streamParameters(os, cf.ci);
}

unsigned CCore::printInstruction(unsigned ip) {
	if(!data.cs)
	os << hex << showbase;
	os << "L" << ip << ":\t";

	byte op = IB;
	int imm32;
	byte rs, rd;

	switch (op)
	{
		OPC(ADD)	FETCH_RD_RS	ARITH(rd, RD, +, RS);	EOP;
		OPC(ADDI)	FETCH_RD_CONST	ARITH(rd, RD, +, IMM);	EOP;
		OPC(SUB)	FETCH_RD_RS	ARITH(rd, RD, -, RS);	EOP;
		OPC(SUBI)	FETCH_RD_CONST	ARITH(rd, RD, -, IMM);	EOP;
		OPC(MUL)	FETCH_RD_RS	ARITH(rd, RD, *, RS);	EOP;
		OPC(MULI)	FETCH_RD_CONST	ARITH(rd, RD, *, IMM);	EOP;
		OPC(AND)	FETCH_RD_RS	ARITH(rd, RD, &, RS);	EOP;
		OPC(ANDI)	FETCH_RD_CONST	ARITH(rd, RD, &, IMM);	EOP;
		OPC(OR)	FETCH_RD_RS	ARITH(rd, RD, |, RS);	EOP;
		OPC(ORI)	FETCH_RD_CONST	ARITH(rd, RD, |, IMM);	EOP;
		OPC(XOR)	FETCH_RD_RS	ARITH(rd, RD, ^, RS);	EOP;
		OPC(XORI)	FETCH_RD_CONST	ARITH(rd, RD, ^, IMM);	EOP;
		OPC(DIVU)	FETCH_RD_RS	DIVIDE(rd, RDU, RSU);	EOP;
		OPC(DIVUI)	FETCH_RD_CONST	DIVIDE(rd, RDU, IMMU);	EOP;
		OPC(DIV)	FETCH_RD_RS	DIVIDE(rd, RD, RS);		EOP;
		OPC(DIVI)	FETCH_RD_CONST	DIVIDE(rd, RD, "(int)" << IMM);	EOP;
		OPC(SLL)	FETCH_RD_RS	ARITH(rd, RDU, <<, "(byte)" << RS);	EOP;
		OPC(SLLI)	FETCH_RD_IMM8	ARITH(rd, RDU, <<, (uint)(byte)IMM);	EOP;
		OPC(SRA)	FETCH_RD_RS	ARITH(rd, RD, >>, "(byte)" << RS);	EOP;
		OPC(SRAI)	FETCH_RD_IMM8	ARITH(rd, RD, >>, (uint)(byte)IMM);	EOP;
		OPC(SRL)	FETCH_RD_RS	ARITH(rd, RDU, >>, "(byte)" << RS);	EOP;
		OPC(SRLI)	FETCH_RD_IMM8	ARITH(rd, RDU, >>, (uint)(byte)IMM);	EOP;

		OPC(NOT)	FETCH_RD_RS	os << RD << " = ~" << RS << ";";	EOP;
		OPC(NEG)	FETCH_RD_RS	os << RD << " = -" << RS << ";";	EOP;

		OPC(PUSH)	FETCH_RD_RS_UNUSED
		{
			byte n = (rs - rd) + 1;
			if(rd < 2 || int(rd) + n > 32 || n == 0) {
				DUMPINT(rd);
				DUMPINT(n);
				DEBIG_PHAT_ERROR; //raise hell
			}

			os << "sp -= " << (4 * n) << ";\t";
			os << "// push " << RD << ", " << RS << ";";
		}
		EOP;

		OPC(POP) FETCH_RD_RS_UNUSED
		{
			byte n = (rs - rd) + 1;
			if(rd > 31 || int(rs) - n < 1 || n == 0) {
				DUMPINT(rd);
				DUMPINT(rs);
				DUMPINT(n);
				DEBIG_PHAT_ERROR; //raise hell
			}

			os << "sp += " << (4 * n) << ";\t";
			os << "// pop " << RD << ", " << RS << ";";
		}
		EOP;

		OPC(FPUSH) FETCH_FRD_FRS
		{
			byte n = (rs - rd) + 1;
			if(int(rd) + n > 15 || n == 0) {
				DUMPINT(rd);
				DUMPINT(n);
				DEBIG_PHAT_ERROR; //raise hell
			}

			os << "sp -= " << (8 * n) << ";\t";
			os << "// fpush " << FRD << ", " << FRS << ";";
		}
		EOP;

		OPC(FPOP) FETCH_FRD_FRS
		{
			byte n = (rs - rd) + 1;
			if(rd > 15 || n <= 0) {
				DUMPINT(rd);
				DUMPINT(rs);
				DUMPINT(n);
				DEBIG_PHAT_ERROR; //raise hell
			}

			os << "sp += " << (8 * n) << ";\t";
			os << "// fpop " << FRD << ", " << FRS << ";";
		}
		EOP;

		OPC(LDB)
		{
			FETCH_RD_RS_CONST
			os << RD << " = RBYTE(" << RS << " + " << IMM << ");";
		}
		EOP;

		OPC(LDH)
		{
			FETCH_RD_RS_CONST
			os << RD << " = RSHORT(" << RS << " + " << IMM << ");";
		}
		EOP;

		OPC(LDW)
		{
			FETCH_RD_RS_CONST
			os << RD << " = RINT(" << RS << " + " << IMM << ");";
		}
		EOP;

		OPC(STB)
		{
			FETCH_RD_RS_CONST
			os << "WBYTE(" << RD << " + " << IMM << ", " << RS << ");";
		}
		EOP;

		OPC(STH)
		{
			FETCH_RD_RS_CONST
			os << "WSHORT(" << RD << " + " << IMM << ", " << RS << ");";
		}
		EOP;

		OPC(STW)
		{
			FETCH_RD_RS_CONST
			os << "WINT(" << RD << " + " << IMM << ", " << RS << ");";
		}
		EOP;

		OPC(LDI)	FETCH_RD_CONST	WRITE_REG(rd, IMM);	EOP;
		OPC(LDR)	FETCH_RD_RS	WRITE_REG(rd, RS);	EOP;

		OPC(LDDR) FETCH_RD_RS WRITE_REG(rd, RS); os << " "; WRITE_REG(rd+1, REG(rs+1)); EOP;
		OPC(LDDI) FETCH_RD_CONST WRITE_REG(rd, IMM); os << " "; FETCH_CONST; WRITE_REG(rd+1, IMM); EOP;

		OPC(FLOATS) FETCH_FRD_RS os << FRD << " = (double)(int)" << RS << ";"; EOP;
		OPC(FLOATUNS) FETCH_FRD_RS os << FRD << " = (double)(uint)" << RS << ";"; EOP;

		OPC(FLOATD)
			FETCH_FRD_RS;
			os << "FLOATS_DIDF(" << RS << ", "<< REG(rs+1) << ", "<<out<< FRD << ");";
		EOP;

		OPC(FLOATUND)
			FETCH_FRD_RS;
			os << "FLOATU_DIDF(" << RS << ", "<< REG(rs+1) << ", "<<out<< FRD << ");";
		EOP;

		OPC(FSTRS)
			FETCH_RD_FRS;
			os << "MOV_SFSI(" <<out<< RD << ", " << FRS << ");";
		EOP;
		OPC(FSTRD)
			FETCH_RD_FRS
			os << "MOV_DFDI(" <<out<< RD << ", " <<out<< REG(rd+1) << ", " << FRS << ");";
		EOP;

		OPC(FLDRS) FETCH_FRD_RS os << "MOV_SISF(" << RS << ", "<<out<< FRD << ");"; EOP;
		OPC(FLDRD)
			FETCH_FRD_RS
			os << "MOV_DIDF(" << RS << ", " << REG(rs+1) << ", "<<out<< FRD << ");";
		EOP;

		OPC(FLDR) FETCH_FRD_FRS os << FRD << " = " << FRS << ";"; EOP;

		OPC(FLDIS) FETCH_FRD_CONST os << "MOV_SISF(" << IMM << ", "<<out<< FRD << ");"; EOP;
		OPC(FLDID)
		{
			FETCH_FRD_CONST;
			int i0 = IMM;
			FETCH_CONST;
			int i1 = IMM;
			os << "MOV_DIDF(" << i0 << ", " << i1 << ", "<<out<< FRD << ");";
		}
		EOP;

		OPC(FIX_TRUNCS) FETCH_RD_FRS WRITE_REG(rd, "(int)" << FRS); EOP;
		OPC(FIX_TRUNCD)
			FETCH_RD_FRS;
			os << "FIXS_DFDI("<<out<< RD << ", "<<out<< REG(rd+1) << ", " << FRS << ");";
		EOP;

		OPC(FIXUN_TRUNCS) FETCH_RD_FRS WRITE_REG(rd, "(int)(uint)" << FRS); EOP;
		OPC(FIXUN_TRUNCD)
			FETCH_RD_FRS;
			os << "FIXU_DFDI("<<out<< RD << ", "<<out<< REG(rd+1) << ", " << FRS << ");";
		EOP;

		OPC(FSTS)
			FETCH_RD_FRS_CONST
			os << "WFLOAT(" << RD << " + " << IMM << ", " << FRS << ");";
		EOP;

		OPC(FSTD)
			FETCH_RD_FRS_CONST
			os << "WDOUBLE(" << RD << " + " << IMM << ", " << FRS << ");";
		EOP;

		OPC(FLDS)
			FETCH_FRD_RS_CONST
			os << "MOV_SISF(RINT(" << RS << " + " << IMM << "), "<<out<< FRD << ");";
		EOP;

		OPC(FLDD)
			FETCH_FRD_RS_CONST
			os << "MOV_DIDF(RINT(" << RS << " + " << IMM << "), RINT(" << RS << " + " << IMM + 4 << "), "<<out<< FRD << ");";
		EOP;

		OPC(FADD) FETCH_FRD_FRS os << FRD << " += " << FRS << ";"; EOP;
		OPC(FSUB) FETCH_FRD_FRS os << FRD << " -= " << FRS << ";"; EOP;
		OPC(FMUL) FETCH_FRD_FRS os << FRD << " *= " << FRS << ";"; EOP;
		OPC(FDIV) FETCH_FRD_FRS os << FRD << " /= " << FRS << ";"; EOP;

		OPC(FSQRT) FETCH_FRD_FRS os << FRD << " = sqrt(" << FRS << ");"; EOP;
		OPC(FSIN) FETCH_FRD_FRS os << FRD << " = sin(" << FRS << ");"; EOP;
		OPC(FCOS) FETCH_FRD_FRS os << FRD << " = cos(" << FRS << ");"; EOP;
		OPC(FEXP) FETCH_FRD_FRS os << FRD << " = exp(" << FRS << ");"; EOP;
		OPC(FLOG) FETCH_FRD_FRS os << FRD << " = log(" << FRS << ");"; EOP;
		OPC(FPOW) FETCH_FRD_FRS os << FRD << " = pow(" << FRD << ", " << FRS << ");"; EOP;
		OPC(FATAN2) FETCH_FRD_FRS os << FRD << " = atan2(" << FRD << ", " << FRS << ");"; EOP;

		OPC(LDD)
			FETCH_RD_RS_CONST
			os << "{ int addr = "<<RS<<" + "<<IMM<<"; ";
			WRITE_REG(rd, "RINT(addr)"); os << " ";
			WRITE_REG(rd+1, "RINT(addr + 4)");
			os << " }";
		EOP;

		OPC(STD)
			FETCH_RD_RS_CONST
			os << "WINT(" << RD << " + " << IMM << ", " << RS << "); "
			"WINT(" << RD << " + " << IMM + 4 << ", " << REG(rs+1) << ");";
		EOP;

		OPC(RET)
			switch(f.ci.returnType) {
			case eVoid: os << "return;"; break;
			case eInt: os << "return r0;"; USE_I(REG_r0); break;
			case eFloat: os << "return f8;"; USE_F(8); break;
			case eLong:
				os << "return RETURN_DI(r0, r1);";
				USE_I(REG_r0);
				USE_I(REG_r1);
			break;
			case eComplexFloat:
				os << "return RETURN_CF(f8, f9);";
				USE_F(8);
				USE_F(9);
			break;
			case eComplexInt:
				os << "return RETURN_CI(r0, r1);";
				USE_I(REG_r0);
				USE_I(REG_r1);
			break;
			}
		EOP;

		OPC(CALLR)
		{
			FETCH_RD;
			CallMap::const_iterator itr = gCallMap.find(ip);
			if(itr == gCallMap.end()) {
				os << "Broken callreg @ " << ip << " - 2\n";
				return ip;
			}
			const CallInfo& ci(itr->second);

			// ensure that we will get a handler, even if it's empty.
			if(gFunctionPointerMap[ci].size() == 0)// ||
				//(ci.returnType == eVoid && ci.intParams == 4 && ci.floatParams == 2))
			{
#if 0	// debug
				const char* type = NULL;
				switch(ci.returnType) {
				case eVoid: type = "V"; break;
				case eInt: type = "I"; break;
				case eFloat: type = "F"; break;
				case eLong: type = "L"; break;
				case eComplexFloat: type = "CF"; break;
				case eComplexInt: type = "CI"; break;
				}
				printf("empty callReg%s%i%i\n", type, ci.intParams, ci.floatParams);
#endif
			}

			streamReturnType1(ci.returnType);
			streamCallRegName(os, ci);
			os << "(" << RD;
			streamParameters(os, ci, false);
			streamReturnType2(ci.returnType);
			os << ";";
		}
		EOP;
		OPC(CALLI)
		{
			FETCH_CONST;
			Function dummy;
			dummy.start = IMM;
			set<Function>::const_iterator itr = functions.find(dummy);
			//printf("Searched for function at 0x%x\n", IMM);
			if(itr == functions.end()) {
				os << "Broken function @ " << IMM << "\n";
				return ip;
			}
			DEBUG_ASSERT(itr != functions.end());
			const Function& cf(*itr);
			streamReturnType1(cf.ci.returnType);
			streamFunctionCall(os, data.cs, cf);
			streamReturnType2(cf.ci.returnType);
			os << ";";
		}
		EOP;

#define JCI(oper) os << "if (" << RD << " " #oper " " << RS << ") { goto " << label(IMM) <<"; }"
		OPC(JC_EQ) 	FETCH_RD_RS_CONST	JCI(==);	EOP;
		OPC(JC_NE)	FETCH_RD_RS_CONST	JCI(!=);	EOP;
		OPC(JC_GE)	FETCH_RD_RS_CONST	JCI(>=);	EOP;
		OPC(JC_GT)	FETCH_RD_RS_CONST	JCI(>);	EOP;
		OPC(JC_LE)	FETCH_RD_RS_CONST	JCI(<=);	EOP;
		OPC(JC_LT)	FETCH_RD_RS_CONST	JCI(<);	EOP;

#define JCF(oper) os << "if (" << FRD << " " #oper " " << FRS << ") { goto " << label(IMM) <<"; }"
		OPC(FJC_EQ) FETCH_FRD_FRS_CONST	JCF(==);	EOP;
		OPC(FJC_NE)	FETCH_FRD_FRS_CONST	JCF(!=);	EOP;
		OPC(FJC_GE)	FETCH_FRD_FRS_CONST	JCF(>=);	EOP;
		OPC(FJC_GT)	FETCH_FRD_FRS_CONST	JCF(>);	EOP;
		OPC(FJC_LE)	FETCH_FRD_FRS_CONST	JCF(<=);	EOP;
		OPC(FJC_LT)	FETCH_FRD_FRS_CONST	JCF(<);	EOP;

#define JCU(oper) os << "if (" << RDU << " " #oper " " << RSU << ") { goto " << label(IMM) << "; }"
		OPC(JC_LTU)	FETCH_RD_RS_CONST	JCU(<);	EOP;
		OPC(JC_GEU)	FETCH_RD_RS_CONST	JCU(>=);	EOP;
		OPC(JC_GTU)	FETCH_RD_RS_CONST	JCU(>);	EOP;
		OPC(JC_LEU)	FETCH_RD_RS_CONST	JCU(<=);	EOP;

		OPC(JPI)	FETCH_CONST	os << "goto " << label(IMM) << ";";	EOP;

/*
    The index to dispatch on, which has mode `SImode'.
    The lower bound for indices in the table, an integer constant.
    The total range of indices in the table--the largest index minus the smallest one (both inclusive).
    A label that precedes the table itself.
    A label to jump to if the index has a value outside the bounds.
*/
		OPC(CASE)
		{
			FETCH_RD_CONST;
			unsigned low = IMM;
			FETCH_CONST;
			unsigned count = IMM;
			FETCH_CONST;
			unsigned tableAddr = IMM;
			FETCH_CONST;
			unsigned defaultLabel = IMM;
			//printf("case %s, %u, %u, %u, %u\n", RD, low, count, tableAddr, defaultLabel);
			//_flushall();
			os << "switch(" << RD << ") {\n";
			for(unsigned i=0; i<=count; i++) {
				os << "\t\tcase " << (int)(low + i) << ": goto " << label(*((int*)(dataBytes + tableAddr) + i)) << ";\n";
			}
			os << "\t\tdefault: goto " << label(defaultLabel) << ";\n";
			os << "\t}";
		}
		EOP;

		OPC(XB)	FETCH_RD_RS	os << RD << " = ((" << RS << " & 0x80) == 0) ? (" << RS << " & 0xFF) : (" << RS << " | ~0xFF);"; EOP;
		OPC(XH)	FETCH_RD_RS	os << RD << " = ((" << RS << " & 0x8000) == 0) ? (" << RS << " & 0xFFFF) : (" << RS << " | ~0xFFFF);"; EOP;

		OPC(SYSCALL)
			FETCH_IMM8;
			// warning: only works for mapip2_syscalls.s.
			if(f.ci.returnType != eVoid) {
				os << "return ";
			}
			if(data.cs)
				os << "mSyscalls.";
			else
				os << "SYSCALL(";
			streamFunctionCall(os, data.cs, f, true);
			if(!data.cs)
				os << ')';
			os << ";\n";
			// skip return instruction
			DEBUG_ASSERT(IB == OP_RET);
			return ip;
		EOP;

		OPC(NOP)
			os << "//NOP";
		EOP;
	default:
		printf("Unknown instruction 0x%x @ 0x%x!\n", op, ip);
		DEBIG_PHAT_ERROR;
	}
	return ip;
}

void CCore::checkFunctionPointer(unsigned ip) {
	// if ip has a reloc to the text section, it is a function pointer.
	// make sure that the value points to a valid function, then add the value to the callReg map
	RelocMap::const_iterator itr = data.textRelocMap.find(ip);
	if(itr == data.textRelocMap.end())
		return;
	const Elf32_Rela& r(data.textRela[itr->second]);
	setCallRegDataRef(data.symbols, data.strtab, r, data.cr);
}
