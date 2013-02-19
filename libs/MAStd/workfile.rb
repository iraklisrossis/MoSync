#!/usr/bin/ruby

require File.expand_path('../../rules/mosync_lib.rb')

mod = Module.new
mod.class_eval do
	def setup_native
		setup_base
		@SOURCES = ['../ResCompiler']
		@EXTRA_SOURCEFILES = ["conprint.c", "ma.c", "maassert.c", "mactype.c", "madmath.c",
			"mastdlib.c", "mastring.c", "mavsprintf.c", "maxtoa.c", "maheap.c"]
		@SPECIFIC_CFLAGS = @native_specific_cflags

		@LOCAL_DLLS = ["mosync"]
	end

	def copySubHeaders(name)
		@INSTALL_INCDIR = name
		@HEADER_DIRS = [name]
		copyHeaders
	end

	def copyResCompilerHeaders()
		@INSTALL_INCDIR = "ResCompiler"
		@HEADER_DIRS = ["../ResCompiler"]
		@IGNORED_HEADERS = ["ByteArrayStream.h", "rescomp.h", "rescompdefines.h", "ResourceSetLookup.h", "VariantResourceLookup.h"]
		copyHeaders
	end

	def setup_pipe
		setup_base
		@SOURCES = [".", "../libsupc++", "libgcc", "../ResCompiler"]
		@EXTRA_INCLUDES = []
		@IGNORED_FILES << 'new_handler.cc'
		@SPECIFIC_CFLAGS = @pipe_specific_cflags
		if(CONFIG=="debug")
			@EXTRA_CFLAGS = " -DMOSYNCDEBUG"
		end

		if(USE_GNU_BINUTILS)
			@IGNORED_FILES << 'crt0.s'
			@IGNORED_FILES << 'e_pow.c'
			@IGNORED_FILES << 'e_log.c'
			@IGNORED_FILES << 'e_atan2.c'
		else
			@EXTRA_OBJECTS = [FileTask.new(self, "crtlib.s"), FileTask.new(self, "mastack.s")]
		end
		@prerequisites << CopyFileTask.new(self, mosync_include + "/" + @INSTALL_INCDIR + "/new",
			FileTask.new(self, "../libsupc++/new"))
		@prerequisites << CopyFileTask.new(self, mosync_include + "/" + @INSTALL_INCDIR + "/macpp.h",
			FileTask.new(self, "../libsupc++/macpp.h"))
	end

	def setup_base
		@IGNORED_FILES = []
		if(CONFIG == "" && @GCC_IS_V4)
			#broken compiler/stdlib
			native_specflags = {"conprint.c" => " -Wno-unreachable-code"}
			if(@GCC_V4_SUB == 4)
				native_specflags["conprint.c"] += " -Wno-inline"
			end
		else
			native_specflags = {}
		end
		if(CONFIG == "")
			pipe_specflags = {"strtod.c" => " -Wno-unreachable-code",
				"tlsf.c" => " -Wno-unreachable-code"}
			# broken compiler
			native_specflags["mastdlib.c"] = " -Wno-unreachable-code"
			native_specflags["mastring.c"] = " -Wno-unreachable-code"
			native_specflags["mawstring.c"] = " -Wno-unreachable-code"
			native_specflags["mavsprintf.c"] = " -Wno-unreachable-code"
			native_specflags["mawvsprintf.c"] = " -Wno-unreachable-code"
		else
			pipe_specflags = {}
		end
		if(@GCC_IS_V4)
			pipe_specflags['mastring.c'] = ' -Wno-pointer-sign'
		end

		@native_specific_cflags = {
			"madmath.c" => " -Wno-missing-declarations",
			"mavsprintf.c" => " -Wno-float-equal -Wno-missing-format-attribute",
			"mawvsprintf.c" => " -Wno-float-equal",
			'VariantResourceLookup.cpp' => ' -Wno-vla',
			}.merge(native_specflags, &HashMergeAdd)

		@pipe_specific_cflags = @native_specific_cflags.merge({
			"intrinsics.c" => " -Wno-missing-prototypes -Wno-missing-declarations",
			"madmath.c" => " -Wno-missing-prototypes -Wno-missing-declarations -fno-builtin-isnan",
			"maint64.c" => " -fno-strict-aliasing -Wno-missing-prototypes -Wno-missing-declarations",
			"libgcc2.c" => " -Wno-unreachable-code",
			"strtod.c" => " -Wno-float-equal" + @GCC_WNO_UNUSED_BUT_SET_VARIABLE,
			"e_log.c" => " -Wno-float-equal",
			"s_atan.c" => " -fno-strict-aliasing",
			"e_atan2.c" => " -fno-strict-aliasing",
			"e_asin.c" => " -fno-strict-aliasing",
			"mastdlib.c" => " -Wno-deprecated-declarations",
			}, &HashMergeAdd).merge(pipe_specflags, &HashMergeAdd)

		copySubHeaders('GLES')
		copySubHeaders('GLES2')
		copySubHeaders('bits')
		copyResCompilerHeaders()

		@HEADER_DIRS = ["."]
		@INSTALL_INCDIR = "."
		@IGNORED_HEADERS = ["math_private.h", "fdlibm.h"]
		@NAME = "mastd"
	end
end

MoSyncLib.invoke(mod)
