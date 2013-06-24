# Copyright (C) 2009 Mobile Sorcery AB
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to the Free
# Software Foundation, 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

require "#{File.dirname(__FILE__)}/config.rb"
require "#{File.dirname(__FILE__)}/host.rb"
require "#{File.dirname(__FILE__)}/task.rb"
require "#{File.dirname(__FILE__)}/gcc_flags.rb"
require "#{File.dirname(__FILE__)}/loader_md.rb"
require "#{File.dirname(__FILE__)}/flags.rb"

def get_gcc_version_info(gcc)
	return bb10_get_gcc_version_info() if(gcc == :bb10)

	puts "get_gcc_version_string(#{gcc})" if(PRINT_GCC_VERSION_INFO)
	info = {}
	file = open("|#{gcc} -v 2>&1")
	file.each do |line|
		parts = line.split(/ /)
		#puts "yo: #{parts.inspect}"
		if(parts[0] == 'Target:' && parts[1].strip == 'arm-elf')
			info[:arm] = true
		end
		if(parts[0] == "gcc" && parts[1] == "version")
			info[:ver] = parts[2].strip
		elsif(parts[0] == 'clang' && parts[1] == 'version')
			info[:clang] = true
			info[:ver] = parts[2].strip
		end
	end
	if(!info[:ver])
		puts gcc
		error("Could not find gcc version.")
	end
	info[:string] = ''
	info[:string] << 'arm-' if(info[:arm])
	info[:string] << 'clang-' if(info[:clang])
	info[:string] << info[:ver]
	return info
end

#warning("GCC version: #{GCC_VERSION}")
#warning("GCC_IS_V4: #{GCC_IS_V4}")
#warning("GCC_IS_V43: #{GCC_IS_V43}")

# Compiles a source file using gcc.
# Generates extra files for tracking dependencies and flags,
# so that if the flags or any dependency have changed, this file will be recompiled.
# Objects of this class are created by GccWork.
class CompileGccTask < FileTask
	def initialize(work, gcc, name, source, cflags)
		super(work, name)
		@SOURCE = source
		@prerequisites << source
		@gcc = gcc

		@DEPFILE = @work.genfile(source, ".mf")
		@TEMPDEPFILE = @work.genfile(source, ".mft")
		@FLAGS = cflags + depFlags

		initFlags

		# only if the file is not already needed do we care about extra dependencies
		if(!needed?(false)) then
			@prerequisites += MakeDependLoader.load(@DEPFILE, @NAME)
		end
	end

	def depFlags
		if(@work.gcc == :bb10)
			return " -Wp,-MM,-MF,\"#{@TEMPDEPFILE}\""
		else
			return " -MMD -MF \"#{@TEMPDEPFILE}\""
		end
	end

	def needed?(log = true)
		return true if(super(log))
		if(!File.exists?(@DEPFILE))
			puts "Because the dependency file is missing:" if(log)
			return true
		end
		return flagsNeeded?(log)
	end

	def cFlags
		return "#{@FLAGS} #{@work.gccmode} \"#{File.expand_path_fix(@SOURCE)}\""
	end

	def execute
		execFlags
		begin
			sh "#{@gcc} -o \"#{@NAME}\"#{cFlags}"
		rescue => e
			# in case gcc output a broken object file
			FileUtils.rm_f(@NAME)
			raise
		end

		# In certain rare cases (error during preprocess caused by a header file)
		# gcc may output an empty dependency file, resulting in an empty dependency list for
		# the object file, which means it would not be recompiled, even though it should be.
		# Having gcc output the dependency file to a temporary location fixes the problem.
		if(File.exist?(@TEMPDEPFILE))
			if(@work.gcc == :bb10)
				text = ''
				open(@TEMPDEPFILE, 'r') do |file|
					text = file.read
				end
				open(@DEPFILE, 'w') do |file|
					file.write(File.dirname(@DEPFILE) + '/' + text)
				end
				FileUtils.rm(@TEMPDEPFILE)
			else
				FileUtils.mv(@TEMPDEPFILE, @DEPFILE)
			end
		else
			# Some .s files generate no dependency file when compiled.
			FileUtils.touch(@DEPFILE)
		end
	end

	include FlagsChanged
end

# implementations of GccWork should include this module and implement gccVersionClass.
module GccVersion
	def set_defaults
		if(!gccVersionClass.class_variable_defined?(:@@GCC_IS_V4))
			gcc_version_info = get_gcc_version_info(gcc)
			@@gcc_version_info = gcc_version_info
			is_v4 = gcc_version_info[:ver][0] == "4"[0]
			set_class_var(gccVersionClass, :@@GCC_IS_V4, is_v4)
			if(is_v4)
				set_class_var(gccVersionClass, :@@GCC_V4_SUB, gcc_version_info[:ver][2, 1].to_i)
			end

			# Assuming for the moment that clang is command-line-compatible with gcc 4.2.
			isClang = gcc_version_info[:clang]
			if(isClang)
				set_class_var(gccVersionClass, :@@GCC_IS_V4, true)
				set_class_var(gccVersionClass, :@@GCC_V4_SUB, 2)
			end
			set_class_var(gccVersionClass, :@@GCC_IS_CLANG, isClang)
			set_class_var(gccVersionClass, :@@GCC_IS_ARM, gcc_version_info[:arm])
		end
		@gcc_version_info = @@gcc_version_info
		@GCC_IS_CLANG = get_class_var(gccVersionClass, :@@GCC_IS_CLANG)
		@GCC_IS_ARM = get_class_var(gccVersionClass, :@@GCC_IS_ARM)
		@GCC_IS_V4 = get_class_var(gccVersionClass, :@@GCC_IS_V4)
		if(@GCC_IS_V4)
			@GCC_V4_SUB = get_class_var(gccVersionClass, :@@GCC_V4_SUB)
		end

		@GCC_WNO_UNUSED_BUT_SET_VARIABLE = ''
		@GCC_WNO_POINTER_SIGN = ''
		if(@GCC_IS_V4 && @GCC_V4_SUB >= 6)
			@GCC_WNO_UNUSED_BUT_SET_VARIABLE = ' -Wno-unused-but-set-variable'
			@GCC_WNO_POINTER_SIGN = ' -Wno-pointer-sign'
		end
		@GCC_IS_QCC = @gcc_version_info[:qcc]
		super
	end
end

# Base class.
# Compiles C/C++ code into an executable file.
# Supports GCC on mingw, pipe and linux.
# Uses the following variables: @SOURCES, @IGNORED_FILES, @EXTRA_SOURCEFILES,
# @SPECIFIC_CFLAGS and @EXTRA_OBJECTS.
# Requires subclasses to implement methods 'gcc', 'gccmode' and 'object_ending'.
class GccWork < BuildWork
	def initialize
		@TARGET_PLATFORM = HOST
	end

	# Returns a path representing a generated file, given a source filename and a new file ending.
	def genfile(source, ending)
		@BUILDDIR + File.basename(source.to_s).ext(ending)
	end

	# The filename of the target.
	def target
		@TARGET
	end

	def target_platform
		@TARGET_PLATFORM
	end

	private

	def isPipeWork; false; end

	include GccFlags

	def compareFileTasks(a, b)
		if(File.exist?(b.to_s))
			bd = b.newDate
		else
			bd = LATE
		end
		if(File.exist?(a.to_s))
			ad = a.newDate
		else
			ad = LATE
		end
		bd <=> ad
	end

	def setup2
		define_cflags
		@CFLAGS_MAP = { ".c" => @CFLAGS,
			".cpp" => @CPPFLAGS,
			".cc" => @CPPFLAGS,
			".C" => @CPPFLAGS,
		}

		#find source files
		cfiles = collect_files(".c")
		cppfiles = collect_files(".cpp") + collect_files(".cc") + collect_files('.C')

		sExt = '.S' if(USE_ARM && @COLLECT_S_FILES)
		sExt = '.s' if(USE_GNU_BINUTILS && @COLLECT_S_FILES)
		if(sExt)
			@CFLAGS_MAP[sExt] = @CFLAGS if(USE_ARM)
			@CFLAGS_MAP[sExt] = ' -Wa,--gstabs' if(USE_GNU_BINUTILS)
			#@CFLAGS_MAP[sExt] = '' if(USE_GNU_BINUTILS)
			sfiles = collect_files(sExt)
			cfiles += sfiles
		end

		if(TARGET == :darwin)
			@CFLAGS_MAP[".mm"] = @CPPFLAGS
			cppfiles += collect_files(".mm")
		end

		@all_sourcefiles = cfiles + cppfiles
		@all_sourcefiles.sort! do |a,b| compareFileTasks(a, b) end

		@source_objects = objects(@all_sourcefiles)
		all_objects = @source_objects + @EXTRA_OBJECTS

		setup3(all_objects, !cppfiles.empty?)
 	end

	def check_extra_sourcefile(file, ending)
		return false if(file.getExt != ending)
		raise "Extra sourcefile '#{file}' does not exist!" if(!File.exist?(file))
		return true
	end

	# returns an array of FileTasks
	def collect_files(ending)
		files = @SOURCES.collect {|dir| Dir[dir+"/*"+ending]}
		files.flatten!
		files.reject! {|file| @IGNORED_FILES.member?(File.basename(file)) ||
			!file.end_with?(ending)}	# this one's for windows, whose Dir[] implementation is not case-sensitive.
		files += @EXTRA_SOURCEFILES.select do |file| check_extra_sourcefile(file, ending) end
		tasks = files.collect do |file| FileTask.new(self, file) end
		extra_tasks = @EXTRA_SOURCETASKS.select do |file| file.to_s.getExt == ending end
		# todo: make sure all sourcetasks are collected by one of the calls to this function.
		return extra_tasks + tasks
	end

	def getGccFlags(source)
		ext = source.to_s.getExt
		cflags = @CFLAGS_MAP[ext]
		if(cflags == nil) then
			error "Bad ext: '#{ext}' from source '#{source}'"
		end
		need(:@SPECIFIC_CFLAGS)
		cflags += @SPECIFIC_CFLAGS.fetch(File.basename(source.to_s), "")
		return cflags
	end

	# Returns the command to invoke the compiler for the specified file type.
	# Override this if you need different compiler commands for different languages.
	def getGccInvoke(ending)
		return gcc
	end

	def makeGccTask(source, ending)
		objName = genfile(source, ending)
		task = CompileGccTask.new(self, getGccInvoke(source.to_s.getExt), objName, source, getGccFlags(source))
		return task
	end

	# returns an array of CompileGccTasks
	def objects(sources)
		return sources.collect do |path| makeGccTask(path, object_ending) end
	end
end
