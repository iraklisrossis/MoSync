SKIPPED = [
'20010724-1.c', #platform.specific
'990413-2.c', #platform.specific
'bcp-1.c', #fails.on.purpose
'stdio-opt-1.c', #stdio
'stdio-opt-2.c', #stdio
'stdio-opt-3.c', #stdio
'20020720-1.c', #fails.on.purpose
'20030125-1.c', #fails.on.purpose
#'960512-1.c', #complex.type
#'complex-1.c', #complex.type
#'complex-2.c', #complex.type
#'complex-3.c', #complex.type
#'complex-4.c', #complex.type
'complex-5.c', #complex builtins
#'complex-6.c', #complex.type
'ffs-1.c', #builtin.function
'ffs-2.c', #builtin.function
'nestfunc-1.c', #no.trampolines
'nestfunc-2.c', #no.trampolines
'nestfunc-3.c', #no.trampolines
'nestfunc-4.c', #no.nested.functions, please
'nestfunc-5.c', #no.trampolines
'nestfunc-6.c', #todo.qsort
'bitfld-1.c', #fails.on.purpose
'20010122-1.c', #fails.on.purpose
'20000822-1.c', #no.trampolines
'921215-1.c', #no.trampolines
'931002-1.c', #no.trampolines
'eeprof-1.c', #profiler
'941014-1.c', #pipe-tool reports: illegal function pointer manipulation.
'20101011-1.c',	# integer division by zero
'align-3.c',	# illegal alignment
'pr47237.c',	# __builtin_apply() not supported yet.
'20030501-1.c', #no.trampolines
'20040302-1.c',	# indirect jump
'20040520-1.c', #no.trampolines
'20041214-1.c',	# nested function
'20061220-1.c',	# nested function
'20071210-1.c',	# indirect jump
'20071220-1.c',	# indirect jump
'20090219-1.c', # nested function
'920302-1.c', # indirect jump
'920415-1.c', # indirect jump
'921017-1.c',#nf
'980526-1.c',#ij
'built-in-setjmp.c',#ij
'nest-align-1.c',#nf
'nestfunc-7.c',#nf
'pr22061-3.c',#nf
'pr22061-4.c',#nf
'compile/20010903-2.c',	# trampolines
'compile/20010226-1.c',	# trampolines
'compile/20011029-1.c',	# setjmp
'compile/20031011-1.c',	# trampolines
'compile/20040310-1.c',	# trampolines
'compile/20040317-3.c',	# trampolines
'compile/20040323-1.c',	# alias
'compile/20050119-1.c',	# trampolines
'compile/20050122-1.c',	# trampolines
'compile/20050122-2.c',	# trampolines
'compile/20050510-1.c',	# trampolines
'compile/920502-1.c',	#ij
'compile/920520-1.c',	# broken inline assembly
'compile/920521-1.c',	# broken inline assembly
'compile/920826-1.c',#ij
'compile/920831-1.c',#ij
'compile/930506-2.c',	# trampolines
'compile/930623-1.c',	# __builtin_apply
'compile/991213-3.c',#ij
'compile/calls.c',	# broken calls to immediate non-function addresses
'compile/complex-6.c',#setjmp
'compile/dll.c',	# DLL
'compile/labels-3.c',#ij
'compile/mipscop-1.c',	# MIPS
'compile/mipscop-2.c',	# MIPS
'compile/mipscop-3.c',	# MIPS
'compile/mipscop-4.c',	# MIPS
'compile/nested-1.c',	# trampolines
'compile/nested-2.c',	# trampolines
'compile/pr17913.c',#ij
'compile/pr21356.c',#ij
'compile/pr21728.c',#ij
'compile/pr23237.c',	# section attributes are not supported for this target
'compile/pr25224.c',#ij
'compile/pr27863.c',#ij
'compile/pr27889.c',	# trampolines
'compile/pr28489.c',#ij
'compile/pr29128.c',#ij
'compile/pr30311.c',	# x86
'compile/pr30984.c',#ij
'compile/pr32919.c',#ij
'compile/pr35006.c',#nf
'compile/pr38789.c',	# broken inline assembly
'compile/pr42956.c',	# weird. not valid C, as far as I can tell.
'compile/pr44197.c',	# alias
'compile/pr46107.c',#ij
'unsorted/dump-noaddr.c',	# .x file, too complicated.
#'c-c++-common/asmgoto-2.c',	# fails on purpose
#'c-c++-common/builtin-offsetof.c',	# fails on purpose
#'c-c++-common/fmax-errors.c',	# fails on purpose
'c-c++-common/int128-1.c',	# __int128
'c-c++-common/int128-2.c',	# __int128
'c-c++-common/int128-types-1.c',	# __int128
#'c-c++-common/pr33193.c',	# fails on purpose
#'c-c++-common/pr43690.c',	# fails on purpose
#'c-c++-common/pr51768.c',	# fails on purpose
'c-c++-common/torture/complex-alias-1.c',	# complex builtins
'c-c++-common/torture/complex-sign-mul-minus-one.c',	# complex builtins
'c-c++-common/torture/complex-sign-mul-one.c',	# complex builtins
'c-c++-common/torture/complex-sign-mul.c',	# complex builtins
'c-c++-common/Wunused-var-13.c',#ij
'g++.old-deja/g++.abi/cxa_vec.C',	# setjmp
'g++.old-deja/g++.abi/ptrflags.C',	# typeinfo
'g++.old-deja/g++.abi/vmihint.C',	# typeinfo
'g++.old-deja/g++.benjamin/15071.C',	# iostream
'g++.old-deja/g++.benjamin/typeid01.C',	# typeinfo
'g++.old-deja/g++.brendan/eh1.C',	# exceptions
'g++.old-deja/g++.bugs/900205_02.C', # unparsable by dejaGnu.rb
'g++.old-deja/g++.jason/conversion10.C',	# old, unfixed gcc bug
'g++.old-deja/g++.jason/dcast2.C',	# dynamic_cast
'g++.old-deja/g++.jason/dcast3.C',	# dynamic_cast
'g++.old-deja/g++.jason/thunk3.C',	# variadric thunk
'g++.old-deja/g++.law/operators27.C',	# exceptions
#'g++.old-deja/g++.law/pr25000.C',	# exceptions
'g++.old-deja/g++.law/profile1.C',	# profile mode not supported
'g++.old-deja/g++.martin/new1.C',	# exceptions
#'g++.old-deja/g++.mike/dyncast3.C',	# dynamic_cast
'g++.old-deja/g++.mike/dyncast9.C',	# dynamic_cast
'g++.old-deja/g++.mike/p16146.C',	# dynamic_cast
#'g++.old-deja/g++.mike/p7912.C',	# exceptions
#'g++.old-deja/g++.mike/p9706.C',	# exceptions
'g++.old-deja/g++.oliva/delete3.C',	# exceptions
'g++.old-deja/g++.oliva/new1.C',	# exceptions
'g++.old-deja/g++.oliva/expr1.C',	# error: could not convert '0' from 'void' to 'bool'
'g++.old-deja/g++.oliva/linkage1.C',	# unfixed gcc bug
'g++.old-deja/g++.oliva/ns2.C',	# unfixed gcc bug
'g++.old-deja/g++.other/array1.C',	# exceptions
#'g++.old-deja/g++.other/crash15.C',	# exceptions
#'g++.old-deja/g++.other/dcast2.C',	# dynamic_cast
'g++.old-deja/g++.other/delete3.C',	# exceptions
'g++.old-deja/g++.other/eh3.C',	# exceptions
#'g++.old-deja/g++.other/eh5.C',	# exceptions
'g++.old-deja/g++.other/init7.C',	# exceptions
'g++.old-deja/g++.other/new7.C',	# exceptions
'g++.old-deja/g++.other/singleton.C',	# exceptions
'g++.old-deja/g++.other/syntax1.C',	# -fsyntax-only
'g++.old-deja/g++.other/syntax2.C',	# -fsyntax-only
'g++.old-deja/g++.other/vbase2.C',	# exceptions
'g++.old-deja/g++.pt/const2.C',	# fails on purpose, though it shouldn't. very weird.
'g++.old-deja/g++.pt/crash20.C',	# fails on purpose?
'g++.old-deja/g++.pt/explicit76.C', # fails on purpose?
'g++.old-deja/g++.pt/fntry1.C',	# exceptions
'g++.old-deja/g++.pt/instantiate4.C',	# fails on purpose
'g++.old-deja/g++.pt/instantiate6.C',	# fails on purpose
'g++.old-deja/g++.pt/overload7.C',	# gcc bug
'g++.old-deja/g++.pt/repo1.C',	# fails on purpose?
#'g++.old-deja/g++.robertl/eb123.C',	# exceptions
#'g++.old-deja/g++.robertl/eb124.C',	# exceptions
'g++.old-deja/g++.robertl/eb130.C',	# requires GNU stl
'g++.old-deja/g++.robertl/eb17.C',	# dynamic_cast
'g++.old-deja/g++.robertl/eb50.C',	# exceptions
'g++.old-deja/g++.robertl/eb88.C',	# exceptions
'g++.old-deja/g++.robertl/eh990323-1.C',	# exceptions
'g++.old-deja/g++.robertl/eh990323-2.C',	# exceptions
'g++.old-deja/g++.robertl/eh990323-3.C',	# exceptions
'g++.old-deja/g++.robertl/eh990323-4.C',	# exceptions
'g++.old-deja/g++.robertl/eh990323-5.C',	# exceptions
'g++.old-deja/g++.robertl/ice990323-2.C',	# exceptions

'g++.dg/abi/dcast1.C', #rtti
'g++.dg/abi/mangle60.C', # gcc bug
'g++.dg/charset/asm1.c', # broken asm
'g++.dg/charset/asm3.c', # broken asm
'g++.dg/charset/asm4.c', # broken asm
'g++.dg/charset/asm5.c', # broken asm
'g++.dg/compat/struct-layout-1_generate.c', # not a test program
'g++.dg/cpp0x/constexpr-base3.C',	# exceptions
#'g++.dg/cpp0x/constexpr-defarg2.C',	# exceptions
#'g++.dg/cpp0x/constexpr-potential1.C',	# exceptions
'g++.dg/cpp0x/implicit8.C', # fails on purpose
'g++.dg/cpp0x/initlist48.C', # unique_ptr
'g++.dg/cpp0x/move1.C',	# std::move
'g++.dg/cpp0x/nullptr21.C',	# exceptions
'g++.dg/cpp0x/range-for7.C', # exceptions
'g++.dg/cpp0x/variadic73.C', # exceptions
'g++.dg/dfp/44473-1.C',	# unable to emulate 'SD'
'g++.dg/expr/overflow1.C', # fails on purpose?
'g++.dg/ext/alias-canon2.C', # fails assembly
'g++.dg/ext/asm2.C', # x64 assembly
'g++.dg/ext/asm3.C', # invalid inline asm?
'g++.dg/ext/attrib6.C', # exceptions
'g++.dg/ext/attrib32.C', # duplicate definition. gcc bug?
'g++.dg/ext/has_nothrow_assign.C',#e
'g++.dg/ext/has_nothrow_constructor.C',#e
'g++.dg/ext/has_nothrow_copy-1.C',#e
'g++.dg/ext/label2.C', # indirect jump
'g++.dg/ext/label3.C', # exceptions
'g++.dg/ext/tmplattr9.C', # as: redefined symbol cannot be used on reloc
'g++.dg/init/array16.C', # exceptions
'g++.dg/init/array5.C', # exceptions
'g++.dg/init/copy7.C', # implements memcpy()
'g++.dg/init/ctor1.C', # exceptions
'g++.dg/init/new5.C', # std::nothrow
'g++.dg/init/placement2.C', # exceptions
'g++.dg/init/ref9.C',#e
'g++.dg/lookup/builtin1.C',#e
'g++.dg/lookup/java2.C', # JNI
#'g++.dg/lto/20081109-1_0.C',#e
#'g++.dg/lto/20081118_0.C', # fails on purpose?
'g++.dg/opt/eh2.C',#e
'g++.dg/opt/eh3.C',#e
'g++.dg/opt/pr22167.C',#e
'g++.dg/opt/pr23299.C',#e
'g++.dg/opt/pr23478.C',#e
'g++.dg/opt/pr36449.C',#e
'g++.dg/opt/pr47639.c',#e
'g++.dg/opt/reg-stack.C',#e
'g++.dg/opt/temp1.C', # implements memcpy()
'g++.dg/opt/temp2.C', # implements memcpy()
'g++.dg/other/warning1.C', # error: field initializer is not constant
'g++.dg/overload/conv-op1.C', # fails on purpose
'g++.dg/parse/varmod1.C',#nf
'g++.dg/tc1/dr127.C',#e
'g++.dg/template/builtin1.C', # gcc bug?
'g++.dg/template/crash56.C', # fails on purpose
'g++.dg/template/error43.C', # xfail
'g++.dg/template/fntry1.C',#e
'g++.dg/template/sfinae20.C', # std::declval
'g++.dg/template/sfinae22.C', # std::declval
'g++.dg/torture/pr42462.C',#ij
'g++.dg/torture/pr42739.C',#ij
'g++.dg/torture/pr49115.C',#e
'g++.dg/tree-prof/indir-call-prof-2.C',#e
'g++.dg/tree-ssa/pr20458.C', # xfail?
'g++.dg/tree-ssa/pr21407.C', # rtti
'g++.dg/warn/Walways-true-2.C', # as: redefined symbol
'g++.dg/warn/Wunused-label-3.C',#ij

'gcc.dg/20020312-2.c',#ij
'gcc.dg/20031218-3.c',#nf
'gcc.dg/20081223-1.c',	# fails on purpose
'gcc.dg/Walways-true-2.c',  # gas says: redefined symbol cannot be used on reloc
'gcc.dg/Wunused-var-1.c',#nf
'gcc.dg/alias-5.c',	# require-alias
'gcc.dg/alias-6.c',	# require-alias
'gcc.dg/alias-7.c',	# require-alias
'gcc.dg/always_inline.c',	# fails on purpose
'gcc.dg/always_inline2.c',	# fails on purpose
'gcc.dg/always_inline3.c',	# fails on purpose
'gcc.dg/asm-2.c',	# broken inline assembly
'gcc.dg/asm-3.c',	# broken inline assembly
'gcc.dg/asm-4.c',	# broken inline assembly
'gcc.dg/asm-pr24146.c',	# broken inline assembly
'gcc.dg/attr-alias-2.c',	# alias
'gcc.dg/builtin-apply1.c',	# __builtin_apply
'gcc.dg/builtin-apply2.c',	# __builtin_apply
'gcc.dg/builtin-apply3.c',	# __builtin_apply
'gcc.dg/builtin-apply4.c',	# __builtin_apply
'gcc.dg/builtin-return-1.c',	# __builtin_return
'gcc.dg/builtin-unreachable-1.c',	# broken inline assembly
'gcc.dg/builtins-20.c',	# fails on purpose
'gcc.dg/builtins-57.c',	# fails on purpose
'gcc.dg/builtins-65.c',	# fails on purpose
'gcc.dg/comp-goto-1.c',#ij
'gcc.dg/comp-goto-2.c',#ij
'gcc.dg/cproj-fails-with-broken-glibc.c',	# fails on purpose
'gcc.dg/inline-28.c',#nf
'gcc.dg/inline-29.c',#nf
'gcc.dg/inline-30.c',#nf
'gcc.dg/nested-func-8.c',#nf
'gcc.dg/non-local-goto-1.c',#ij
'gcc.dg/non-local-goto-2.c',#nf+ij
'gcc.dg/old-style-asm-1.c',	# broken inline assembly
'gcc.dg/pr18963-1.c',#nf
'gcc.dg/pr26983.c',#setjmp
'gcc.dg/pr30045.c',#nf
'gcc.dg/pr38338.c',	# __builtin_apply
'gcc.dg/pr43379.c',#ij
'gcc.dg/pr43564.c',	# broken inline assembly
'gcc.dg/pr45259.c',#ij
'gcc.dg/pr47276.c',	# require-alias
'gcc.dg/pr52139.c',#ij
'gcc.dg/pragma-isr-trapa2.c',	# sh-* only
'gcc.dg/charset/asm1.c',	# broken inline assembly
'gcc.dg/charset/asm4.c',	# broken inline assembly
'gcc.dg/charset/asm5.c',	# broken inline assembly
'gcc.dg/charset/asm6.c',	# broken inline assembly
'gcc.dg/compat/scalar-by-value-5.c',	# not meant to be compiled on its own
'gcc.dg/compat/scalar-by-value-6.c',	# not meant to be compiled on its own
'gcc.dg/compat/scalar-by-value-dfp_x.c',	# DFP not supported
'gcc.dg/compat/scalar-by-value-dfp_y.c',	# DFP not supported
'gcc.dg/compat/scalar-return-dfp_x.c',	# DFP not supported
'gcc.dg/compat/scalar-return-dfp_y.c',	# DFP not supported
'gcc.dg/compat/struct-layout-1_generate.c',	# not meant to be compiled on its own
'gcc.dg/cpp/dir-only-6.c', # unterminated comment
'gcc.dg/cpp/error-1.c', # unterminated comment
'gcc.dg/lto/20081120-1_0.c',	# lacks main()
'gcc.dg/lto/20081120-2_0.c',	# lacks main()
'gcc.dg/lto/20081204-1_0.c',	# lacks main()
'gcc.dg/lto/20081212-1_0.c',	# lacks main()
'gcc.dg/lto/20081222_1.c',	# alias
'gcc.dg/lto/20081224_0.c',	# lacks main()
'gcc.dg/lto/20090116_0.c',	# lacks main()
'gcc.dg/special/20000419-2.c',	# alias
'gcc.dg/special/alias-1.c',	# alias
'gcc.dg/special/alias-2.c',	# alias
'gcc.dg/special/wkali-2.c',	# alias
'gcc.dg/special/wkali-2a.c',	# main
'gcc.dg/special/wkali-2b.c',	# alias
'gcc.dg/torture/builtin-math-7.c',	# complex
'gcc.dg/torture/pr46216.c',#ij
'gcc.dg/torture/pr47473.c',	# complex
'gcc.dg/torture/pr48044.c',	# alias
'gcc.dg/tree-prof/pr34999.c',#setjmp
'gcc.dg/vect/pr32224.c',	# broken inline assembly

#bugs below

'builtin-bitops-1.c', #builtin.functions. todo: implement them.

# mapip2 bug, segfault due to "emit_move_insn QImode -> SImode"
#'compile/930503-2.c',	# MINIMAL TESTCASE!
'compile/limits-exprparen.c',	# gcc bug, segfault due to 27796 stack frames.
#'g++.old-deja/g++.law/ctors18.C',	# emit_move_insn SImode -> QImode. This one's different.
#'g++.old-deja/g++.law/cvt3.C',	# emit_move_insn SImode -> QImode.
'g++.old-deja/g++.niklas/t140.C',	# mapip2 bug, missing function stab for constructor in struct nested in function.
'g++.old-deja/g++.other/local1.C',	# mapip2 bug, missing function stab for constructor in struct nested in function.
'g++.old-deja/g++.other/local3.C',	# mapip2 bug, missing function stab for constructor in struct nested in function.
'g++.old-deja/g++.other/lookup6.C',	# mapip2 bug, missing function stab for constructor in struct nested in function.
'g++.old-deja/g++.pt/local6.C',	# mapip2 bug, missing function stab for constructor in struct nested in function.

'gcc.dg/initpri1.c',	# mapip2 bug, constructor priority.
'gcc.dg/initpri3.c',	# mapip2 bug, constructor priority.
'g++.dg/special/conpr-2.C',	# mapip2 bug, constructor priority.
'g++.dg/special/conpr-3.C',	# mapip2 bug, constructor priority.
'g++.dg/special/conpr-4.C',	# mapip2 bug, constructor priority.
'g++.dg/special/initp1.C',	# mapip2 bug, constructor priority.
'g++.dg/special/initpri1.C',	# mapip2 bug, constructor priority.
'gcc.dg/compat/scalar-return-4_x.c',	# mapip2 bug, promote_function_mode(QCImode)
'gcc.dg/compat/scalar-return-4_y.c',	# mapip2 bug, emit_move_insn QCImode -> SImode
'20050121-1.c', # mapip2 bug, emit_move_insn QCImode -> SImode
'gcc.dg/cpp/trigraphs.c',	# does NOT like // comments.
#'gcc.dg/debug/pr41717.c',	# mapip2 bug, complex float
#'gcc.dg/graphite/id-2.c',	# mapip2 bug, complex float
'gcc.dg/torture/fp-int-convert-float.c',	# mapip2 bug, floatdisf2
#'gcc.dg/torture/pr26869.c',	# mapip2 bug, complex float
#'gcc.dg/torture/pr27773.c',	# mapip2 bug, complex float
#'gcc.dg/torture/pr40328.c',	# mapip2 bug, complex float
]

SKIPPED_REGEXP = [
%r{g\+\+.old-deja/g\+\+.abi/vtable3..C},	# typeinfo
%r{g\+\+.old-deja/g\+\+.mike/eh.+.C},	# exceptions
%r{g\+\+.old-deja/g\+\+.other/dyncast..C},	# dynamic_cast
/gcc.dg\/c99-tgmath-..c/,	# tgmath.h
]

# if a dejaGnu test includes any of these lines, it is skipped.
SKIP_LINES = [
'#include <iostream>',
'#include <typeinfo>',
'#include <complex>',
'#include <fstream>',
'#include <iomanip>',
'#include <exception>',
'#include <sstream>',
'#include <unwind.h>',
'#include <setjmp.h>',
'#include "tree-vect.h"',
'#include <sys/mman.h>',
'#include <initializer_list>',
'#include <type_traits>',
'#include <stdexcept>',
]

SKIPPED_DIRS = [
'g++.old-deja/g++.eh',
'g++.dg/eh',
'g++.dg/graphite',
'g++.dg/lto',
'g++.dg/pch',
'g++.dg/plugin',
'g++.dg/rtti',
'gcc.dg/dfp',
'gcc.dg/fixed-point',
'gcc.dg/graphite',
'gcc.dg/lto',	# while we should theoretically support LTO, the tests are too tricky.
'gcc.dg/pch',
'gcc.dg/plugin',
'gcc.dg/vmx',
'gcc.dg/vxworks',
]

if(!USE_NEWLIB)
	SKIP_LINES << '#include <string>'
	SKIP_LINES << '#include <list>'
	SKIP_LINES << '#include <map>'
	SKIP_LINES << '#include <set>'
	SKIP_LINES << '#include <vector>'
	SKIP_LINES << '#include <iterator>'
	SKIP_LINES << '#include <utility>'
	SKIP_LINES << '#include <algorithm>'
	SKIP_LINES << '#include <limits>'
	SKIP_LINES << '#include <memory>'
	#SKIP_LINES << '#include <cstring>'
	#SKIP_LINES << '#include <cassert>'
	#SKIP_LINES << '#include <cstdarg>'
	SKIP_LINES << '#include "guality.h"'
	SKIPPED << 'pr34456.c'	# qsort
	SKIPPED << 'g++.old-deja/g++.jason/template44.C'	# qsort
	SKIPPED << 'g++.old-deja/g++.martin/bitset1.C'	# bitset
	SKIPPED << 'g++.old-deja/g++.mike/ns15.C'	# qsort
	SKIPPED << 'g++.old-deja/g++.ns/using6.C'	# vector
	SKIPPED << 'g++.old-deja/g++.other/bitfld1.C'	# utility
	SKIPPED << 'g++.old-deja/g++.other/init18.C'	# atexit
	SKIPPED << 'g++.old-deja/g++.other/init19.C'	# atexit
	SKIPPED << 'g++.old-deja/g++.other/init5.C'	# atexit
	SKIPPED << 'g++.dg/init/ref15.C'	# atexit
	#SKIPPED << 'g++.dg/opt/dtor4.C'	# <cassert>
	SKIPPED << 'g++.dg/torture/pr31863.C' # atexit
	SKIPPED << 'g++.dg/warn/format1.C' # scanf
	SKIPPED << 'gcc.dg/cdce1.c'	# errno.h
	SKIPPED << 'gcc.dg/cdce2.c'	# errno.h
	SKIPPED << 'gcc.dg/errno-1.c'	# errno.h
	SKIPPED << 'gcc.dg/lto/20081024_0.c'	# vsnprintf
end

if(CONFIG == "")
	SKIPPED << 'fprintf-chk-1.c'	#fails.on.purpose
	SKIPPED << 'vfprintf-chk-1.c'	#fails.on.purpose
	#SKIPPED << 'nest-stdar-1.c'	# mapip2 bug, incorrect vararg stack pointer offset in inner function.
	#SKIPPED << 'pr41239.c'	# mapip2 bug, incorrect vararg stack pointer offset in normal function.
	SKIPPED << 'compile/limits-structnest.c'	# gcc bug, eats 2+ GB RAM in a few seconds. todo: test on Windows.
	SKIPPED << 'compile/pr21840.c'	# illegal call
	SKIPPED << 'gcc.dg/opts-2.c'	# not designed for -O2
	SKIPPED << 'gcc.dg/torture/pr24626-1.c' # call 0

	# strange. seems that if there are no builtins for a certain function, there are link errors.
	SKIPPED << 'gcc.dg/torture/builtin-convert-1.c'
	SKIPPED << 'gcc.dg/torture/builtin-convert-2.c'
	SKIPPED << 'gcc.dg/torture/builtin-convert-3.c'
	SKIPPED << 'gcc.dg/torture/builtin-power-1.c'

	if(!USE_NEWLIB)
	end
else
	SKIPPED << 'pr17377.c'	# mapip2 bug, __builtin_return_address
	#SKIPPED << 'c-c++-common/Wunused-var-8.c'	# mapip2 bug, find_valid_class(DI, SI)
	SKIPPED << 'c-c++-common/restrict-1.c'	# fails on purpose
	SKIPPED << 'gcc.dg/torture/builtin-power-1.c'	# fails on purpose
end

if(defined?(WP7) && WP7)
	# C# bug; conversion from double to ulong loses precision. No viable workarounds exist.
	SKIPPED << 'ieee/rbug.c'
end
