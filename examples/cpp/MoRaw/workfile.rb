#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ['.']
	@EXTRA_CPPFLAGS = ' -Wno-strict-aliasing -Wno-missing-format-attribute'
	@LIBRARIES = ['mafs', 'mautil']
	@EXTRA_LINKFLAGS = standardMemorySettings(16)
	@NAME = 'moRaw'
end

work.invoke
