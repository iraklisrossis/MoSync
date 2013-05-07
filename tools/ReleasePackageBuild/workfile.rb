
require File.expand_path('../../rules/dynlibconv.rb')

# Create and populate default_etc.
DirTask.new(nil, mosyncdir+'/bin/default_etc').invoke
def jni(name, subDir = '')
	CopyFileTask.new(nil, "#{mosyncdir}/bin/default_etc/#{name}",
		FileTask.new(nil, "../../runtimes/java/platforms/androidJNI/#{subDir}#{name}")).invoke
end
jni('default.icon')
jni('mosync.keystore')
jni('icon.svg', 'AndroidProject/res/drawable/')

CopyDirWork.new("#{mosyncdir}/bin", 'Batik', "build_package_tools/bin/Batik").invoke

def cft(dst, src)
	CopyFileTask.new(nil, dst, FileTask.new(nil, src)).invoke
end

def copyIndependentFiles()
	filenames = [
		'maspec.fon',
		'default_maprofile.h',
		'unifont-5.1.20080907.ttf',
		'MoSyncOnlineDocs.URL',
		'javame/JadTool.jar',
	]
	DirTask.new(nil, mosyncdir+'/bin/javame').invoke
	filenames.each do |f|
		cft(mosyncdir+'/bin/'+f, 'build_package_tools/mosync_bin/'+f)
	end

	# from osx_bin
	[
		'pcab.pl',
		'pcab.conf.template',
	].each do |f|
		cft(mosyncdir+'/bin/'+f, 'build_package_tools/osx_bin/'+f)
	end
end

# Populate bin.
case(HOST)
when :win32
	CopyDirWork.new(mosyncdir, 'bin', "build_package_tools/mosync_bin").invoke
when :darwin
	CopyDirWork.new(mosyncdir, 'bin', "build_package_tools/osx_bin").invoke
	cft("#{mosyncdir}/bin/zip", '/usr/bin/zip')
	cft("#{mosyncdir}/bin/unzip", '/sw/bin/unzip')
	cft("#{mosyncdir}/bin/openssl", '/opt/local/bin/openssl')
	cft("#{mosyncdir}/bin/ImageMagick/convert", '/opt/local/bin/convert')

	DynLibConv.run("/sw/lib", "@loader_path", "#{mosyncdir}/bin/openssl")
	DynLibConv.run("/opt/local/lib", "@loader_path", "#{mosyncdir}/bin/openssl")

	copyIndependentFiles()
when :linux
	copyIndependentFiles()
else
	raise "Unsupported HOST: #{HOST}"
end

# Copy platforms.
CopyDirWork.new(mosyncdir, 'profiles/platforms', '../../platforms').invoke
