#!/usr/bin/ruby

require './config.rb'

# set up BB10 NDK compiler

TARGET = 'bb10'

require File.expand_path('../../../../../rules/exe.rb')

# Override the GNU version; it seems impossible to determine qcc's version dynamically.
def get_gcc_version_info(gcc)
	v = {
		:string => '4.6.3',
		:ver => '4.6.3',
		:arm => false,
		:clang => false,
		:qcc => true,
	}
	return v
end

class CompileGccTask < FileTask
#	# override parameter order
#	def cFlags
#		return " #{@work.gccmode} \"#{File.expand_path_fix(@SOURCE)}\"#{@FLAGS}"
#	end

	def depFlags; " -Wp,-MM,-MF,\"#{@TEMPDEPFILE}\""; end

	alias original_execute :execute
	def execute
		original_execute
		text = ''
		open(@DEPFILE, 'r') do |file|
			text = file.read
		end
		open(@DEPFILE, 'w') do |file|
			file.write(File.dirname(@DEPFILE) + '/' + text)
		end
	end
end

class BB10ExeWork < ExeWork
	def getGccInvoke(ending)
		case ending
			when '.c'; return 'qcc -V4.6.3,gcc_ntoarmv7le'
			when '.cpp'; return 'qcc -V4.6.3,gcc_ntoarmv7le_cpp'
			else raise "Unsupported file type #{ending}"
		end
	end
	def initialize
		super
		q = ENV['QNX_TARGET']
		@EXTRA_INCLUDES = [
			"#{q}/usr/include/freetype2",
			"#{q}/../target-override/usr/include",
		]
	end
	def customTargetSetFlags
		@TARGET_FLAGS = ' -D_FORTIFY_SOURCE=2 -D__BB10__ -Wno-psabi'
		@TARGET_CPPFLAGS = ''
	end

	def setup3(all_objects, have_cppfiles)
		@EXTRA_LINKFLAGS += ' -lang-c++ -g -Wl,-z,relro -Wl,-z,now'
		@LIBRARIES.each { |l| @EXTRA_LINKFLAGS += ' -l' + l }
		need(:@NAME)
		need(:@BUILDDIR)
		need(:@TARGETDIR)
		target = @BUILDDIR + @NAME + '.elf'
		linker = have_cppfiles ? 'qcc -V4.6.3,gcc_ntoarmv7le' : 'qcc -V4.6.3,gcc_ntoarmv7le_cpp'
		@TARGET = link_task_class.new(self, target, all_objects, [], [], @EXTRA_LINKFLAGS, linker)
		@prerequisites += [@TARGET]
	end
end

work = BB10ExeWork.new
work.instance_eval do
	BD = '../../../../..'
	@SOURCES = [
		'src',
		"#{BD}/runtimes/cpp/base",
		"#{BD}/intlibs/helpers/platforms/linux",
		"#{BD}/intlibs/hashmap",
	]
	@IGNORED_FILES = [
		'ThreadPool.cpp',
		'MoSyncDB.cpp',
		'pim.cpp',
		'networking.cpp',
		'WaveAudioSource.cpp',
		'BufferAudioSource.cpp',
		'AudioSource.cpp',
		'AudioInterface.cpp',
		'AudioChannel.cpp',
	]
	@EXTRA_SOURCEFILES = [
		"#{BD}/runtimes/cpp/core/Core.cpp",
		"#{BD}/intlibs/filelist/filelist-linux.c",
	]
	@EXTRA_INCLUDES += [
		"#{BD}/runtimes/cpp/core",
		"#{BD}/runtimes/cpp/base",
		"#{BD}/runtimes/cpp",
		"#{BD}/intlibs",
		'src',
	]
	@SPECIFIC_CFLAGS = {
		'Core.cpp' => ' -DHAVE_IOCTL_ELLIPSIS -Wno-float-equal',
		'SyscallImpl.cpp' => ' -DHAVE_IOCTL_ELLIPSIS',
		'Syscall.cpp' => ' -Wno-float-equal',
		'Image.cpp' => ' -Wno-shadow',
	}
	@LIBRARIES = [
		'm',
	]

	@NAME = "MoSync"
end

work.invoke
