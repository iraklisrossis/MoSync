#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

raise unless(USE_NEWLIB)

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ["."]
	@EXTRA_INCLUDES = ["#{mosync_include}/stlport", "#{mosync_include}/glm", "#{mosync_include}/MoGraph"]
	@EXTRA_CPPFLAGS = ' -Wno-float-equal -Wno-unreachable-code -Wno-shadow -Wno-missing-noreturn'
	@LIBRARIES = ["stlport", "mautil", "MoGraph", "yajl"]
	@EXTRA_LINKFLAGS = standardMemorySettings(11)
	@NAME = "MoGraphFinance"
end

work.invoke
