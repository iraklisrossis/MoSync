#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = ['src', 'src/endian']
	@EXTRA_INCLUDES = ['inc', '.']
	@LSTFILES = ['resources.lst']
	@EXTRA_CPPFLAGS = ' -DBENCHMARK'
	@EXTRA_LINKFLAGS = ' -heapsize 386 -stacksize 16'
	@NAME = 'QuakeMDL'
	@BB10_SETTINGS = {
		:AUTHOR => 'a',
		:AUTHOR_ID => 'gYAAgEtfkcaouNHlVckFZUnMyKo',
		:APP_NAME => 'QuakeMDL',
		:PACKAGE_NAME => 'com.example.QuakeMDL',
		:ID => 'testDev_mple_QuakeMDL14348e',
		:VERSION => '1.0.0.1',
		:VERSION_ID => 'testMS4wLjAuMSAgICAgICAgICB',
	}
end

work.invoke
