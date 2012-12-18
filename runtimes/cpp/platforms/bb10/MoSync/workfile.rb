#!/usr/bin/ruby

require './config.rb'

# set up BB10 NDK compiler

TARGET = 'bb10'

require File.expand_path('../../../../../rules/exe.rb')
require 'zip/zip'
require 'erb'
require 'digest'
require 'base64'

# Override the GNU version; it seems impossible to determine qcc's version dynamically.
def get_gcc_version_info(gcc)
	v = {
		:string => "4.6.3_#{CONFIG_COMPILER}",
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

# zip-file-name, FileTask.
Asset = Struct.new(:target, :source)

class BarTask < FileTask
	def initialize(work, name, elf, otherAssets)
		super(work, name)
		@elf = elf
		@others = otherAssets
		@prerequisites << @elf
		@others.each do |a|
			@prerequisites << a.source
		end
		@templateFile = 'MANIFEST.MF.erb'
		@prerequisites << FileTask.new(work, @templateFile)
		@prerequisites << FileTask.new(work, 'workfile.rb')
	end

	# Return string containing manifest lines for all assets.
	def assets
		s = ''
		@others.each do |a|
			s << "Archive-Asset-Name: #{a.target}\n"
			s << "Archive-Asset-SHA-512-Digest: #{sha512digest(a.source)}\n"
			s << "\n"
		end
		s << "Archive-Asset-Name: native/#{File.basename(@elf)}\n"
		s << "Archive-Asset-SHA-512-Digest: #{sha512digest(@elf)}\n"
		s << "Archive-Asset-Type: Qnx/Elf\n"
		return s
	end
	def sha512digest(filename)
		sha = Digest::SHA512.new
		return Base64.urlsafe_encode64(sha.digest(open(filename, 'rb').read)).gsub('==','')
	end
	def execute
		open('build/MANIFEST.MF', 'wb') do |file|
			file.write(ERB.new(open(@templateFile).read).result(binding))
		end
		FileUtils.rm_f(@NAME)
		zip = Zip::ZipFile.new(@NAME, true)
		zip.add('META-INF/MANIFEST.MF', 'build/MANIFEST.MF')
		zip.add("native/#{File.basename(@elf)}", @elf)
		@others.each do |a|
			zip.add(a.target, a.source)
		end
		zip.close
	end
end

class BB10ExeWork < ExeWork
	def getGccInvoke(ending)
		return "qcc -V4.6.3,#{CONFIG_COMPILER}"
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
		target = @BUILDDIR + @NAME
		linker = "qcc -V4.6.3,#{CONFIG_COMPILER}"
		@TARGET = link_task_class.new(self, target, all_objects, [], [], @EXTRA_LINKFLAGS, linker)

		# packaging
		assets = [
			Asset.new('native/bar-descriptor.xml', FileTask.new(self, 'bar-descriptor.xml')),
			Asset.new('native/icon.png', FileTask.new(self, 'icon.png')),
		]
		Dir['assets/*'].each do |file|
			assets << Asset.new("native/#{File.basename(file)}", FileTask.new(self, file))
		end
		@barFile = BarTask.new(self, @TARGET.to_s + '.bar', @TARGET, assets)
		@prerequisites << @barFile
	end
	def run
		sh "blackberry-deploy -installApp -launchApp #{CONFIG_RUN_DEVICE} #{@barFile}"
		#sh "blackberry-deploy -listInstalledApps #{CONFIG_RUN_DEVICE}"
		#sh "blackberry-deploy -printExitCode #{CONFIG_RUN_DEVICE} -package-fullname com.example.MoSync.testDev_mple_MoSyncdc14348e"

		# wait until app exits. poll every second.
		count = 0
		puts 'Waiting for app to exit'
		loop do
			cmd = "|blackberry-deploy -isAppRunning #{CONFIG_RUN_DEVICE} -package-fullname com.example.MoSync.testDev_mple_MoSyncdc14348e"
			isRunning = nil
			open(cmd).each do |line|
				isRunning = false if(line.strip == 'result::false')
				isRunning = true if(line.strip == 'result::true')
			end
			raise "error" if(isRunning == nil)
			count += 1
			break if(!isRunning)
			sleep(1)
		end
		puts "App exited after #{count} iterations."
		sh "blackberry-deploy -getFile logs/log log.txt #{CONFIG_RUN_DEVICE} -package-fullname com.example.MoSync.testDev_mple_MoSyncdc14348e"
		#sh "blackberry-deploy -getFile logs/MoSync.core core.bin #{CONFIG_RUN_DEVICE} -package-fullname com.example.MoSync.testDev_mple_MoSyncdc14348e"
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
		"#{BD}/runtimes/cpp/platforms/sdl/FileImpl.cpp",
		"#{BD}/intlibs/filelist/filelist-linux.c",
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
		'SyscallImpl.cpp' => ' -DHAVE_IOCTL_ELLIPSIS',
		'Syscall.cpp' => ' -Wno-float-equal',
		'Image.cpp' => ' -Wno-shadow',
	}
	@LIBRARIES = [
		'm',
		'screen',
		'bps',
	]

	@NAME = "MoSync"
end

work.invoke
