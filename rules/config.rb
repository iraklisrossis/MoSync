
require "#{File.dirname(__FILE__)}/util.rb"
require "#{File.dirname(__FILE__)}/mosync_util.rb"

# load local_config.rb, if it exists.
lc = "#{File.dirname(__FILE__)}/local_config.rb"
require lc if(File.exists?(lc))

default_const(:USE_ARM, false)
default_const(:USE_GCC_VERSION_IN_BUILDDIR_NAME, true)
default_const(:GCC_PIPE_EXTRA_FLAGS, ' -pipe')
default_const(:PRINT_FLAG_CHANGES, false)
default_const(:PRINT_GCC_VERSION_INFO, false)
default_const(:GCC_DRIVER_NAME, mosyncdir + '/mapip2/xgcc' + EXE_FILE_ENDING)
# array of strings, filenames.
default_const(:PIPE_EXTRA_DEPENDENCIES, [
	mosyncdir + '/libexec/gcc/mapip2/4.6.3/cc1' + EXE_FILE_ENDING,
	mosyncdir + '/libexec/gcc/mapip2/4.6.3/as' + EXE_FILE_ENDING,
])
default_const(:USE_GNU_BINUTILS, true)
default_const(:LD_EXTRA_DEPENDENCIES, [
	mosyncdir + '/libexec/gcc/mapip2/4.6.3/ld' + EXE_FILE_ENDING,
	mosyncdir + '/libexec/gcc/mapip2/4.6.3/ldscripts/elf32mapip2.x',
])
default_const(:EXIT_ON_ERROR, true)
default_const(:PRINT_WORKING_DIRECTORY, false)
default_const(:INSTALL_DIR, false)
