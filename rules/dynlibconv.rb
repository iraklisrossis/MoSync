
class DynLibConv
	def self.run(cur, new, fileList)
		change_name(fileList, cur, new)
	end

private

	##
	# Changes the dylib paths of the given files.
	#
	# files A list of existing files.
	# name A name to change.
	# new_name The new_name to replace it with.
	#
	def self.change_name(files, name, new_name)
		files.each do |file|
			paths = run_otool( file )
			change_lib_paths(file, paths, name, new_name)
		end
	end

	##
	# Gets the dylib information from the file and returns it.
	#
	def self.run_otool(file)
		clean_lines = Array.new
		cmd = "otool -L #{file}"
		puts cmd
		IO::popen(cmd) do |io|
			while !io.eof?
				line = io.gets( )
				stripped_line = line.strip
				path_part = line.split(" ")[0]
				clean_lines.push(path_part.strip)
			end
		end
		return clean_lines
	end

	###
	## Changes the lib path for the given paths, by replaces
	## name in the paths with new_name.
	##
	## file The binary or dylib to change
	## paths A set of dylib paths included in the
	## name String to replace in the path
	## new_name New string
	##
	def self.change_lib_paths(file, paths, name, new_name)
		paths.each do |path|
			if path.strip.include?(name.strip)
				puts "changing the paths for #{file} from #{name} to #{new_name}...\n"
				new_path = path.gsub(name.strip, new_name.strip)
				sh "install_name_tool -change #{path} #{new_path} #{file}"
			end
		end
	end
end
