
require "#{File.dirname(__FILE__)}/exe.rb"
require "#{File.dirname(__FILE__)}/native_lib.rb"
require "#{File.dirname(__FILE__)}/targets.rb"
require 'zip/zip'
require 'erb'
require 'digest'
require 'base64'

# Override the GNU version; it seems impossible to determine qcc's version dynamically.
def bb10_get_gcc_version_info()
	v = {
		:string => "4.6.3_#{BB10_COMPILER}",
		:ver => '4.6.3',
		:arm => false,
		:clang => false,
		:qcc => true,
	}
	return v
end

class BarDescriptorTask < MemoryGeneratedFileTask
	def initialize(work, name, s)
		super(work, 'build/bar-descriptor.xml')
		parts = s[:VERSION].split('.')
		versionMain = "#{parts[0]}.#{parts[1]}.#{parts[2]}"
		versionBuild = parts[3]
		@buf = ERB.new(open("#{File.dirname(__FILE__)}/bb10/bar-descriptor.xml.erb").read).result(binding)
	end
end

# zip-file-name, FileTask.
Asset = Struct.new(:target, :source)

class BarTask < FileTask
	def initialize(work, name, elf, otherAssets, settings)
		super(work, name)
		@elf = elf
		@others = otherAssets
		@settings = settings
		@prerequisites << @elf
		@others.each do |a|
			@prerequisites << a.source
		end
		@templateFile = "#{File.dirname(__FILE__)}/bb10/MANIFEST.MF.erb"
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
		@mosyncVersionDat = getMosyncVersionDat
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
		puts "Wrote #{@NAME}."
	end
end

module BB10Mod
	def getGccInvoke(ending)
		return "qcc -V4.6.3,#{BB10_COMPILER}"
	end
	def initialize
		super
		@TARGET_PLATFORM = :bb10
		q = ENV['QNX_TARGET']
		@EXTRA_INCLUDES = [
			"#{q}/usr/include/freetype2",
			"#{q}/../target-override/usr/include",
		]

		ENV['QNX_TARGET'] = 'C:/Programs/bbndk/target_10_0_9_1101/qnx6'
		ENV['QNX_HOST'] = 'C:/Programs/bbndk/host_10_0_9_284/win32/x86'
		ENV['QNX_CONFIGURATION'] = 'C:/Users/Fredrik/AppData/Local/Research In Motion/BlackBerry Native SDK'
		ENV['MAKEFLAGS'] = " -I#{ENV['QNX_TARGET']}/usr/include"
		ENV['PATH'] = "#{ENV['QNX_HOST']}/usr/bin;#{ENV['QNX_CONFIGURATION']}/bin;C:/Programs/bbndk/ide/win32/x86/eclipse/jre/bin;#{ENV['PATH']}"
		ENV['PYTHONPATH'] = ''
		ENV['QDE'] = 'C:/Programs/bbndk/ide/win32/x86'
	end
	def customTargetSetFlags
		@TARGET_FLAGS = ' -D_FORTIFY_SOURCE=2 -D__BB10__ -Wno-psabi'
		@TARGET_CPPFLAGS = ''
	end
	def gcc
		return :bb10
	end
end

class BB10LibWork < NativeLibWork
	include BB10Mod
	def run; end
end

BB10_RUNTIME_LIBS = [
	'm',
	'screen',
	'bps',
	'img',
	'freetype',
	'socket',
	'ssl',
	'crypto',
	'mmrndclient',
	'strm',
	'EGL',
	'GLESv1_CM',
	'GLESv2',
	'btapi',
]

class BB10ExeWork < ExeWork
	include BB10Mod

	def setup3(all_objects, have_cppfiles)
		need(:@BB10_SETTINGS)
		@BB10_SETTINGS.need(:AUTHOR, :AUTHOR_ID, :APP_NAME, :PACKAGE_NAME, :ID, :VERSION, :VERSION_ID)

		@EXTRA_LINKFLAGS += ' -lang-c++ -g -Wl,-z,relro -Wl,-z,now'
		@LIBRARIES.each { |l| @EXTRA_LINKFLAGS += ' -l' + l }
		@LOCAL_LIBS.each { |ll| all_objects << FileTask.new(self, @COMMON_BUILDDIR + ll + ".a") }
		need(:@NAME)
		need(:@BUILDDIR)
		need(:@TARGETDIR)
		target = @BUILDDIR + @NAME
		linker = "qcc -V4.6.3,#{BB10_COMPILER}"
		@TARGET = link_task_class.new(self, target, all_objects, [], [], @EXTRA_LINKFLAGS, linker)

		# packaging
		assets = [
			Asset.new('native/bar-descriptor.xml', BarDescriptorTask.new(self, @NAME, @BB10_SETTINGS)),
			Asset.new('native/icon.png', FileTask.new(self, "#{File.dirname(__FILE__)}/bb10/icon.png")),
		]
		Dir['assets/*'].each do |file|
			assets << Asset.new("native/#{File.basename(file)}", FileTask.new(self, file))
		end
		assets += @BB10_ASSETS if(@BB10_ASSETS)
		@barFile = BarTask.new(self, @TARGET.to_s + '.bar', @TARGET, assets, @BB10_SETTINGS)
		@prerequisites << @barFile
	end
	def run
		s = @BB10_SETTINGS
		#sh "blackberry-deploy -terminateApp #{CONFIG_RUN_DEVICE} -package-fullname #{s[:PACKAGE_NAME]}.#{s[:ID]}"
		sh "blackberry-deploy -installApp -launchApp #{CONFIG_RUN_DEVICE} #{@barFile}"
		#sh "blackberry-deploy -listInstalledApps #{CONFIG_RUN_DEVICE}"
		#sh "blackberry-deploy -printExitCode #{CONFIG_RUN_DEVICE} -package-fullname #{s[:PACKAGE_NAME]}.#{s[:ID]}"

		# wait until app exits. poll every second.
		count = 0
		puts 'Waiting for app to exit'
		loop do
			cmd = "|blackberry-deploy -isAppRunning #{CONFIG_RUN_DEVICE} -package-fullname #{s[:PACKAGE_NAME]}.#{s[:ID]}"
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
		sh "blackberry-deploy -getFile logs/log log.txt #{CONFIG_RUN_DEVICE} -package-fullname #{s[:PACKAGE_NAME]}.#{s[:ID]}"
		#sh "blackberry-deploy -getFile data/maSound.mp3 maSound.mp3 #{CONFIG_RUN_DEVICE} -package-fullname #{s[:PACKAGE_NAME]}.#{s[:ID]}"
		#sh "blackberry-deploy -getFile logs/MoSync.core core.bin #{CONFIG_RUN_DEVICE} -package-fullname #{s[:PACKAGE_NAME]}.#{s[:ID]}"
	end
end
