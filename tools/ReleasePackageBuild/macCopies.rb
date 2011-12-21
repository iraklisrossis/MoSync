class MacCopy
def getfiles
	return @filesToCopy
end
def initialize(mosyncDir, sourceDir)
	@filesToCopy = {
			"/usr/bin/zip", "#{mosyncDir}/bin",
			"/sw/bin/unzip", "#{mosyncDir}/bin",
			"/usr/bin/c++filt" , "#{mosyncDir}/bin",
			"/opt/local/bin/openssl", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/mosync_bin/openssl.cnf", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/mosync_bin/unifont-5.1.20080907.ttf", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/BMFont", "#{mosyncDir}/bin/BMFont",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/ImageMagick", "#{mosyncDir}/bin/ImageMagick",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/*.dll", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/updater*", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/lcab", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/pcab.pl", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/javame", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/iphoneos/iphonesim", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/pcab.conf.template", "#{mosyncDir}/bin",
			"#{sourceDir}/runtimes/java/platforms/android/default.icon", "#{mosyncDir}/etc",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/ImageMagick/*", "#{mosyncDir}/bin/ImageMagick",
			"#{sourceDir}/tools/idl2/output/asm_config.lst", "#{mosyncDir}/bin",
			"#{sourceDir}/build/release/e32hack", "#{mosyncDir}/bin",
			"#{sourceDir}/tools/ReleasePackageBuild/build_package_tools/osx_bin/android", "#{mosyncDir}/bin/android",
			"#{sourceDir}/runtimes/java/platforms/android/mosync.keystore", "#{mosyncDir}/etc",
			"#{sourceDir}/runtimes/java/platforms/android/AndroidProject/res/drawable/*", "#{mosyncDir}/etc",
			"#{@buildToolsDir}/build_package_tools/bin/Batik", "#{mosyncDir}/bin",
			"#{sourceDir}/rules", "#{mosyncDir}/",
	}
end
end
