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

# This file defines the class used for compiling MoSync programs.

require "#{File.dirname(__FILE__)}/pipe.rb"
require "#{File.dirname(__FILE__)}/mosync_util.rb"
require "#{File.dirname(__FILE__)}/mosync_resources.rb"
require "#{File.dirname(__FILE__)}/targets.rb"
require "#{File.dirname(__FILE__)}/exe.rb"
require "#{File.dirname(__FILE__)}/arm.rb"

module PipeElimTask
	def execute
		execFlags
		# pipe-tool may output an empty file and then fail.
		begin
			sh "#{mosyncdir}/bin/pipe-tool -elim#{cFlags}"
			tarDir = @work.instance_variable_get(:@TARGETDIR) + "/" + @work.instance_variable_get(:@BUILDDIR)
			tarRebuild = tarDir + 'rebuild.se'
			tarSld = tarDir + 'slde.tab'
			FileUtils.mv('rebuild.s', tarRebuild)	# clean up cwd.
			FileUtils.rm_f(@NAME)	# make sure we know about any silent fail.
			sh "#{mosyncdir}/bin/pipe-tool#{@FLAGS} -sld=#{tarSld} #{@NAME} #{tarRebuild}"
		rescue => e
			FileUtils.rm_f('rebuild.s')
			FileUtils.rm_f(@NAME)
			raise
		end
		if(!File.exist?(@NAME))
			error "Pipe-tool failed silently!"
		end
	end
end

class PipeCppTask < PipeTask
	def initialize(work, name, objects, linkflags)
		@targetDir = File.dirname(name)
		super(work, name, objects, linkflags + ' -cpp', [@targetDir + '/rebuild.build.cpp', @targetDir + '/data_section.bin'])
	end
	def execute
		super
		FileUtils.mv('rebuild.build.cpp', @targetDir + '/rebuild.build.cpp')
		FileUtils.mv('data_section.bin', @targetDir + '/data_section.bin')
	end
end

# Generates C++ or C# code from an ELF file.
class Mapip2RebuildSourceTask < MultiFileTask
	def initialize(work, name, objects, libs, linkflags, mode)
		@elfTask = Mapip2LinkTask.new(work, name, objects, libs, linkflags)
		@name = name
		@mode = mode
		@dataSectionName = name + '.data_section.bin'
		super(work, name + '.rebuild.' + mode, [
			@dataSectionName,
		])
		@prerequisites << @elfTask
		@prerequisites << FileTask.new(work, "#{mosyncdir}/bin/elfStabSld#{EXE_FILE_ENDING}")
	end
	def dataSectionName; @dataSectionName; end
	def execute
		sh "#{mosyncdir}/bin/elfStabSld -#{@mode} #{@elfTask} rebuild.build.#{@mode}"
		FileUtils.mv("rebuild.build.#{@mode}", @NAME)
		FileUtils.mv('data_section.bin', @dataSectionName)
	end
end

class Mapip2CppTask < Mapip2RebuildSourceTask
	def initialize(work, name, objects, libs, linkflags)
		super(work, name, objects, libs, linkflags, 'cpp')
	end
end

class Mapip2CsTask < Mapip2RebuildSourceTask
	def initialize(work, name, objects, libs, linkflags)
		super(work, name, objects, libs, linkflags, 'cs')
	end
end

# Compiles the generated C++ file. Unused.
class RebuildCompileTask < FileTask
	def initialize(work, cppTask)
		targetDir = File.dirname(cppTask.to_s)
		name = targetDir + '/rebuild.build.o'
		super(work, name)
		@cppTask = cppTask
		@prerequisites << cppTask
	end
	def execute
		sh 'gcc -O2 -fomit-frame-pointer'+
			" -c #{@cppTask} -I #{mosyncdir}/include-rebuild -o #{@NAME}"+
			' -fno-exceptions -fno-rtti'+
			' -Wall -Wextra -Werror'+
			' -Wno-unused-label -Wno-unused-but-set-variable -Wno-return-type -Wno-unused-function'+
			#' -Wno-error=uninitialized'+
			' -Wno-maybe-uninitialized'+
			' -Wno-unused-parameter'+
			''
	end
end

# Links the generated C++ file with MoRE/Reload.
# Supports run.
class Mapip2RebuildTask < Task
	def initialize(work, name, objects, libs, linkflags)
		super(work)
		@cppTask = Mapip2CppTask.new(work, name, objects, libs, linkflags)
		@prerequisites << @cppTask
	end
	def execute
		#puts @work.class.name
		if(@work.respond_to?(:shouldRun) && @work.shouldRun)
		#puts "shouldRun: #{@work.shouldRun}"
		#if(@work.shouldRun)
			args = ["run", 'EXTRA_RUNPARAMS=-noscreen']
		else
			args = Targets.goals.collect do |g|
				g.to_s
			end
		end
		if(@work.respond_to?(:rebuildArgs))
			args += @work.rebuildArgs
		end
		args << "REBUILD_CPP=\"#{File.expand_path(@cppTask)}\""
		args << "DATA_SECTION=\"#{File.expand_path(@cppTask.dataSectionName)}\""
		args << "RESOURCE=\"#{File.expand_path(@work.resourceTask)}\"" if(@work.resourceTask)
		puts "Mapip2RebuildTask: invoke_subdir_ex"
		Work.invoke_subdir_ex(true, MOSYNC_SOURCEDIR + '/runtimes/cpp/platforms/sdl/Rebuild',
			*args)
		puts "Mapip2RebuildTask complete"
	end
end

# Packs a MoSync program for installation.
# resource can be nil. all other parameters must be valid.
class MoSyncPackTask < Task
	def initialize(work, options = {})
		super(work)
		@o = options
		@o[:packpath] = @o[:buildpath] + @o[:model] if(!@o[:packpath])
		@prerequisites = [@o[:program], DirTask.new(work, @o[:packpath])]
		@prerequisites << @o[:resource] if(@o[:resource])
		@o[:vendor] = 'Built with MoSync' if(!@o[:vendor])
	end
	def execute
		if(@o[:resource])
			r = File.expand_path(@o[:resource])
			resArg = " -r \"#{r}\""
		end
		p = File.expand_path(@o[:program])
		d = File.expand_path(@o[:packpath])
		co = File.expand_path(@o[:cppOutput])
		FileUtils.cd(@o[:tempdir], :verbose => true) do
			sh "#{mosyncdir}/bin/package -p \"#{p}\"#{resArg} -m \"#{@o[:model]}\""+
				" -d \"#{d}\" -n \"#{@o[:name]}\" --vendor \"#{@o[:vendor]}\""+
				" --version #{@o[:version]}"+
				" --ios-cert \"#{@o[:iosCert]}\""+
				" --cpp-output \"#{co}\" --ios-project-only"+
				" --android-package \"#{@o[:androidPackage]}\""+
				" --android-version-code \"#{@o[:androidVersionCode]}\""+
				" --android-keystore \"#{@o[:androidKeystore]}\""+
				" --android-storepass \"#{@o[:androidStorepass]}\""+
				" --android-alias \"#{@o[:androidAlias]}\""+
				" --android-keypass \"#{@o[:androidKeypass]}\""+
				" --show-passwords"+
				@o[:extraParameters].to_s
		end
	end
end

class MxConfigTask < MultiFileTask
	def dllName(e)
		"#{@extDir}/ext_#{e[1]}.dll"
	end

	def initialize(work, extDir, extensions)
		@extensions = extensions
		@extDir = extDir
		mxNames = extensions.collect do |e|
			"build/mx_#{e[1]}.h"
		end
		super(work, 'build/mxConfig.txt', mxNames)
		@mxConfig = mosyncdir + '/bin/mx-config'
		@prerequisites << DirTask.new(work, 'build')
		@prerequisites << FileTask.new(work, @mxConfig + EXE_FILE_ENDING)
		@extensions.each do |e|
			@prerequisites << FileTask.new(work, e[0])
			@prerequisites << FileTask.new(work, dllName(e))
		end
	end
	def execute
		params = ''
		@extensions.each do |e|
			params += " #{e[0]} #{dllName(e)}"
		end
		sh "#{@mxConfig} -o build#{params}"
	end
end

module MoSyncMemorySettings
	# Returns pipe-tool flags for memory settings,
	# with a datasize of 2^pow2kb KiB.
	# For example, for 2 MiB, call standardMemorySettings(11).
	# A power-of-2 argument may seem strange, but because the runtimes
	# force datasize to the closest upper power-of-2 anyway,
	# it should minimize accidental memory waste.
	def standardMemorySettings(pow2kb)
		raise "Insufficient memory. Need at least 64 KiB." if(pow2kb < 6)
		d = (1 << (pow2kb))
		h = d - (d >> 2)
		s = (d >> 4)
		return " -heapsize #{h} -stacksize #{s}"
	end
end

class Mapip2MxTask < MultiFileTask
	def initialize(work, prereq, mxFlags)
		@progName = prereq.to_s
		@mxFlags = mxFlags
		name = @progName.ext('')
		@sldName = name + '.sld'
		super(work, name, [@sldName])
		@prerequisites << prereq
	end
	def execute
		sh "#{mosyncdir}/bin/elfStabSld -mx #{@NAME}#{@mxFlags} #{@progName} #{@sldName}"
	end
end

module MoSyncExeModule
	include MoSyncMemorySettings
	def set_defaults
		default(:TARGETDIR, '.')
		super
	end
	def setup
		set_defaults
		@buildpath = @TARGETDIR + "/" + @BUILDDIR
		@SLD = @buildpath + "sld.tab"
		stabs = @buildpath + "stabs.tab"
		@FLAGS = " \"-sld=#{@SLD}\" \"-stabs=#{stabs}\" -B"
		@EXTRA_INCLUDES = @EXTRA_INCLUDES.to_a +
			[mosync_include, "#{mosyncdir}/profiles/vendors/MoSync/Emulator"]
		@prerequisites << MxConfigTask.new(self, "#{@COMMON_BASEDIR}/build/#{CONFIG}", @EXTENSIONS) if(@EXTENSIONS)
		super
	end
	def isPackingForIOS
		return (defined?(PACK) && @PACK_MODEL.beginsWith('Apple/'))
	end
	def resourceTask
		@resourceTask
	end
	def pipeTaskClass
		if(defined?(MODE))
			raise hell if(defined?(PACK))
			return Mapip2CppTask if(MODE == 'cpp')
			return Mapip2CsTask if(MODE == 'cs')
			return Mapip2RebuildTask if(MODE == 'rebuild')
			raise hell
		end
		return (isPackingForIOS ? Mapip2CppTask : super)
	end
	def libTask(lib)
		return FileTask.new(self, "#{mosync_libdir}/#{@COMMON_BUILDDIR_NAME}/#{lib}#{libFileEnding}")
	end
	def libTasks
		return (@LIBRARIES + @DEFAULT_LIBS).collect do |lib|
			libTask(lib)
		end
	end
	def setup3(all_objects, have_cppfiles)
		# resource compilation
		if(!@LSTFILES)
			if(@SOURCES[0])
				@LSTFILES = Dir[@SOURCES[0] + "/*.lst"]
			else
				@LSTFILES = []
			end
		end

		# rescomp support
		if(@LSTX)
			lstxTask = RescompTask.new(self, @BUILDDIR_BASE, @LSTX, @RES_PLATFORM)
			@resourceTask = PipeResourceTask.new(self, 'build/resources', [lstxTask])
		end

		if(@resourceTask)
			@prerequisites << @resourceTask
		elsif(@LSTFILES.size > 0)
			lstTasks = @LSTFILES.collect do |name| FileTask.new(self, name) end
			@resourceTask = PipeResourceTask.new(self, "build/resources", lstTasks)
			@prerequisites << @resourceTask
		end
		if(USE_NEWLIB)
			default(:DEFAULT_LIBS, ['newlib'])
		else
			default(:DEFAULT_LIBS, ['mastd'])
		end

		if(defined?(PACK))
			default(:PACK_MODEL, PACK)
			default(:PACK_VERSION, '1.0')
			default(:PACK_IOS_CERT, 'iPhone developer')
			default(:PACK_CPP_OUTPUT, @buildpath)
			default(:PACK_ANDROID_PACKAGE, "com.mosync.app_#{@NAME}")
			default(:PACK_ANDROID_VERSION_CODE, 1)
			default(:PACK_ANDROID_KEYSTORE, mosyncdir+'/etc/mosync.keystore')
			default(:PACK_ANDROID_STOREPASS, 'default')
			default(:PACK_ANDROID_ALIAS, 'mosync.keystore')
			default(:PACK_ANDROID_KEYPASS, 'default')
		end

		pipeFlags = @FLAGS + @EXTRA_LINKFLAGS
		if(!pipeFlags.include?(' -heapsize') && USE_NEWLIB && !USE_ARM)
			@EXTRA_LINKFLAGS << standardMemorySettings(10)
		end

		super

		if(USE_GNU_BINUTILS)
			if(!defined?(MODE) && !isPackingForIOS)
				@TARGET = Mapip2MxTask.new(self, @TARGET, @EXTRA_LINKFLAGS)
				@prerequisites << @TARGET
			end
		end

		if(ELIM)
			@TARGET.extend(PipeElimTask)
		end
		if(defined?(PACK))
			@prerequisites << @TARGET = MoSyncPackTask.new(self,
				:tempdir => @BUILDDIR_BASE,
				:buildpath => @buildpath,
				:model => @PACK_MODEL,
				:program => @TARGET,
				:resource => @resourceTask,
				:name => @NAME,
				:vendor => @VENDOR,
				:version => @PACK_VERSION,
				:iosCert => @PACK_IOS_CERT,
				:cppOutput => @PACK_CPP_OUTPUT,
				:androidPackage => @PACK_ANDROID_PACKAGE,
				:androidVersionCode => @PACK_ANDROID_VERSION_CODE,
				:androidKeystore => @PACK_ANDROID_KEYSTORE,
				:androidStorepass => @PACK_ANDROID_STOREPASS,
				:androidAlias => @PACK_ANDROID_ALIAS,
				:androidKeypass => @PACK_ANDROID_KEYPASS,
				:extraParameters => @PACK_PARAMETERS
				)
		end

		if(INSTALL_DIR)
			@prerequisites << CopyFileTask.new(self, "#{INSTALL_DIR}/#{File.basename(@TARGET.to_s)}", @TARGET)
			if(!defined?(PACK) && @resourceTask)
				@prerequisites << CopyFileTask.new(self, "#{INSTALL_DIR}/#{File.basename(@resourceTask.to_s)}", @resourceTask)
			end
		end
	end

	def emuCommandLine
		if(@resourceTask)
			resArg = " -resource \"#{@resourceTask}\""
		end
		if(@EXTENSIONS)
			extArg = " -x build/mxConfig.txt"
		end
		if(USE_GNU_BINUTILS)
			sldArg = " -sld \"#{@TARGET}.sld\""
		else
			sldArg = " -sld \"#{@SLD}\""
		end
		return "#{mosyncdir}/bin/MoRE -program \"#{@TARGET}\"#{sldArg}#{resArg}#{extArg}#{@EXTRA_EMUFLAGS}"
	end
	def run
		if(!defined?(MODE))
			# run the emulator
			sh emuCommandLine
		end
	end
	def gdb
		# debug the emulator
		sh "gdb --args #{emuCommandLine}"
	end
	def invoke
		super
		# If you invoke a work without setting up any targets,
		# we will check for the "gdb" goal here.
		if(Targets.size == 0)
			Targets.setup
			if(Targets.goals.include?(:gdb))
				self.gdb
				return
			end
		end
	end
end

class OriginalPipeExeWork < PipeGccWork
	include MoSyncExeModule
	def libFileEnding; '.lib'; end
end

class MoSyncArmExeWork < ExeWork
	include MoSyncExeModule
	include MoSyncInclude
	include MoSyncArmGccMod
	def libFileEnding; NATIVE_LIB_FILE_ENDING; end
	def linkerName(have_cppfiles); ARM_DRIVER_NAME; end
	def applyLibraries; end
	def libTasks
		# reorder libraries according to dependencies,
		# to avoid "undefined reference" errors from GNU ld.
		libDeps = {
			'mtxml' => 'mautil',
			'maui' => 'mautil',
			'matest' => 'mautil',
			'map' => 'mautil',
			'testify' => 'mautil',
			'stlport' => 'newlib',
			'ads' => 'mautil',
			'Facebook' => 'mautil',
			'nativeui' => 'mautil',
			'Notification' => 'mautil',
			'Wormhole' => 'nativeui',
		}
		libs = @LIBRARIES.sort do |a,b|
			num = 1
			num = -1 if(libDeps[a] == b)
			num = 0 if(a == b)
			num
		end

		return (libs + @DEFAULT_LIBS).collect do |lib|
			libTask(lib)
		end
	end
	def setup3(a, b)
		@EXTRA_LINKFLAGS << " -nodefaultlibs -B#{ARM_BASEDIR}/arm-elf/sys-include/gcc -lgcc"
		super(a, b)
	end
end

if(USE_ARM)
	PipeExeWorkBase = MoSyncArmExeWork
else
	PipeExeWorkBase = OriginalPipeExeWork
end

class PipeExeWork < PipeExeWorkBase
	include MoSyncMemorySettings
end
