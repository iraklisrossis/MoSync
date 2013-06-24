#!/usr/bin/ruby

require File.expand_path('../../../../../rules/util.rb')

if(ARGV[0] == 'all')
	sh "ruby workfile.rb BB10_ARCH=arm"
	sh "ruby workfile.rb BB10_ARCH=x86"
	sh "ruby workfile.rb BB10_ARCH=arm CONFIG=debug"
	sh "ruby workfile.rb BB10_ARCH=x86 CONFIG=debug"
else
	require './config.rb'
end

require File.expand_path('../../../../../rules/bb10.rb')

BD = '../../../../..'

class MocTask < FileTask
	def initialize(work, src)
		super(work, "build/moc_#{File.basename(src)}.cpp")
		@prerequisites << FileTask.new(work, src)
		@src = src
	end
	def execute
		sh "#{ENV['QNX_HOST']}/usr/bin/moc -o#{@NAME} -i #{@src}"
	end
end

def doExampleFile(path)
	if(!File.exist?(path))
	  CopyFileTask.new(nil, path,
		  FileTask.new(nil, "#{path}.example")).invoke
	end
end

doExampleFile('src/config_platform.h')
doExampleFile("#{BD}/intlibs/bluetooth/config_bluetooth.h")

mosync_base = BB10LibWork.new
mosync_base.instance_eval do
	@SOURCES = [
		'src',
		"#{BD}/runtimes/cpp/base",
		"#{BD}/intlibs/helpers/platforms/linux",
		"#{BD}/intlibs/hashmap",
		"#{BD}/intlibs/net",
		"#{BD}/intlibs/bluetooth/bb10",
	]
	@IGNORED_FILES = [
		'MoSyncDB.cpp',
		'pim.cpp',
		'WaveAudioSource.cpp',
		'BufferAudioSource.cpp',
		'AudioSource.cpp',
		'AudioInterface.cpp',
		'AudioChannel.cpp',
		'mosync.cpp',
		'SyscallImpl.cpp',
	]
	@EXTRA_SOURCEFILES = [
		"#{BD}/runtimes/cpp/platforms/sdl/FileImpl.cpp",
		"#{BD}/runtimes/cpp/platforms/sdl/netImpl.cpp",
		"#{BD}/intlibs/filelist/filelist-linux.c",
	]
	@PREREQUISITES = [
		MocTask.new(self, 'src/slots.h'),
	]
	@EXTRA_INCLUDES += [
		"#{BD}/runtimes/cpp/base",
		"#{BD}/runtimes/cpp",
		"#{BD}/intlibs",
		'src',
		"#{BD}/runtimes/cpp/platforms/sdl",
	]
	@SPECIFIC_CFLAGS = {
		'Syscall.cpp' => ' -Wno-float-equal',
		'Image.cpp' => ' -Wno-shadow',
		'bluetooth.cpp' => ' -Wno-missing-noreturn',	# temp hack until all syscalls are implemented.
		'NativeUI.cpp' => " -Wno-missing-noreturn -I \"#{ENV['QNX_TARGET']}/usr/include/qt4\" -Wp,-std=gnu++0x",
		'nfc.cpp' => ' -Wp,-std=gnu++0x',
	}

	@NAME = 'mosync_base'
	def setup
		set_defaults
		@INSTALLDIR = "#{mosyncdir}/lib/#{@BUILDDIR_NAME}"
		super
	end
end
mosync_base.invoke

mosynclib = BB10LibWork.new
mosynclib.instance_eval do
	@SOURCES = [
		"#{BD}/runtimes/cpp/base/lib",
		'src/lib',
	]
	@EXTRA_INCLUDES += [
		"#{BD}/runtimes/cpp/base",
		"#{BD}/runtimes/cpp",
		"#{BD}/intlibs",
		'src',
		"#{BD}/runtimes/cpp/platforms/sdl",
	]
	@SPECIFIC_CFLAGS = {
		'SyscallImpl-lib.cpp' => ' -DHAVE_IOCTL_ELLIPSIS -Wno-float-equal',
	}
	@SPECIFIC_CFLAGS['SyscallImpl-lib.cpp'] << ' -Wno-missing-noreturn'	# temp hack until all syscalls are implemented.

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
	@SOURCES = [
		'src/vm',
	]
	@EXTRA_SOURCEFILES = [
		"#{BD}/runtimes/cpp/core/Core.cpp",
		"#{BD}/runtimes/cpp/core/mainLoop.cpp",
	]
	@EXTRA_INCLUDES += [
		"#{BD}/runtimes/cpp/core",
		"#{BD}/runtimes/cpp/base",
		"#{BD}/runtimes/cpp",
		"#{BD}/intlibs",
		'src',
		"#{BD}/runtimes/cpp/platforms/sdl",
	]
	@SPECIFIC_CFLAGS = {
		'Core.cpp' => ' -DHAVE_IOCTL_ELLIPSIS -Wno-float-equal',
		'SyscallImpl-vm.cpp' => ' -DHAVE_IOCTL_ELLIPSIS -Wno-float-equal',
	}
	@EXTRA_CPPFLAGS = ' -UMOSYNC_NATIVE'
	@SPECIFIC_CFLAGS['SyscallImpl-vm.cpp'] << ' -Wno-missing-noreturn'	# temp hack until all syscalls are implemented.

	@LOCAL_LIBS = [
		'mosync_base',
	]
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

target :default do
	work.invoke
end

target :all do
	# see hack at top of file.
end

Targets.invoke
