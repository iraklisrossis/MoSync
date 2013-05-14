#!/usr/bin/ruby

require File.expand_path(ENV['MOSYNCDIR']+'/rules/task.rb')
require File.expand_path('./parse_example_list.rb')
SUBDIRS = parseExampleList

target :default do
	Work.invoke_subdirs(SUBDIRS)
end

target :clean do
	Work.invoke_subdirs(SUBDIRS, 'clean')
end

Targets.setup

# This should be generalized into HAS_LIBC and HAS_STL,
# for the benefit of native modes.
if(!USE_NEWLIB)
	newlibOnlyExamples = [
		'cpp/HelloSTL',
		'cpp/Graphun',
		'cpp/MoGraph/MoGraphWave',
		'cpp/MoGraph/MoGraphWave2',
		'cpp/MoGraph/MoGraphFinance',
	]
	puts "Skipping the following examples, because they require newlib:"
	p newlibOnlyExamples
	SUBDIRS.reject! do |dir|
		newlibOnlyExamples.include?(dir)
	end
end

Targets.invoke
