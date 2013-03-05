#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')
require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_resources.rb')

work = PipeExeWork.new
work.instance_eval do
	@SOURCES = [
		'.',
		'Controller',
		'Model',
		'View',
	]
	@PREREQUISITES = [BundleTask.new(self, './Resources/LocalFiles.bin', './LocalFiles')]
	@LSTFILES = ['Resources/Resources.lst']
	@LIBRARIES = ['mautil', 'Wormhole', 'mafs', 'nativeui', 'yajl', 'Notification']
	@EXTRA_LINKFLAGS = standardMemorySettings(11)
	@NAME = 'EuropeanCountries'
end

work.invoke
