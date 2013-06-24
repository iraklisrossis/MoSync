#!/usr/bin/ruby

# File.expand_path is used here to ensure the files are really only loaded once.
require File.expand_path('rules/githooks.rb')
require File.expand_path('rules/targets.rb')
require File.expand_path('rules/host.rb')
require File.expand_path('rules/task.rb')
require File.expand_path('rules/mosync_util.rb')

enforceGithooks

if(HOST == :win32) then
	INTLIB_PLATFORM = "windows"
	PLATFORM_TOOLS = ["tools/makesis-2.0.0", "tools/makesis-4",
		"tools/MoSyncUpdater"]
elsif(HOST == :darwin)
	INTLIB_PLATFORM = "linux"
	PLATFORM_TOOLS = ["tools/makesis-2.0.0_unix", "tools/makesis-4_unix",
	]
else
	INTLIB_PLATFORM = HOST
	# todo: add lcab
	PLATFORM_TOOLS = [
		'tools/MoCab', 'tools/makesis-2.0.0_unix', 'tools/makesis-4_unix',
	]
end

PRE_DIRS = [
	"intlibs/idl-common",
	"intlibs/filelist",
	"intlibs/helpers/platforms/#{INTLIB_PLATFORM}",
]

MORE_DIRS = [
	"intlibs/bluetooth",
	"intlibs/demangle",
	"intlibs/dll",
	"intlibs/sqlite",
	"intlibs/gsm_amr",
	"intlibs/net",
	"intlibs/stabs",
	"intlibs/dgles-0.5",
	"intlibs/profiledb",
	"runtimes/cpp/platforms/sdl",
	"runtimes/cpp/platforms/sdl/mosynclib",
	"runtimes/cpp/platforms/sdl/MoRE"
]

BASE_DIRS = MORE_DIRS + PLATFORM_TOOLS

PIPE_DIRS = ['tools/elfStabSld',
	"tools/protobuild", "tools/pipe-tool", "tools/DefaultSkinGenerator"]
EXAM_DIRS = ["tests/unitTest", "examples"]
TOOL_DIRS = [
	'tools/ReleasePackageBuild',
	'intlibs/mustache',
	"tools/FontGenerator", "tools/PanicDoc", "tools/Bundle",
	"tests/unitTestServer", "tools/iphone-builder", "tools/icon-injector", "tools/e32hack",
	"tools/winphone-builder",
	"tools/mx-invoker",
	"tools/mx-config",
	"tools/profiledb", "tools/rescomp",
	"tools/mifconv", "tools/rcomp", "tools/package", "tools/uidcrc",
	"tools/nbuild",
]

MAIN_DIRS = BASE_DIRS + TOOL_DIRS + PIPE_DIRS
ALL_DIRS = MAIN_DIRS + EXAM_DIRS

LIB_DIRS = ['libs']

SKINS = CopyDirWork.new(mosyncdir, 'skins')
RULES = CopyDirWork.new(mosyncdir, 'rules')

class GenOpcodesTask < FileTask
	def initialize(mode, name)
		super(nil, name)
		@mode = mode
		@gen = 'runtimes/cpp/core/gen-opcodes.rb'
		@prerequisites << FileTask.new(nil, @gen)
		@prerequisites << DirTask.new(nil, File.dirname(@NAME))
	end
	def execute
		sh "ruby #{@gen} #{@mode} #{@NAME}"
	end
end

GEN_OPCODES = GenOpcodesTask.new('ccore', 'runtimes/cpp/core/gen-opcodes.h')
GEN_CS_OPCODES = GenOpcodesTask.new('cscore', 'runtimes/csharp/windowsphone/mosync/mosyncRuntime/Source/gen-core.cs')
GEN_JAVA_OPCODES = GenOpcodesTask.new('jcore', 'runtimes/java/Shared/generated/gen-opcodes.h')

class ExtensionIncludeWork < Work
	def setup
		extIncDir = mosyncdir + '/ext-include'
		@prerequisites = []
		@prerequisites << DirTask.new(self, extIncDir)
		sources = [
			'runtimes/cpp/core/extensionCommon.h',
			'runtimes/cpp/core/ext/invoke-extension.h',
			'runtimes/cpp/core/ext/extension.h',
			'runtimes/cpp/core/syscall_arguments.h',
			'runtimes/cpp/core/CoreCommon.h',
			'intlibs/helpers/cpp_defs.h',
			'intlibs/helpers/maapi_defs.h',
			]
		sources.each do |src|
			@prerequisites << CopyFileTask.new(self, "#{extIncDir}/#{File.basename(src)}",
				FileTask.new(self, src))
		end
	end
end

EXTENSION_INCLUDES = ExtensionIncludeWork.new

target :base => [SKINS, RULES] do
	SKINS.invoke
	RULES.invoke
	GEN_OPCODES.invoke
	GEN_CS_OPCODES.invoke
	GEN_JAVA_OPCODES.invoke
	Work.invoke_subdirs(PRE_DIRS)
	#Work.invoke_subdir("tools/WrapperGenerator", "compile")
	Work.invoke_subdir("tools/idl2", "compile")
	EXTENSION_INCLUDES.invoke
end

target :main => :base do
	Work.invoke_subdirs(MAIN_DIRS)
end

target :default => :main do
	Work.invoke_subdirs_ex(true, LIB_DIRS)
end

target :libs => :base do
	Work.invoke_subdir('tools/DefaultSkinGenerator')
	Work.invoke_subdirs_ex(true, LIB_DIRS)
end

target :examples => :default do
	Work.invoke_subdirs_ex(true, EXAM_DIRS)
end

target :all => :examples do
end

target :more => :base do
	Work.invoke_subdirs(MORE_DIRS)
end

target :version do
	rev = open('|git rev-parse --verify HEAD').read.strip
	mod = open('|git status --porcelain').read.strip
	mod = 'mod ' if(mod.length > 0)
	fn = "#{mosyncdir}/bin/version.dat"
	open(fn, 'w') do |file|
		file.puts("Developer local build")
		file.puts(Time.new.strftime('%Y%m%d-%H%M'))
		file.puts(mod+rev)
	end
	puts "Wrote #{fn}:"
	puts open(fn).read.strip
end

target :clean_more do
	verbose_rm_rf("build")
	Work.invoke_subdirs(PRE_DIRS, "clean")
	Work.invoke_subdir("tools/idl2", "clean")
	Work.invoke_subdirs(MORE_DIRS, "clean")
end

target :clean do
	verbose_rm_rf("build")
	Work.invoke_subdirs(PRE_DIRS, "clean")
	Work.invoke_subdir("tools/idl2", "clean")
	Work.invoke_subdirs(MAIN_DIRS, "clean")
end

target :clean_examples do
	Work.invoke_subdirs_ex(true, EXAM_DIRS, "clean")
end

target :clean_all => :clean do
	Work.invoke_subdirs_ex(true, LIB_DIRS, "clean")
	Work.invoke_subdirs_ex(true, EXAM_DIRS, "clean")
end

target :check_libs => :base do
	Work.invoke_subdirs(PIPE_DIRS)
	Work.invoke_subdir_ex(true, 'libs/MAStd') unless(USE_NEWLIB)
	Work.invoke_subdir_ex(true, 'libs/newlib') if(USE_NEWLIB)
	Work.invoke_subdir_ex(true, 'libs/MAUtil')
end

target :check => :check_libs do
	Work.invoke_subdirs(MORE_DIRS)
	Work.invoke_subdir_ex(true, 'testPrograms/gcc-torture')
end

# non-native only. don't modify; used by build system.
def all_configs(target)
	sh "ruby workfile.rb #{target}"
	sh "ruby workfile.rb #{target} CONFIG=debug"
	sh "ruby workfile.rb #{target} USE_NEWLIB="
	sh "ruby workfile.rb #{target} USE_NEWLIB= CONFIG=debug"
end

def bb10_configs(target)
	sh "ruby workfile.rb #{target} MODE=bb10 BB10_ARCH=arm"
	sh "ruby workfile.rb #{target} MODE=bb10 BB10_ARCH=x86"
	sh "ruby workfile.rb #{target} MODE=bb10 BB10_ARCH=arm CONFIG=debug"
	sh "ruby workfile.rb #{target} MODE=bb10 BB10_ARCH=x86 CONFIG=debug"
end

target :all_configs do
	all_configs('')
end

target :all_libs do
	all_configs('libs')
end

target :all_check do
	all_configs('check_libs')
	all_configs('check')
end

target :bb10_libs do
	bb10_configs('libs')
end

target :all_ex do
	all_configs('examples')
end

target :bb10_ex do
	bb10_configs('examples')
end

Targets.invoke
