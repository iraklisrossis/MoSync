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

require "#{File.dirname(__FILE__)}/gcc.rb"
require "#{File.dirname(__FILE__)}/mosync_util.rb"
require "#{File.dirname(__FILE__)}/targets.rb"
require "#{File.dirname(__FILE__)}/config.rb"
require "#{File.dirname(__FILE__)}/native_link.rb"

module MoSyncInclude
	def mosync_include; "#{mosyncdir}/include" + sub_include; end
	def mosync_libdir; "#{mosyncdir}/lib"; end
	def sub_include; USE_NEWLIB ? "/newlib" : ""; end
	def use_stlport
		@EXTRA_INCLUDES << "#{mosync_include}/stlport" if(!MOSYNC_NATIVE)
		@LIBRARIES << "stlport" if(!MOSYNC_NATIVE)
	end
end

class PipeTask < MultiFileTask
	def initialize(work, name, objects, libs, linkflags, files = [])
		super(work, name, files)
		@FLAGS = linkflags
		dirTask = DirTask.new(work, File.dirname(name))
		@objects = objects + libs
		@prerequisites += @objects + [dirTask]

		initFlags
	end

	def needed?(log = true)
		return true if(super(log))
		return flagsNeeded?(log)
	end

	def cFlags
		return "#{@FLAGS} \"#{@NAME}\" \"#{@objects.join('" "')}\""
	end

	def execute
		execFlags
		# pipe-tool may output an empty file and then fail.
		begin
			sh "#{mosyncdir}/bin/pipe-tool#{cFlags}"
		rescue => e
			FileUtils.rm_f(@NAME)
			raise
		end
		if(!File.exist?(@NAME))
			error "Pipe-tool failed silently!"
		end
	end

	include FlagsChanged
end

# adds dependency handling
class PipeResourceTask < PipeTask
	def initialize(work, name, objects)
		@depFile = name + '.mf'
		@tempDepFile = "#{@depFile}t"
		super(work, name, objects, [], " -depend=#{@tempDepFile} -R")

		# only if the file is not already needed do we care about extra dependencies
		if(!needed?(false)) then
			@prerequisites += MakeDependLoader.load(@depFile, @NAME)
		end
	end
	def needed?(log = true)
		if(!File.exists?(@depFile))
			puts "Because the dependency file is missing:" if(log)
			return true
		end
		return super(log)
	end
	def execute
		super
		FileUtils.mv(@tempDepFile, @depFile)
	end
end

module PipeGccMod
	def gccmode; '-S'; end
	def mod_flags; ' -g'; end
	def builddir_prefix
		if(USE_NEWLIB)
			return 'newlib_'
		else
			return 'pipe_'
		end
	end
	def object_ending; '.s'; end
	def pipeTaskClass; PipeTask; end
end

class Mapip2LinkTask < NativeGccLinkTask
	def initialize(work, name, objects, libs, linkflags)
		super(work, name, objects, GCC_DRIVER_NAME)
		#@FLAGS = linkflags
		@FLAGS = ' -nodefaultlibs -nostartfiles -Wl,--warn-common,--emit-relocs,--no-check-sections'
		LD_EXTRA_DEPENDENCIES.each do |d|
			@prerequisites << FileTask.new(self, d)
		end
		@prerequisites += libs
		@FLAGS << ' -Wl,--start-group'
		libs.each do |l|
			@FLAGS << " #{l}"
		end
		@FLAGS << ' -Wl,--end-group'
	end
end

module Mapip2GccMod
	def gccmode; '-c'; end
	def mod_flags; ''; end
	def builddir_prefix
		if(USE_NEWLIB)
			return 'mapip2_newlib_'
		else
			return 'mapip2_'
		end
	end
	def object_ending; '.o'; end
	def pipeTaskClass; Mapip2LinkTask; end
end

class PipeGccWork < GccWork
	def isPipeWork; true; end
	def gccVersionClass; PipeGccWork; end
	include GccVersion

	if(USE_GNU_BINUTILS)
		include Mapip2GccMod
	else
		include PipeGccMod
	end

	def gcc
		return GCC_DRIVER_NAME
	end

	def target_flags;
		flags = ''
		flags << GCC_PIPE_EXTRA_FLAGS
		flags << mod_flags
		flags += ' -DUSE_NEWLIB' if(USE_NEWLIB)
		return flags
	end
	def target_cppflags
		return ''#' -frtti'
	end

	include MoSyncInclude

	def set_defaults
		default(:BUILDDIR_PREFIX, "")
		default(:COMMOM_BUILDDDIR_PREFIX, "")
		@BUILDDIR_PREFIX << builddir_prefix
		@COMMOM_BUILDDDIR_PREFIX << builddir_prefix
		super
	end

	private

	def setup3(all_objects, have_cppfiles)
		raise hell if(@gcc_version_info[:arm])
		#puts all_objects
		llo = @LOCAL_LIBS.collect { |ll| FileTask.new(self, @COMMON_BUILDDIR + ll + ".lib") }
		need(:@NAME)
		if(@TARGET_PATH == nil)
			need(:@BUILDDIR)
			need(:@TARGETDIR)
			@TARGET_PATH = @TARGETDIR + "/" + @BUILDDIR + "program"
			if(ELIM)
				@TARGET_PATH += "e"
			end
			@TARGET_PATH << '.elf' if(USE_GNU_BINUTILS)
		end
		@TARGET = pipeTaskClass.new(self, @TARGET_PATH, (all_objects + llo), libTasks, @FLAGS + @EXTRA_LINKFLAGS)
		@prerequisites << @TARGET
	end

	def makeGccTask(source, ending)
		task = super
		PIPE_EXTRA_DEPENDENCIES.each do |d|
			task.prerequisites << FileTask.new(self, d)
		end
		return task
	end
end
