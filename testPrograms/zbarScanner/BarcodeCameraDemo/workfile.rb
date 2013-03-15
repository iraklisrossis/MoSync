#!/usr/bin/ruby

USE_NEWLIB = true

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ['.']
	@EXTRA_INCLUDES = [
		"#{mosyncdir}/include/newlib/stlport",
		"#{mosyncdir}/include/newlib/zbar",
	]
	@LIBRARIES = ['mautil', 'nativeui', 'zbar']
	@EXTRA_LINKFLAGS = standardMemorySettings(11)
	@NAME = 'BarcodeCameraDemo'
end

work.invoke
