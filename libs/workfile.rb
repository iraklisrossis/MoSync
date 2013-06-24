#!/usr/bin/ruby

require File.expand_path('../rules/task.rb')
require File.expand_path('../rules/targets.rb')

target :pipe do
	Work.invoke_subdirs(SUBDIRS, 'pipe')
end

target :native do
	Work.invoke_subdirs(SUBDIRS, 'native')
end

target :arm do
	Work.invoke_subdirs(SUBDIRS, 'arm')
end

target :default do
	Work.invoke_subdirs(SUBDIRS)
end

target :clean do
	Work.invoke_subdirs(SUBDIRS, 'clean')
end

Targets.setup

if(USE_NEWLIB)
	stdlibs = ["newlib", "stlport"]
else
	stdlibs = ["MAStd"]
end

SUBDIRS = stdlibs + ["MAUtil", "MTXml", "MAUI", "MAUI-revamp", "MATest", "MAP",
	"Testify", "MAFS", "yajl", "Ads", "Facebook", "NativeUI", "Notification", "Wormhole", "Purchase"]

if(HAVE_LIBC)
	SUBDIRS << "MoGraph"
end

if(MOSYNC_NATIVE)
	CopyDirWork.new(mosyncdir + "/include/glm", 'glm').invoke()
end

Targets.invoke
