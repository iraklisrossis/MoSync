#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ["."]
	@EXTRA_CFLAGS = " -Wno-unreachable-code"
	if(USE_NEWLIB)
		@EXTRA_LINKFLAGS = standardMemorySettings(8)
	end
	@NAME = "MAStx"
end

work.invoke
