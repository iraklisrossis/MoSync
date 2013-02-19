# Copyright (C) 2009 Mobile Sorcery AB
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to the Free
# Software Foundation, 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

def switchPathSlashes(cmd)
	raise "null path" if(!cmd)
	if(HOST == :win32)
		# bug in windows command line parser causes commands on the following format to fail:
		# \directory\subdir/executable
		# to work around, we substitute all \ for /.
		return cmd.gsub('\\', '/')
	end
	return cmd
end

# This should always be used instead of the ENV.
def mosyncdir
	m = ENV['MOSYNCDIR']
	raise "Environment variable MOSYNCDIR is not set" if(!m)
	return switchPathSlashes(m)
end

def getMosyncVersion
	open(mosyncdir + '/bin/version.dat') do |vdat|
		return vdat.lines.to_a[2].strip
	end
rescue Exception => e
	puts "Warning: #{e.message}"
	return "Unknown"
end

def getMosyncVersionDat
	open(mosyncdir + '/bin/version.dat') do |vdat|
		return vdat.lines.collect do |line|
			line.strip
		end
	end
rescue Exception => e
	puts "Warning: #{e.message}"
	return "Unknown"
end

require "#{File.dirname(__FILE__)}/host.rb"
require "#{File.dirname(__FILE__)}/error.rb"
