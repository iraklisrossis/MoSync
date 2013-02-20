#!/usr/bin/ruby

require File.expand_path('../../rules/mosync_lib.rb')

mod = Module.new
mod.class_eval do
	def setup_pipe
		@SOURCES = ['.', 'src']
		@SOURCES << 'src/stdlib' if(!HAVE_LIBC)

		@HEADER_DIRS = ['.', 'src/api']
		@EXTRA_INCLUDES = ['src/stdlib'] if(!HAVE_LIBC)
		@EXTRA_CFLAGS = ' -Wno-float-equal -Wno-unreachable-code'

		@INSTALL_INCDIR = 'yajl'
		@NAME = 'yajl'
	end
end

MoSyncLib.invoke(mod)
