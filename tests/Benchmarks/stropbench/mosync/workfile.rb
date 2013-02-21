#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ['.']
	@LIBRARIES = ['mautil']
	unless(defined?(MODE) && MODE == 'bb10')
		@LIBRARIES << 'stlport'
		@EXTRA_INCLUDES = ["#{mosyncdir}/include/newlib/stlport"]
	end
	@EXTRA_LINKFLAGS = standardMemorySettings(14)
	@NAME = 'StropBench'
end

work.invoke
