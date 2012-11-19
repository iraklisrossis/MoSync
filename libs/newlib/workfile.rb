#!/usr/bin/ruby

require File.expand_path('../../rules/mosync_lib.rb')

default_const(:USE_NEWLIB, true)
if(!USE_NEWLIB)
	error "Must USE_NEWLIB!"
end

mod = Module.new
mod.class_eval do
	def copyHeaderDir(name)
		@INSTALL_INCDIR = name
		@HEADER_DIRS = ["libc/include/" + name]
		copyHeaders
	end

	def copyGlHeaders()
		@INSTALL_INCDIR = "GLES"
		@HEADER_DIRS = ["../MAStd/GLES"]
		copyHeaders
	end

	def copyGl2Headers()
		@INSTALL_INCDIR = "GLES2"
		@HEADER_DIRS = ["../MAStd/GLES2"]
		copyHeaders
	end

	def copyResCompilerHeaders()
		@INSTALL_INCDIR = "ResCompiler"
		@HEADER_DIRS = ["../ResCompiler"]
		@IGNORED_HEADERS = ["ByteArrayStream.h", "rescomp.h", "rescompdefines.h", "ResourceSetLookup.h", "VariantResourceLookup.h"]
		copyHeaders
	end

	def setup_pipe
		@IGNORED_FILES = []
		@SOURCES = ["libc/sys/mosync", "../libsupc++", "libc/sys/mosync/quad",
			"libc/misc", "libc/unix", "libc/posix", "libc/locale", "libc/reent", "libc/stdio",
			"libc/search", "libc/stdlib", "libc/string", "libc/time", "libc/ctype", "libc/errno",
			"libm/math", "libm/common", "../ResCompiler"]
		@EXTRA_INCLUDES = ["libc/include", "libc/sys/mosync", "libm/common"]

		if(@GCC_IS_ARM)
			#@IGNORED_FILES << 'matask.c'
			#@IGNORED_FILES << 'macpp.cpp'
		else
			@SOURCES << 'libc/sys/mosync/libgcc'
		end

		if(USE_GNU_BINUTILS)
			@IGNORED_FILES << 'crtlib.s'
			@EXTRA_SOURCEFILES << '../MAStd/crtlib.s'
			@EXTRA_SOURCEFILES << '../MAStd/mapip2_builtins.s'
			@EXTRA_SOURCEFILES << '../MAStd/mastack.s'
			@EXTRA_SOURCEFILES << '../MAStd/intrinsics.c'
			@EXTRA_SOURCEFILES << '../MAStd/madmath.c'
		else
			@EXTRA_OBJECTS = [FileTask.new(self, 'libc/sys/mosync/crtlib.s')]
		end

		@IGNORED_FILES << 'engine.c'
		@IGNORED_FILES << 'rename.c'
		@IGNORED_FILES << 'memset.c'
		@IGNORED_FILES << 'memcpy.c'

		@EXTRA_CFLAGS = " -DUSE_EXOTIC_MATH -Wno-float-equal -Wno-unreachable-code -Wno-sign-compare -Wno-old-style-definition"
		if(CONFIG=="")
			# buggy compiler, buggy libs... I won't fix them.
			@EXTRA_CFLAGS += " -Wno-uninitialized"
		end
		if(CONFIG=="debug")
			@EXTRA_CFLAGS += " -DMOSYNCDEBUG"
		end
		@SPECIFIC_CFLAGS = {
			"dtoa.c" => " -Wno-shadow -Wno-write-strings",
			"ldtoa.c" => " -Wno-shadow",
			"vfprintf.c" => " -Wno-shadow -Wno-missing-format-attribute -Wno-write-strings -Wno-missing-declarations -Wno-missing-prototypes",
			"svfprintf.c" => " -Wno-shadow -Wno-missing-format-attribute -Wno-write-strings -Wno-missing-declarations -Wno-missing-prototypes",
			"vfwprintf.c" => " -Wno-shadow -Wno-missing-format-attribute -Wno-write-strings" + @GCC_WNO_POINTER_SIGN,
			"svfwprintf.c" => " -Wno-shadow -Wno-missing-format-attribute -Wno-write-strings" + @GCC_WNO_POINTER_SIGN,
			"vfscanf.c" => " -Wno-shadow -Wno-missing-declarations -Wno-missing-prototypes" + @GCC_WNO_POINTER_SIGN,
			"svfscanf.c" => " -Wno-shadow -Wno-missing-declarations -Wno-missing-prototypes" + @GCC_WNO_POINTER_SIGN,
			"collate.c" => @GCC_WNO_POINTER_SIGN,
			"vasprintf.c" => @GCC_WNO_POINTER_SIGN,
			"asprintf.c" => @GCC_WNO_POINTER_SIGN,
			"impure.c" => " -Wno-extra",
			"madmath.c" => " -Wno-missing-prototypes -Wno-missing-declarations",
			"maint.c" => " -Wno-missing-prototypes -Wno-missing-declarations",
			"intrinsics.c" => " -Wno-missing-prototypes -Wno-missing-declarations",
			"logbl.c" => " -Wno-missing-prototypes -Wno-missing-declarations",
			"log2l.c" => " -Wno-missing-prototypes -Wno-missing-declarations",
			"machine.c" => " -Wno-missing-noreturn -D_COMPILING_NEWLIB",
			"gdtoa-gethex.c" => " -Wno-shadow",
			"strtod.c" => " -Wno-shadow",
			"wctomb.c" => " -Wno-shadow",
			"wctomb_r.c" => " -Wno-shadow",
			"sf_ldexp.c" => " -Wno-shadow",
			"s_ldexp.c" => " -Wno-shadow",
			"e_pow.c" => " -Wno-shadow",
			"ef_pow.c" => " -Wno-shadow",
			"s_floor.c" => " -Wno-shadow",
			"sf_floor.c" => " -Wno-shadow",
			"s_ceil.c" => " -Wno-shadow",
			"sf_ceil.c" => " -Wno-shadow",
			"ef_hypot.c" => " -Wno-shadow",
			"s_rint.c" => " -Wno-shadow",
			"s_lrint.c" => " -Wno-shadow",
			"s_llrint.c" => " -Wno-shadow",
			"sf_rint.c" => " -Wno-shadow",
			"sf_lrint.c" => " -Wno-shadow",
			"sf_llrint.c" => " -Wno-shadow",
			"s_modf.c" => " -Wno-shadow",
			"sf_modf.c" => " -Wno-shadow",
			"e_hypot.c" => " -Wno-shadow",
			"regexec.c" => " -Wno-char-subscripts",
			"regcomp.c" => " -Wno-char-subscripts",
			"mktemp.c" => " -DHAVE_MKDIR",
			'libgcc2.c' => ' -Wno-declaration-after-statement',
		}

		# copy subdirs
		copyHeaderDir("sys")
		copyHeaderDir("machine")
		copyGlHeaders()
		copyGl2Headers()
		copyResCompilerHeaders()

		@prerequisites << CopyFileTask.new(self, mosync_include + '/new',
			FileTask.new(self, '../libsupc++/new'))
		@prerequisites << CopyFileTask.new(self, mosync_include + '/macpp.h',
			FileTask.new(self, '../libsupc++/macpp.h'))

		@HEADER_DIRS = ["libc/include", "libc/sys/mosync"]
		@INSTALL_INCDIR = "."

		@NAME = "newlib"
	end
end

MoSyncLib.invoke(mod)
