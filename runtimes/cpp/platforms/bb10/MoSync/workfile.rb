#!/usr/bin/ruby

require './config.rb'

require File.expand_path('../../../../../rules/bb10.rb')

BD = '../../../../..'

if(defined?(NATIVE_PROGRAM_DIR))
	npl = BB10LibWork.new
	npl.instance_eval do
		@SOURCES = [
			NATIVE_PROGRAM_DIR,
			"#{BD}/libs/ResCompiler",
		]
		@EXTRA_CPPFLAGS = " -Wno-shadow -Wno-vla"
		@EXTRA_INCLUDES = ["#{mosyncdir}/include"]
		@NAME = 'program'
		def run; end
	end
	npl.invoke
end

mosynclib = BB10LibWork.new
mosynclib.instance_eval do
	extraSources = ["#{BD}/runtimes/cpp/base/lib"]
	extraSourcefiles = []
	extraIncludes = []
	extraSpecificFlags = {
		'mosync.cpp' => ' -DNATIVE_PROGRAM=1',
	}

	@SOURCES = [
		'src',
		"#{BD}/runtimes/cpp/base",
		"#{BD}/intlibs/helpers/platforms/linux",
		"#{BD}/intlibs/hashmap",
		"#{BD}/intlibs/net",
	] + extraSources
	@IGNORED_FILES = [
		'MoSyncDB.cpp',
		'pim.cpp',
		'WaveAudioSource.cpp',
		'BufferAudioSource.cpp',
		'AudioSource.cpp',
		'AudioInterface.cpp',
		'AudioChannel.cpp',
	]
	@EXTRA_SOURCEFILES = [
		"#{BD}/runtimes/cpp/platforms/sdl/FileImpl.cpp",
		"#{BD}/runtimes/cpp/platforms/sdl/netImpl.cpp",
		"#{BD}/intlibs/filelist/filelist-linux.c",
	] + extraSourcefiles
	@EXTRA_INCLUDES += [
		"#{BD}/runtimes/cpp/base",
		"#{BD}/runtimes/cpp",
		"#{BD}/intlibs",
		'src',
		"#{BD}/runtimes/cpp/platforms/sdl",
	]
	@SPECIFIC_CFLAGS = {
		'SyscallImpl.cpp' => ' -DHAVE_IOCTL_ELLIPSIS -Wno-float-equal',
		'Syscall.cpp' => ' -Wno-float-equal',
		'Image.cpp' => ' -Wno-shadow',
	}.merge(extraSpecificFlags)
	@SPECIFIC_CFLAGS['SyscallImpl.cpp'] << ' -Wno-missing-noreturn'	# temp hack until all syscalls are implemented.

	@NAME = "mosynclib"
	def setup
		set_defaults
		@INSTALLDIR = "#{mosyncdir}/lib/#{@BUILDDIR_NAME}"
		super
	end
end
mosynclib.invoke

work = BB10ExeWork.new
work.instance_eval do
	if(defined?(NATIVE_PROGRAM_DIR))
		extraSources = ["#{BD}/runtimes/cpp/base/lib"]
		extraSourcefiles = []
		extraIncludes = []
		@EXTRA_OBJECTS = [npl.target]
		extraSpecificFlags = {
			'mosync.cpp' => ' -DNATIVE_PROGRAM=1',
		}
		@SPECIFIC_OBJNAMES = {
			'mosync.cpp' => 'mosync-native',
		}
	else
		extraSources = []
		extraSourcefiles = ["#{BD}/runtimes/cpp/core/Core.cpp"]
		extraIncludes = ["#{BD}/runtimes/cpp/core"]
		extraSpecificFlags = { 'mosync.cpp' => ' -DNATIVE_PROGRAM=0' }
		@SPECIFIC_OBJNAMES = {
			'mosync.cpp' => 'mosync-vm',
		}
	end
	@SOURCES = [
		'src',
		"#{BD}/runtimes/cpp/base",
		"#{BD}/intlibs/helpers/platforms/linux",
		"#{BD}/intlibs/hashmap",
		"#{BD}/intlibs/net",
	] + extraSources
	@IGNORED_FILES = [
		'MoSyncDB.cpp',
		'pim.cpp',
		'WaveAudioSource.cpp',
		'BufferAudioSource.cpp',
		'AudioSource.cpp',
		'AudioInterface.cpp',
		'AudioChannel.cpp',
	]
	@EXTRA_SOURCEFILES = [
		"#{BD}/runtimes/cpp/platforms/sdl/FileImpl.cpp",
		"#{BD}/runtimes/cpp/platforms/sdl/netImpl.cpp",
		"#{BD}/intlibs/filelist/filelist-linux.c",
	] + extraSourcefiles
	@EXTRA_INCLUDES += [
		"#{BD}/runtimes/cpp/base",
		"#{BD}/runtimes/cpp",
		"#{BD}/intlibs",
		'src',
		"#{BD}/runtimes/cpp/platforms/sdl",
	]
	@SPECIFIC_CFLAGS = {
		'Core.cpp' => ' -DHAVE_IOCTL_ELLIPSIS -Wno-float-equal',
		'SyscallImpl.cpp' => ' -DHAVE_IOCTL_ELLIPSIS -Wno-float-equal',
		'Syscall.cpp' => ' -Wno-float-equal',
		'Image.cpp' => ' -Wno-shadow',
	}.merge(extraSpecificFlags)
	@SPECIFIC_CFLAGS['SyscallImpl.cpp'] << ' -Wno-missing-noreturn'	# temp hack until all syscalls are implemented.
	@LIBRARIES = BB10_RUNTIME_LIBS

	@NAME = "MoSync"
	@BB10_SETTINGS = {
		:AUTHOR => 'a',
		:AUTHOR_ID => 'gYAAgEtfkcaouNHlVckFZUnMyKo',
		:APP_NAME => 'MoSync',
		:PACKAGE_NAME => 'com.example.MoSync',
		:ID => 'testDev_mple_MoSyncdc14348e',
		:VERSION => '1.0.0.1',
		:VERSION_ID => 'testMS4wLjAuMSAgICAgICAgICA',
	}
end

work.invoke
