#!/usr/bin/ruby

USE_NEWLIB = true

require './settings.rb'
require './argv.rb'

EXIT_ON_ERROR = false if(!SETTINGS[:stop_on_fail])

target = nil
if(ARGV.size > 0 && ARGV[0].end_with?('.c'))
	target = ARGV[0]
	ARGV.delete_at(0)
end

require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_exe.rb')
require File.expand_path(ENV['MOSYNCDIR']+'/rules/mosync_lib.rb')
require 'fileutils'
require 'stringio'
require './skipped.rb'

if(target)
	puts "Target: #{target}"
	SETTINGS[:strict_prerequisites] = true
end

loader = PipeExeWork
loader.instance_eval do
	@EXTRA_SOURCEFILES = [
		'loader/loader.cpp',
	]
	@EXTRA_INCLUDES = ['.'] if(!USE_NEWLIB)
	@NAME = 'loader'
end

class Libdir
	def self.set(dir)
		#p dir
		@libdir = dir
	end

	def self.get()
		#p @libdir
		@libdir
	end
end

def input_files(filename)
	base = File.dirname(filename) + '/' + File.basename(filename, File.extname(filename))
	inputName = base + '.input'
	if(File.exists?(inputName))
		return [inputName]
	else
		return []
	end
end

def writeArgvFile(file, argv, testSrcName)
	argv = DEFAULT_ARGV if(!argv)
	file.write("const char* gArgv[] = {\"#{File.basename(testSrcName, File.extname(testSrcName))}\", ")
	argv.each do |arg|
		file.write("\"#{arg}\",")
	end
	file.write(" 0 };\n")
	file.write("const int gArgc = #{argv.size + 1};\n")
	inputs = input_files(testSrcName)
	file.write("\n")
	file.write("#include <stdio.h>\n") if(!inputs.empty?)
	#file.write("#include <stdlib.h>\n") if(!inputs.empty?)
	file.write("void setup_stdin(void) {\n")
	if(!inputs.empty?)
		file.write("\tstdin = fopen(\"#{File.basename(inputs[0])}\", \"r\");\n")
	end
	file.write("}\n")
end

def doArgv(baseName, argv, testSrcName)
	cName = "build/argv-#{baseName}.c"
	FileUtils.mkdir_p('build')
	file = open(cName, 'w')
	writeArgvFile(file, argv, testSrcName)
	file.close
	return cName
end

class ArgvTask < MemoryGeneratedFileTask
	def initialize(work, baseName, argv, testSrcName)
		cName = "build/argv-#{baseName}.c"
		io = StringIO.new
		writeArgvFile(io, argv, testSrcName)
		@buf = io.string
		super(work, cName)
	end
end

def dos2unixInPlaceCommand
	v = open('|dos2unix --version 2>&1').read.strip
	if(v.include?('version 0.'))
		# in-place conversion is default; must specify source and target formats.
		return 'dos2unix --d2u'
	else
		# source and target formats are default; must specify in-place conversion.
		return 'dos2unix -o'
	end
end

def clear_filesystem
	FileUtils.rm_rf('filesystem')
	FileUtils.mkdir_p('filesystem/tmp')
end
clear_filesystem

# treat filesystem/ as root. output a compiled resource file that setup_filesystem() can use
# to reproduce all the files.
def writeResourceFile(name)
	FileUtils.mkdir_p(File.dirname(name))
	resFileName = "#{name}.lst"
	resFile = open("#{resFileName}", 'w')
	resFile.puts('.res')
	resFile.puts('.label "start"')

	dir = Dir.new('filesystem/')
	doResourceDir(resFile, dir, '/', 0)

	resFile.puts('')
	resFile.puts('.res')
	resFile.puts('.label "end"')
	resFile.close
	return resFileName
end

def doResourceDir(resFile, dir, prefix, count)
	dir.each do |name|
		next if(name[0,1] == '.')
		realPath = dir.path+name
		resFile.puts('')
		resFile.puts(".res RES_FILE_#{count}")
		count += 1
		resFile.puts(".bin")
		if(File.directory?(realPath))
			runtimeName = prefix+name+'/'
			resFile.puts(".cstring \"#{runtimeName}\"")

			d2 = Dir.new(realPath+'/')
			count = doResourceDir(resFile, d2, runtimeName, count)
		else
			runtimeName = prefix+name
			resFile.puts(".cstring \"#{runtimeName}\"")
			resFile.puts(".include \"../#{realPath}\"") if(File.size(realPath) > 0)
		end
	end
	return count
end

DEFAULT_RESOURCES = writeResourceFile('build/default')

def getResFile(dataFiles, code, ofn, inputs)
	clear_filesystem
	has_files = false

	# copy files only when executing
	dataFiles.each do |file|
		has_files = true
		FileUtils.cp_r(file, 'filesystem/')
	end
	if(HOST == :win32)
		cmd = dos2unixInPlaceCommand
		inputs.each do |input|
			sh "#{cmd} \"filesystem/#{File.basename(input)}\""
		end
	end

	if(code)
		has_files = true
		code.call
	end

	if(has_files)
		resFile = writeResourceFile(ofn)
	else
		resFile = DEFAULT_RESOURCES
	end
	return resFile
end

mod = Module.new
mod.class_eval do
	def setup_pipe
		@EXTRA_SOURCEFILES = [
			'helpers.c',
			'setup_filesystem.c',
		]
		@EXTRA_SOURCETASKS = [
			ArgvTask.new(self, 'default', DEFAULT_ARGV, 'default'),
		]
		@EXTRA_INCLUDES = ['.'] if(!USE_NEWLIB)
		@EXTRA_CFLAGS = ' -Wno-missing-prototypes -Wno-missing-declarations'
		@EXTRA_CFLAGS << ' -Wno-declaration-after-statement'
		@EXTRA_CFLAGS << ' -Wno-inline'
		@NAME = 'helpers'
		Libdir.set(@COMMON_BUILDDIR)
	end
	def mosync_libdir; 'build'; end
	def copyHeaders; end
end
MoSyncLib.invoke(mod)

class CombTask < FileTask
	def initialize(work, target, sources)
		super(work, target)
		@prerequisites = sources
	end
	def execute
		open(@NAME, 'wb') do |f|
			@prerequisites.each do |p|
				f.write(open(p, 'rb').read)
			end
		end
	end
end

class TTWork < PipeExeWork
	def initialize(f, name)
		super()
		@BUILDDIR_PREFIX = 'rebuild_' if(defined?(MODE) && MODE == 'rebuild')
		@EXTRA_SOURCEFILES = [
			f,
		]

		@EXTRA_CFLAGS = " -I- -iquote sys -iquote . -std=gnu99 -iquote \"#{File.dirname(f)}\""

		@EXTRA_OBJECTS = [
			FileTask.new(self, Libdir.get()+'helpers.o'),
			FileTask.new(self, Libdir.get()+'setup_filesystem.o'),
		]

		bn = File.basename(f)
		argv = SPECIFIC_ARGV.fetch(bn, nil)
		inputs = input_files(f)
		@EXTRA_SOURCETASKS = []
		if(argv != nil || !inputs.empty?)
			@EXTRA_SOURCETASKS << ArgvTask.new(self, bn, argv, f)
		else
			@EXTRA_OBJECTS << FileTask.new(self, Libdir.get()+'argv-default.o')
		end

		dataFiles = SPECIFIC_FILES.fetch(bn, [])
		dataFiles += inputs

		code = SPECIFIC_CODE.fetch(bn, nil)

		lstFileName = getResFile(dataFiles, code, 'build/'+name, inputs)
		resFileName = 'build/'+name+'.res'
		resFileName = 'build/default.res' if(lstFileName == DEFAULT_RESOURCES)

		@resourceTask = PipeResourceTask.new(self, resFileName,
			[FileTask.new(self, lstFileName)])

		@SPECIFIC_CFLAGS = {
		}
		@EXTRA_LINKFLAGS = " -stacksize 512 -heapsize #{10*1024}"
		@EXTRA_EMUFLAGS = ' -noscreen -allowdivzero'
		@NAME = name
	end
	def define_cflags
		#puts 'define_cflags'
		include_dirs = @EXTRA_INCLUDES
		include_flags = include_dirs.collect {|dir| " -I \""+File.expand_path_fix(dir)+'"'}.join
		flags = ' -g'
		flags << ' -DNO_TRAMPOLINES -DUSE_EXOTIC_MATH -include skeleton.h'
		flags << ' -O2 -fomit-frame-pointer' if(CONFIG == "")
		flags << include_flags
		@CFLAGS = flags + @EXTRA_CFLAGS
		@CPPFLAGS = flags + @EXTRA_CPPFLAGS

		@TARGET_PATH = @BUILDDIR + @NAME.ext('.moo')
	end
	def builddir; @BUILDDIR; end
	def invoke
		super
		return if(!SETTINGS[:htdocs_dir])
		name = @NAME + '.comb'
		CombTask.new(self, SETTINGS[:htdocs_dir] + name, [@TARGET, @resourceTask]).invoke
	end
end


# Go through each directory, search for a Makefile and parse it to find "tests". Print the results.

MAKEFILE_TEST_ARRAYS = ['tests', 'tests-static', 'libm-tests', 'strop-tests']

def process_line(line)
	lineIsInteresting = false
	MAKEFILE_TEST_ARRAYS.each do |ta|
		if(line.beginsWith(ta))
			lineIsInteresting = true
		end
	end
	if(lineIsInteresting)
		#p line
		words = line.scan(/[^ \t]+/)

		#puts "test: #{words[0]}"
		return nil unless(MAKEFILE_TEST_ARRAYS.include?(words[0]))
		#p words
		if(words[1] == '=' || words[1] == ':=' || words[1] == '+=')
			result = words.slice(2..-1)
			if(words[0] == 'strop-tests')
				#p result
				result.collect! do |t| "test-#{t}" end
				#p result
			end
			return result
		end
	end
	return nil
end

SPACE_MARK = "__&NBSP;__"
def parse_makefile(filename)
	tests = []
	open(filename) do |mf|
		# Read the entire file.
		lines = mf.read

		# Get rid of comments, and combine logical lines into physical lines, for easy processing.
		lines.gsub!(/\\ /, SPACE_MARK)
		lines.gsub!(/#[^\n]*\n/m, "")
		lines.gsub!(/\\\n/, ' ')
		lines.split("\n").each do |line|
			new_tests = process_line(line)
			#p new_tests
			tests += new_tests if(new_tests)
		end
	end
	return tests
end

# find test programs
files = []
pattern = "#{SETTINGS[:source_path]}*/"
p pattern
dirs = Dir[pattern]
total = 0
files = {}
p dirs
dirs.each do |dir|
	dirName = File.basename(dir)
	if(SKIPPED_DIRECTORIES.include?(dirName))
		puts "Skipped #{dirName}"
		next
	end
	mfPath = dir + 'Makefile'
	if(File.exists?(mfPath))
		$stdout.write dirName + ': '
		tests = parse_makefile(mfPath)
		if(tests)
			tests.uniq!
			puts tests.size
			total += tests.size
			#p tests
			tests.each do |t|
				fn = t + '.c'
				files[dir + fn] = dirName
			end
		end
	else
		puts dirName + ' has no Makefile.'
	end
end

puts
puts "Total: #{total}"
puts "#{files.count} files to test."

# check for dupes
basenames = {}
overrides = []
files.each do |f, dir|
	bn = File.basename(f)
	if(basenames[bn])
		puts "Duplicate: #{f} - #{basenames[bn]}"
		overrides << f
		overrides << basenames[bn]
	else
		basenames[bn] = f
	end
end

# mark dupes
new_files = {}
files.each do |f, dir|
	bn = File.basename(f)
	if(overrides.include?(f))
		new_files[f] = dir + '_' + bn
		puts "de-dupe: #{new_files[f]}"
	else
		new_files[f] = bn
	end
end
files = new_files

builddir = nil
targetFound = false

def matchRegexp(tName)
	SKIPPED_REGEXP.each do |r|
		return true if(r.match(tName))
	end
	return false
end

# run the tests. keep counts.
unskippedCount = 0
wins = 0

LOADER_URLS_FILE = open(SETTINGS[:htdocs_dir] + 'libc_tests.urls', 'wb') if(SETTINGS[:htdocs_dir])

files.sort.each do |filename, targetName|
	bn = targetName
	next if(target && bn != target)
	if(!File.exists?(filename))
		puts "Nonexistant: #{bn}"
		next
	end
	skip = SKIPPED_FILES.include?(bn)
	SKIPPED_PATTERNS.each do |r|
		if(r.match(bn))
			skip = true
			break
		end
	end
	if(skip)
		puts "Skipped #{bn}"
		next
	end
	puts bn
	unskippedCount += 1

	if(!builddir)
		work = TTWork.new(filename, bn)
		work.setup
		builddir = work.builddir
	end

	ofn = builddir + bn.ext('.o')
	suffix = ''
	pfn = ofn.ext('.moo' + suffix)
	winFile = ofn.ext('.win' + suffix)
	failFile = ofn.ext('.fail' + suffix)
	logFile = ofn.ext('.log' + suffix)
	sldFile = ofn.ext('.sld' + suffix)
	force_rebuild = SETTINGS[:rebuild_failed] && File.exists?(failFile)

	LOADER_URLS_FILE.puts(SETTINGS[:loader_base_url] + bn.ext('.comb'))
	#LOADER_URLS_FILE.flush

	next if(!SETTINGS[:retry_failed] && File.exists?(failFile))

	if(SETTINGS[:strict_prerequisites])
		if(!work)
			work = TTWork.new(filename, bn)
		end
		work.invoke
	end

	if(force_rebuild)
		FileUtils.rm_f(ofn)
		FileUtils.rm_f(pfn)
		FileUtils.rm_f(winFile)
	end

	winTask = FileTask.new(work, winFile)
	#winTask.prerequisites << FileTask.new(work, ofn)
	if(!winTask.needed?(false))
		#puts "#{bn} won"
		wins += 1
		next
	end

	if(!work)
		work = TTWork.new(filename, bn)
	end

	begin
		FileUtils.mkdir_p(builddir)
		FileUtils.rm_f(winFile)
		FileUtils.touch(failFile)
		work.invoke
		work.run
		FileUtils.touch(winFile)
		FileUtils.rm_f(failFile)
		wins += 1
	rescue
		FileUtils.touch(failFile)
		FileUtils.rm_f(winFile)
		raise if(SETTINGS[:stop_on_fail])
	end
end

if(target && !targetFound)
	puts "Error: target not found!"
	exit(1)
end
