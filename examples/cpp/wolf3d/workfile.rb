#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ['.']
	@EXTRA_CFLAGS = ' -Wno-missing-prototypes -Wno-missing-declarations -Wno-old-style-definition'+
		' -Wno-strict-prototypes -Wno-sign-compare -Wno-old-style-declaration -Wno-missing-field-initializers'+
		' -Wno-shadow -Wno-format'
	@LIBRARIES = ['mafs', 'mautil']
	@NAME = 'wolf3d'
end

work.invoke
