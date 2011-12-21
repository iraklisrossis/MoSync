class WinCopy
def getfiles
	return @filesToCopy
end
def getFolders
    return @foldersToMake
end
def initialize(mosyncDir, sourceDir)
	@filesToCopy = {
        "#{@buildToolsDir}/build_package_tools/mosync_bin/*", "#{@mosyncDir}/bin",
        "#{@buildToolsDir}/build_package_tools/mapip_bin/*", "#{@mosyncDir}/mapip/bin",
        "#{@sourceDir}/skins", "#{@mosyncDir}/skins",
        "#{@sourceDir}/MoSyncRules.rules", "#{@mosyncDir}",
        "#{@sourceDir}/runtimes/java/platforms/android/AndroidProject/res/drawable/*", "#{@mosyncDir}/etc",
        "#{@sourceDir}/runtimes/java/platforms/android/mosync.keystore", "#{@mosyncDir}/etc",
        "#{@buildToolsDir}/build_package_tools/mapip_bin/*", "#{@mosyncDir}/libexec/gcc/mapip/3.4.6",
        "#{@buildToolsDir}/build_package_tools/mosync_bin/version.dat", "#{@mosyncDir}/bin",
        "#{@buildToolsDir}/build_package_tools/bin/Batik", "#{@mosyncDir}/bin",
        "#{@sourceDir}/rules","#{@mosyncDir}/",
        "#{@sourceDir}/runtimes/java/platforms/android/default.icon", "#{@mosyncDir}/etc"
	}
    @foldersToMake = {
        "#{@mosyncDir}/libexec/gcc/mapip/3.4.6"
    }
end
end
