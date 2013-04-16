
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

def copyIndependentFiles()
	filenames = [
		'maspec.fon',
		'default_maprofile.h',
		'pcab.conf.template',
		'unifont-5.1.20080907.ttf',
		'MoSyncOnlineDocs.URL',
		'javame/JadTool.jar',
	]
	DirTask.new(nil, mosyncdir+'/bin/javame').invoke
	filenames.each do |f|
		CopyFileTask.new(nil, mosyncdir+'/bin/'+f, FileTask.new(nil, 'build_package_tools/mosync_bin/'+f)).invoke
	end
end

# Populate bin.
case(HOST)
when :win32
	src = 'mosync_bin'
when :darwin
	src = 'osx_bin'
when :linux
	copyIndependentFiles()
	exit(0)
else
	raise "Unsupported HOST: #{HOST}"
end

CopyDirWork.new(mosyncdir, 'bin', "build_package_tools/#{src}").invoke

CopyDirWork.new(mosyncdir, 'profiles/platforms', '../../platforms').invoke
