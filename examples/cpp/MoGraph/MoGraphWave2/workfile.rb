#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

raise unless(HAVE_LIBC)

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ["."]
	@EXTRA_INCLUDES = ["#{mosync_include}/glm", "#{mosync_include}/MoGraph"]
	@EXTRA_CPPFLAGS = ' -Wno-float-equal -Wno-unreachable-code -Wno-shadow -Wno-missing-noreturn'
	@LIBRARIES = ["mautil", "MoGraph"]
	use_stlport
	@EXTRA_LINKFLAGS = standardMemorySettings(11)
	@NAME = "MoGraphWave2"
end

work.invoke
