#include "elfStabSld.h"
#include "../../runtimes/cpp/core/gen-opcodes.h"

#include <sstream>
#include <fstream>

static void streamFunctionPrototype(ostream& os, const Function& f, bool cs);
static void streamFunctionContents(const DebuggingData& data,
	const Array0<byte>& textBytes, const Array0<byte>& dataBytes,
	ostream& os, const Function& f, CallRegs& cr, const Array0<Elf32_Rela>& textRela);
static void parseFunctionInfo(Function& f);
static void setCallRegDataRefs(const DebuggingData& data, CallRegs& cr);
static void parseStabParams(const char* comma, unsigned& intParams, unsigned& floatParams);
static void streamFunctionPrototypeParams(ostream& os, bool cs, const CallInfo& ci, bool first = true);
static void streamCallRegPrototype(ostream& os, const CallInfo& ci, bool cs);
static const Function& getFunction(unsigned address);
static void streamCallRegSet(ostream& file, const CallInfo& ci, const FunctionPointerSet& fps, bool cs);

// warning: must match enum ReturnType!
static const char* returnTypeStrings[] = {
	"void",
	"int",
	"double",
	"int64_t",
	"__complex__ double",
	"__complex__ int",
};

// warning: must match enum ReturnType!
static const char* cSharpReturnTypeStrings[] = {
	"void",
	"int",
	"double",
	"long",
	"ComplexDouble",
	"ComplexInt",
};

void writeCpp(const DebuggingData& data, const char* cppName) {
	Array0<byte> textBytes, dataBytes;
	DEBUG_ASSERT(readSegments(data, textBytes, dataBytes));

	// output data_section.bin
	ofstream bin("data_section.bin", ios_base::binary);
	bin.write((const char*)dataBytes.p(), dataBytes.size());
	bin.close();

	// gotta do this first; the others rely on it.
	for(set<Function>::iterator i = functions.begin(); i != functions.end(); ++i) {
		Function& f((Function&)*i);
		parseFunctionInfo(f);
	}

	CallRegs cr;
	setCallRegDataRefs(data, cr);

	ofstream file(cppName);
	gOutputFile = &file;

	if(data.cs)
	{
		file << "public class CoreNativeProgram : MoSync.CoreNative {\n"
			"\n"
			"#pragma warning disable 0164\n"
			"#pragma warning disable 1717\n"
			"#pragma warning disable 1718\n"
			"\n";
	}
	else
	file <<
"//****************************************\n"
"//          Generated Cpp code\n"
"//****************************************\n"
"\n"
"#include \"mstypeinfo.h\"\n"
"\n"
"// Prototypes\n"
"\n";

	ostringstream oss;
	unsigned scope = -1;

	// normal function declarations
	if(!data.cs)
	for(set<Function>::iterator i = functions.begin(); i != functions.end(); ++i) {
		Function& f((Function&)*i);
		if(f.scope != scope) {
			scope = f.scope;
			oss << "// " << files[scope].name << "\n";
		}
		streamFunctionPrototype(oss, f, data.cs);
		oss << ";\n";
	}

	oss <<
"\n"
"// Definitions\n";

	// normal function definitions.
	// this also populates the callReg map.
	scope = -1;
	for(set<Function>::iterator i=functions.begin(); i!=functions.end(); ++i) {
		const Function& f(*i);
		oss << '\n';
		if(f.scope != scope) {
			scope = f.scope;
			oss << "// " << files[scope].name << "\n";
		}
		if(data.cs)
			oss << "private ";
		streamFunctionPrototype(oss, f, data.cs);
		oss << " {\n";
		streamFunctionContents(data, textBytes, dataBytes, oss, f, cr, data.textRela);
		oss << "}\n";
	}

	// callReg declarations
	if(!data.cs)
	for(FunctionPointerMap::const_iterator itr = gFunctionPointerMap.begin();
		itr != gFunctionPointerMap.end(); ++itr)
	{
		const CallInfo& ci(itr->first);
		streamCallRegPrototype(file, ci, data.cs);
		file << ";\n";
	}

	file << oss.str();
	if(!data.cs)
	file << hex << showbase;

	// CallReg definitions
	for(FunctionPointerMap::const_iterator itr = gFunctionPointerMap.begin();
		itr != gFunctionPointerMap.end(); ++itr)
	{
		const CallInfo& ci(itr->first);
		const FunctionPointerSet& fps(itr->second);
		file << '\n';
		streamCallRegPrototype(file, ci, data.cs);
		file << " {\n"
		"\tswitch(pointer) {\n";
		streamCallRegSet(file, ci, fps, data.cs);
		if(ci.returnType == eVoid) {
			// Certain function calls are Void even though their target function is not.
			// Thus, all FPs with identical parameters must be listed here.
			// Known examples thus far:
			// Calls that return structs, but whose return value is ignored. (ex struct-ret-1.c)
			for(FunctionPointerMap::const_iterator jtr = gFunctionPointerMap.begin();
				jtr != gFunctionPointerMap.end(); ++jtr)
			{
				const CallInfo& jci(jtr->first);
				if(jci.intParams == ci.intParams && jci.floatParams == ci.floatParams && jci.returnType != eVoid)
					streamCallRegSet(file, ci, jtr->second, data.cs);
			}
		}
		const char* csPanic = data.cs ? " throw new System.Exception();" : "";
		file << "\tdefault: maPanic(1, \"Invalid callReg\");" << csPanic << "\n"
		"\t}\n"
		"}\n";
	}

	// entry point
	file << "\n";
	if(data.cs)
		file << "public override void Main() {\n";
	else
		file << "void entryPoint() {\n";
	file <<
	"\tint p0 = "<< calculateDataSize(data, dataBytes.size()) <<";\n"
	"\tint p1 = "<< data.stackSize <<";\n"
	"\tint p2 = "<< data.heapSize <<";\n"
	"\tint p3 = "<< data.ctorAddress <<";\n"
	"\tint g0 = "<< data.dtorAddress <<";\n";
	if(data.cs)
		file << "\tInitData(\"data_section.bin\", "<< dataBytes.size() <<", p0);\n";
	file << "\t";
	streamFunctionName(file, data.cs, getFunction(data.entryPoint));
	file << "(p0, p1, p2, p3, g0);\n"
	"}\n";
	if(data.cs)
		file << "}\n";
}

static void streamCallRegSet(ostream& file, const CallInfo& ci, const FunctionPointerSet& fps, bool cs) {
	for(FunctionPointerSet::const_iterator jtr = fps.begin(); jtr != fps.end(); ++jtr) {
		file << "\tcase " << *jtr << ": ";
		if(ci.returnType != eVoid)
			file << "return ";
		streamFunctionCall(file, cs, getFunction(*jtr));
		if(ci.returnType == eVoid)
			file << "; return";
		file << ";\n";
	}
}

static const Function& getFunction(unsigned address) {
	Function dummy;
	dummy.start = address;
	set<Function>::const_iterator itr = functions.find(dummy);
	DEBUG_ASSERT(itr != functions.end());
	return *itr;
}

static void streamCallRegPrototype(ostream& os, const CallInfo& ci, bool cs) {
	if(!cs)
	os << noshowbase << dec;
	if(cs)
		os << "private " << cSharpReturnTypeStrings[ci.returnType] << " ";
	else
		os << "static " << returnTypeStrings[ci.returnType] << " ";
	streamCallRegName(os, ci);
	os << "(int pointer";
	streamFunctionPrototypeParams(os, cs, ci, false);
	if(!cs)
	os << showbase << hex;
}

bool readSegments(const DebuggingData& data, Array0<byte>& textBytes, Array0<byte>& dataBytes)
{
	//Read Program Table
	//LOG("%i segments, offset %X:\n", data.e_phnum, data.e_phoff);

	// first, find the size of the data.
	unsigned datasize = 0, textsize = 0;
	Stream& file(data.elfFile);
	const Elf32_Ehdr& ehdr(data.ehdr);
	for(unsigned i=0; i<ehdr.e_phnum; i++) {
		Elf32_Phdr phdr;
		TEST(file.seek(Seek::Start, ehdr.e_phoff + i * sizeof(Elf32_Phdr)));
		TEST(file.readObject(phdr));
#if 0
		LOG("Type: 0x%X, Offset: 0x%X, VAddress: %08X, PAddress: %08X, Filesize: 0x%X",
			phdr.p_type, phdr.p_offset, phdr.p_vaddr, (phdr.p_paddr), phdr.p_filesz);
		LOG(", Memsize: 0x%X, Flags: %08X, Align: %i\n",
			(phdr.p_memsz), phdr.p_flags, (phdr.p_align));
#endif
		if(phdr.p_type == PT_NULL)
			continue;
		else if(phdr.p_type == PT_LOAD) {
			bool text = (phdr.p_flags & PF_X);
#if 0
			LOG("%s segment: 0x%x, 0x%x bytes\n",
				text ? "text" : "data",
				phdr.p_vaddr, phdr.p_filesz);
#endif
			unsigned& size(text ? textsize : datasize);
			// assume all segments are ordered
			if(phdr.p_vaddr < size) {
				LOG("segment overlap!\n");
				DEBIG_PHAT_ERROR;
			}
			size = phdr.p_vaddr + phdr.p_filesz;
		} else {
			DEBIG_PHAT_ERROR;
		}
	}

	// then we can allocate the buffer and read the data.
	textBytes.resize(textsize);
	dataBytes.resize(datasize);

	for(unsigned i=0; i<ehdr.e_phnum; i++) {
		Elf32_Phdr phdr;
		TEST(file.seek(Seek::Start, ehdr.e_phoff + i * sizeof(Elf32_Phdr)));
		TEST(file.readObject(phdr));
		if(phdr.p_type == PT_NULL)
			continue;
		else if(phdr.p_type == PT_LOAD) {
			bool text = (phdr.p_flags & PF_X);
			TEST(file.seek(Seek::Start, phdr.p_offset));
			void* dst = (text ? textBytes : dataBytes) + phdr.p_vaddr;
			//LOG("Reading 0x%x bytes to %p...\n", phdr.p_filesz, dst);
			TEST(file.read(dst, phdr.p_filesz));
		} else {
			DEBIG_PHAT_ERROR;
		}
	}
	return true;
}

#if 0
typedef struct
{
  Elf32_Addr	r_offset;		/* Address */
  Elf32_Word	r_info;			/* Relocation type and symbol index */
  Elf32_Sword	r_addend;		/* Addend */
} Elf32_Rela;

typedef struct
{
  Elf32_Word	st_name;		/* Symbol name (string tbl index) */
  Elf32_Addr	st_value;		/* Symbol value */
  Elf32_Word	st_size;		/* Symbol size */
  unsigned char	st_info;		/* Symbol type and binding */
  unsigned char	st_other;		/* Symbol visibility */
  Elf32_Section	st_shndx;		/* Section index */
} Elf32_Sym;
#endif

void setCallRegDataRef(const Array0<Elf32_Sym>& symbols, const Array0<char>& strtab, const Elf32_Rela& r, CallRegs& cr) {
	const Elf32_Sym& sym(symbols[ELF32_R_SYM(r.r_info)]);
	const char* symName = strtab + sym.st_name;
#if 0
	printf("stt: %i, val: %i, section %i, name %s, ro %x, ra %x\n",
		ELF32_ST_TYPE(sym.st_info), sym.st_value, sym.st_shndx, symName,
		r.r_offset, r.r_addend);
	fflush(stdout);
#endif
	// assuming here that section 1 is .text
	Function dummy;
	set<Function>::const_iterator itr = functions.end();
	// static function reference; no function symbol, just .text section + addend.
	if(ELF32_ST_TYPE(sym.st_info) == STT_SECTION && sym.st_shndx == 1)
	{
		dummy.start = sym.st_value + r.r_addend;
		itr = functions.find(dummy);
	}
	else
	// standard function reference
	if(ELF32_ST_TYPE(sym.st_info) == STT_FUNC ||
		(ELF32_ST_TYPE(sym.st_info) == STT_NOTYPE && sym.st_shndx == 1))
	{
		dummy.start = sym.st_value;
		itr = functions.find(dummy);
		if(itr == functions.end()) {
			printf("Invalid function pointer @ 0x%x to %s (0x%x) + 0x%x.\n",
				r.r_offset, symName, sym.st_info, r.r_addend);
			DEBIG_PHAT_ERROR;
		}
	}
	if(itr != functions.end())
	{
		const Function& f(*itr);
		//printf("Found function pointer to: %s, type %s\n", f.name, returnTypeStrings[f.ci.returnType]);
		gFunctionPointerMap[f.ci].insert(dummy.start);
	}
}

static void setCallRegDataRefs(const DebuggingData& data, const Array0<Elf32_Rela>& rela, CallRegs& cr) {
	for(size_t i=0; i<rela.size(); i++) {
		const Elf32_Rela& r(rela[i]);
		setCallRegDataRef(data.symbols, data.strtab, r, cr);
	}
}

static void setCallRegDataRefs(const DebuggingData& data, CallRegs& cr)
{
	// We search the relocations in the data segment for references to the code segment,
	// which means function pointers.
	// Together with immediates that will be extracted from the code section,
	// this will be the list of functions that can be called by register.
	printf("rodata: %" PRIuPTR " relas\n", data.rodataRela.size());
	setCallRegDataRefs(data, data.rodataRela, cr);
	printf("data: %" PRIuPTR " relas\n", data.dataRela.size());
	setCallRegDataRefs(data, data.dataRela, cr);
	printf("ctors: %" PRIuPTR " relas\n", data.ctorsRela.size());
	setCallRegDataRefs(data, data.ctorsRela, cr);
	printf("dtors: %" PRIuPTR " relas\n", data.dtorsRela.size());
	setCallRegDataRefs(data, data.dtorsRela, cr);
	printf("Total callRegs: %" PRIuPTR "\n", gFunctionPointerMap.size());
	fflush(stdout);
}

typedef const char* (*getRegNameFunc)(size_t);
static void streamRegisterDeclarations(ostream& os, const char* type, unsigned regUsage, size_t nRegs,
	size_t start, size_t paramLow, size_t paramHi, unsigned nParams, getRegNameFunc grn, bool cs)
{
	if(regUsage != 0) {
		bool first = true;
		for(size_t i=start; i<nRegs; i++) {
			if((regUsage & (1 << i)) == 0)
				continue;
			if(i >= paramLow && i <= paramHi && (i - paramLow) < nParams)
				continue;
			if(first) {
				first = false;
				os << type;
			} else
				os << ", ";
			os << grn(i);
			if(cs && grn == getIntRegName)
				os << "=0";
		}
		if(!first)
			os << ";\n";
	}
}

static void streamFunctionContents(const DebuggingData& data,
	const Array0<byte>& textBytes, const Array0<byte>& dataBytes,
	ostream& os, const Function& f, CallRegs& cr, const Array0<Elf32_Rela>& textRela)
{
	// output instructions
	ostringstream oss;
	SIData pid = { oss, cr, data.elfFile, textBytes, dataBytes,
		textRela, data.symbols, data.strtab, data.textRelocMap, data.cs,
		{0,0} };
	streamFunctionInstructions(pid, f);

	// declare registers
	streamRegisterDeclarations(os, "int ", pid.regUsage.i, nIntRegs, REG_fp,
		REG_p0, REG_p0 + f.ci.intParams, f.ci.intParams, getIntRegName, data.cs);
	streamRegisterDeclarations(os, "double ", pid.regUsage.f, nFloatRegs, 0,
		8, 15, f.ci.floatParams, getFloatRegName, data.cs);

	os << oss.str();
}

static void streamFunctionPrototypeParams(ostream& os, bool cs, const CallInfo& ci, bool first) {
	if(!cs)
	os << dec;
	if(first)
		os << '(';
	for(unsigned j=0; j<ci.intParams; j++) {
		if(first)
			first = false;
		else
			os << ", ";
		os << "int " << getIntRegName(REG_p0 + j);
	}
	for(unsigned j=0; j<ci.floatParams; j++) {
		if(first)
			first = false;
		else
			os << ", ";
		os << "double f" << (8+j);
	}
	os << ')';
	if(!cs)
		os << hex;
}

static void streamFunctionPrototype(ostream& os, const Function& f, bool cs) {
	if(cs)
		os << cSharpReturnTypeStrings[f.ci.returnType] << " ";
	else
		os << returnTypeStrings[f.ci.returnType] << " ";
	streamFunctionName(os, cs, f);
	streamFunctionPrototypeParams(os, cs, f.ci);
}

static bool isctype(int c) {
	return isalnum(c) || c == '_';
}

void streamFunctionName(ostream& os, bool cs, const Function& f, bool syscall) {
	const char* ptr = f.name;
	if(cs && syscall) {
		DEBUG_ASSERT(ptr[0] == '_');
		ptr++;
	}
	while(*ptr) {
		if(isctype(*ptr))
			os << *ptr;
		else
			os << '_';
		ptr++;
	}
	if(!syscall)
	{
		if(!cs)
			os << dec;
		os << '_' << f.scope;
		if(!cs)
			os << hex;
	}
}

static void parseFunctionInfo(Function& f) {
	DEBUG_ASSERT(f.info);

	const char* type = f.info;
	const char* comma = strchr(type, ',');
	DEBUG_ASSERT(comma);
	int tlen = comma - type;
	if(strncmp(type, "void", tlen) == 0)
		f.ci.returnType = eVoid;
	else if(strncmp(type, "int", tlen) == 0)
		f.ci.returnType = eInt;
	else if(strncmp(type, "double", tlen) == 0)
		f.ci.returnType = eFloat;
	else if(strncmp(type, "float", tlen) == 0)
		f.ci.returnType = eFloat;
	else if(strncmp(type, "long", tlen) == 0)
		f.ci.returnType = eLong;
	else if(strncmp(type, "complexFloat", tlen) == 0)
		f.ci.returnType = eComplexFloat;
	else if(strncmp(type, "CSI", tlen) == 0)
		f.ci.returnType = eComplexInt;
	else if(strncmp(type, "CHI", tlen) == 0)
		f.ci.returnType = eComplexInt;
	else if(strncmp(type, "CQI", tlen) == 0)
		f.ci.returnType = eComplexInt;
	else {
		printf("Unknown function return type: %s %s\n", f.name, f.info);
		DEBIG_PHAT_ERROR;
	}

	parseStabParams(comma, f.ci.intParams, f.ci.floatParams);
}

static void parseStabParams(const char* comma, unsigned& intParams, unsigned& floatParams) {
	const char* ip = comma + 1;
	comma = strchr(ip, ',');
	DEBUG_ASSERT(comma);
	int ilen = comma - ip;
	DEBUG_ASSERT(ilen == 1);
	DEBUG_ASSERT(isdigit(*ip));
	intParams = *ip - '0';

	const char* fp = comma + 1;
	int flen = strlen(fp);
	DEBUG_ASSERT(flen == 1);
	DEBUG_ASSERT(isdigit(*fp));
	floatParams = *fp - '0';
}

struct ReturnTypeMapping {
	const char* mode;
	ReturnType type;
};

// returns comma
static const char* parseReturnType(const char* stab, ReturnType& type) {
	static const ReturnTypeMapping mappings[] = {
		{ "VOID", eVoid },
		{ "SI", eInt },
		{ "DI", eLong },
		{ "SF", eFloat },
		{ "DF", eFloat },
		{ "SC", eComplexFloat },
		{ "DC", eComplexFloat },
		{ "CSI", eComplexInt },
		{ "CHI", eComplexInt },
		{ "CQI", eComplexInt },
	};

	const char* comma = strchr(stab, ',');
	DEBUG_ASSERT(comma);
	int len = comma - stab;
	for(size_t i=0; i<ARRAY_SIZE(mappings); i++) {
		const ReturnTypeMapping& m(mappings[i]);
		if(strncmp(m.mode, stab, len) == 0) {
			type = m.type;
			return comma;
		}
	}
	printf("parseReturnType: %s\n", stab);
	DEBIG_PHAT_ERROR;
}

CallInfo parseCallInfoStab(const char* stab) {
	CallInfo ci;
	const char* comma = parseReturnType(stab, ci.returnType);
	parseStabParams(comma, ci.intParams, ci.floatParams);
	return ci;
}



#if 0

static bool strbeg(const char* a, const char* b) {
	return strncmp(a, b, strlen(b)) == 0;
}

if(strend(str, '\\'))
	//combine with next line

#endif
