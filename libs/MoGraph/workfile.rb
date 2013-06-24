#!/usr/bin/ruby

require File.expand_path('../../rules/mosync_lib.rb')

mod = Module.new
mod.class_eval do
	def setup_pipe
		@SOURCES = ['.']
		@HEADER_DIRS = ['.']
		@EXTRA_INCLUDES = ['..']
		@EXTRA_INCLUDES << mosync_include+'/stlport' if(!MOSYNC_NATIVE)
		@EXTRA_CPPFLAGS = ' -Wno-float-equal -Wno-unreachable-code -Wno-shadow -Wno-missing-noreturn'

		@INSTALL_INCDIR = 'MoGraph'
		@NAME = 'MoGraph'
	end
end

MoSyncLib.invoke(mod)
