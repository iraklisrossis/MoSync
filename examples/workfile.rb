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
if(!HAVE_LIBC)
	libcOnlyExamples = [
		'cpp/HelloSTL',
		'cpp/Graphun',
		'cpp/MoGraph/MoGraphWave',
		'cpp/MoGraph/MoGraphWave2',
		'cpp/MoGraph/MoGraphFinance',
	]
	puts "Skipping the following examples, because they require libc:"
	p libcOnlyExamples
	SUBDIRS.reject! do |dir|
		libcOnlyExamples.include?(dir)
	end
end

Targets.invoke
