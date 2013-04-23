#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ["."]
	@LIBRARIES = ["mautil"]
	@NAME = "ParallelDownloader"
	@PACK_PARAMETERS = ' --s60v3uid E3450F2C --s60v2uid 00297B7C'
end

work.invoke
