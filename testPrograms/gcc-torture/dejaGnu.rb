# support routines for handling dejaGnu tests

BAD_OPTIONS = [
	'-fprofile',
	'-fdump',
#	'-fexceptions',	# only bad if mode == :run
	'-ftrapv',
	'--dump=',
	'-da',
	'-fno-foobar',
	'-ftree-parallelize-loops',
	'-Wclobbered',
	'-mavx',
	'-msse2',
	'-std=iso9899:1990 -pedantic',
	'-std=iso9899:199409 -pedantic',
	'-fopenmp',
	'-frepo',
	'-fsyntax-only',
]

if((defined?(MODE) && MODE == 'rebuild') ||
	WP7)
	BAD_OPTIONS << '-finstrument-functions'
end

EFFECTIVE_TARGETS = [
	'c99_runtime',
	'cxa_atexit',
	'double64plus',
	'freorder',
	'hard_float',
	'int32plus',
	'large_double',
	'lto',
	'mempcpy',
	'named_sections',
	'nonpic',
	'ptr32plus',
	'section_anchors',
	'size32plus',
	'split_stack',
	'stdint_types',
	'vect_condition',
	'vect_double',
	'vect_float',
	'vect_floatint_cvt',
	'vect_int',
	'vect_int_mult',
	'vect_intfloat_cvt',
	'vect_long',
	'vect_long_long',
	'vect_shift',
	'vect_uintfloat_cvt',
	'wchar',
]

INEFFECTIVE_TARGETS = [
	'arm_eabi',
	'arm_neon_ok',
	'arm_thumb1_ok',
	'arm32',
	'dfp',
	'fpic',
	'fstack_protector',
	'ilp32',
	'lp64',
	'non_strict_align',
	'pthread',
	'pthread_h',
	'sse',
	'sse_runtime',
	'sse2_runtime',
	'sync_char_short',
	'sync_int_long',
	'tls',
	'tls_native',
	'tls_runtime',
	'tls_emulated',
	'trampolines',
]

ACCEPTABLE_TARGETS = [
	'c99_runtime',
	'correct_iso_cpp_string_wchar_protos',
	'gas',
	'init_priority',
	'int32plus',
	'inttypes_types',
	'lto',
	'native',
	'nonpic',
	'stdint_types',
	'unwrapped',
	'vect_cmdline_needed',
	'vect_int',
	'wchar',
	'*-*-*gnu*',
	'*-*-*',
	'!',
	'-fomit-frame-pointer',
]

UNACCEPTABLE_TARGETS = [
	'||',
	'&&',
	'-O0',
	'-mlongcall',
	'4byte_wchar_t',
	'avx_runtime',
	'arm_eabi',
	'default_packed',
	'dfp',
	'fixed_point',
	'fpic',
	'ilp32',
	'lp64',
	'int128',
	'pcc_bitfield_type_matters',
	'powerpc_hard_double',
	'powerpc_spe_nocache',
	'short_enums',
	'vmx_hw',
	'vxworks_kernel',
	#'No Inf support',
	#'No NaN support',
	#'No Inf/NaN support',
	#'Bug in _Q_dtoq',
	#'No scheduling',
	#'consts are shorts, not longs',
	#'SPU float rounds towards zero',
	'-m*nofpu*',
	'-m4al*',
	/-mcpu=m32/,
	'-mflip-mips16',
	'-mlp64',
	/-aix/,
	/-cygwin/,
	/-darwin/,
	/-hpux/,
	/-interix/,
	/-linux/,
	/-mingw/,
	/-netware/,
	/-osf5.*/,
	/-solaris/,
	/-vxworks/,
	/alpha.*-/,
	/arm.*-/,
	/avr.*-/,
	/cris.*-/,
	/fido.*-/,
	/h8300.*-/,
	/hppa.*-/,
	/i?86.*-/,
	/i686.*-/,
	/ia64.*-/,
	/m32c.*-/,
	/m68k.*-/,
	/mcore-/,
	/mips.*-/,
	'mips64',
	/mmix.*-/,
	/moxie-/,
	/-msx/,
	/-m2a/,
	/pdp11-/,
	/powerpc.*-/,
	'powerpc_altivec_ok',
	/rs6000.*-/,
	/rx-/,
	/s390.*-/,
	/sh-/,
	/sh2a\*-/,
	/sh\*-/,
	/sh4.*-/,
	/sh\[.*-/,
	/sparc.*-/,
	/spu-/,
	/vax-/,
	/x86_64.*-/,
]

class Object
	def is_an?(c)
		is_a?(c)
	end
end

module DejaGnu
# parse @sourcefile.
# set @mode.
# optionally, set @EXTRA_SOURCEFILES, @EXTRA_CFLAGS, @EXTRA_CPPFLAGS
def parseDejaGnu
	@mode = :compile
	open(@sourcepath) do |file|
		multilineComment = false
		@lineNum = 0
		file.each do |line|
			@lineNum += 1
			if(SKIP_LINES.include?(line.strip))
				@mode = :skip
				@skipReason = "skipped line: #{line}"
				return
			end
			# skip non-comment lines.
			cpp = line.index('//')
			c = line.index('/*')
			start = cpp
			if((cpp && c && c < cpp) || !cpp)
				start = c
			end
			#puts "#{c.inspect}, #{cpp.inspect}, #{start.inspect}"
			if(c)
				multilineComment = line if(!line.include?('*/'))
				#puts "mlc start #{@lineNum}" if(multilineComment)
			end
			next if(!start && !multilineComment)
			if(multilineComment)
				if(line.include?('*/'))
					line = multilineComment
					multilineComment = false
					#puts "mlc stop #{@lineNum}"
				else
					multilineComment << line
				end
				start = 0
			end
			# check to see if it's a dg directive.
			# assuming that any comment containing '{' must be dg-formatted.
			start = line.index('{', start)
			next if(!start)

			# but just in case it isn't...
			stop = line.index('}', start)
			next if(!stop)

			# tr-paste.c
			next if(multilineComment && !line.index('*/', stop))

			# not gonna work
			#@tokens = line.slice(start..-1).split(/[ {}]/)

			@line = line
			@pos = start
			a = tokenize()

			#p a
			# pr43002.c
			next if(a[0] == nil)
			next if(!a[0].start_with?('dg-'))

			# parse the results.
			case(a[0])
			when 'dg-do',
				'dg-lto-do'
				# mode
				@mode = a[1].to_sym
				@mode = :compile if(@mode == :assemble)
				# options
				a[2..-1].each do |o|
					if(o.is_an?(Array))
						case(o[0])
						when 'target'
							# scan all targets. one acceptable is enough.
							acceptable = isAcceptableTarget?(o[1..-1])
							if(!acceptable)
								@mode = :skip
								@skipReason = "dg-do unacceptable target: #{o[1..-1]}"
								return
							end
						when 'xfail'	# expected to fail?
							@mode = :skip
							@skipReason = "dg-do xfail"
							return
						else
							raise "Unknown option in array: #{o.inspect}"
						end
					else
						raise "Unknown option: #{o.inspect}"
					end
				end
			when 'dg-require-effective-target' then
				a[1..-1].each do |ret|
					if(!isEffectiveTarget?(ret))
						@mode = :skip
						@skipReason = "dg-require-effective-target #{ret}"
						return
					end
				end
			when 'dg-skip-if',
			'dg-xfail-run-if'
				reason = a[1]
				raise "Invalid skip-if" if(!reason.is_a?(String))
				a[2..-1].each do |ret|
					if(ret != '' && ret != '*' && ret != [''] && ret != ['*'] &&
						isAcceptableTarget?(ret))
						@mode = :skip
						@skipReason = "#{a[0]} #{reason.inspect}"
						return
					end
				end
			when 'dg-additional-sources'
				a[1..-1].each do |as|
					as.split(' ').each do |s|
						@EXTRA_SOURCEFILES << File.dirname(@sourcepath)+'/'+s
					end
				end
			when 'dg-options',
				'dg-lto-options',
				'dg-extra-ld-options'
				op = a[1]
				if(op.is_an?(Array))
					#raise "Invalid option" if(a[1].size != 1)
					aa = op
					if(aa.is_an?(Array) && aa.size == 1)
						aa = aa[0]
					end
					op = aa
					op = aa.join(' ') if(aa.is_an?(Array))
				end
				raise "Invalid option #{op.inspect} #{op.class.name}" if(!op.is_a?(String))
				# if we don't have a target, set both flags.
				# if target is c, set c flags.
				# if target is c++, set c++ flags.
				# if target is something else, set both flags if the target is acceptable
				options = ' '+op
				raise hell if(a[3])
				BAD_OPTIONS.each do |bo|
					if(options.include?(bo))
						@mode = :skip
						@skipReason = "bad option: #{bo}"
						return
					end
				end

				options.gsub!('${srcdir}', BASE)
				options.gsub!('$srcdir', BASE)
				options.gsub!("\\", '')

				if(a[0] == 'dg-extra-ld-options')
					@EXTRA_LINKFLAGS = options
					next
				end

				ta = a[2]
				if(ta)
					raise "error" if(!ta.is_an?(Array))
					raise hell if(ta[0] != 'target')
					pure = true
					c = false
					cpp = false
					acceptable = false
					ta[1..-1].each do |t|
						if(t == 'c')
							c = true
						elsif(t == 'c++')
							cpp = true
						else
							pure = false
							acceptable |= isAcceptableTarget?(t)
						end
					end
					if(pure)
						if(c)
							@EXTRA_CFLAGS = options
						elsif(cpp)
							@EXTRA_CPPFLAGS = options
						else
							raise hell
						end
					elsif(acceptable)
						@EXTRA_CFLAGS = options
						@EXTRA_CPPFLAGS = options
					end
				else # no target
					@EXTRA_CFLAGS = options
					@EXTRA_CPPFLAGS = options
				end
			when 'dg-final'
				aa = a[1]
				#p aa
				if(aa.is_an?(Array))
					if(aa[0].start_with?('scan-assembler'))
						@mode = :skip
						@skipReason = 'dg-message terminated'
						return
					end
				end
			when 'dg-message'
				case(a[1])
					when 'terminated',
						'undefined',
						'unimplemented'
					@mode = :skip
					@skipReason = "dg-message #{a[1]}"
					return
				end
			when 'dg-require-iconv'
				if(a[1] == 'IBM1047' && HOST == :win32)
					@mode = :skip
					@skipReason = 'dg-require-iconv IBM1047'
					return
				end
			when 'dg-bogus'
				if(a[3].is_an?(Array))
					if(a[3][0] == 'xfail')
						@mode = :skip
						@skipReason = 'dg-bogus xfail'
						return
					end
				end
			when 'dg-require-named-sections',
				'dg-require-ifunc',
				'dg-require-profiling',
				'dg-require-dll',
				'dg-xfail-if',
				'dg-require-linker-plugin',
				'dg-require-compat-dfp',
				'dg-error'
				#puts "Error detected"
				@mode = :skip
				@skipReason = a[0]
				return
			when 'dg-timeout-factor',
				'dg-prune-output',
				'dg-excess-errors',
				'dg-require-alias',
				'dg-require-ascii-locale',
				'dg-require-weak',
				'dg-require-visibility',
				'dg-add-options',
				'xxdg-warning',
				'dg-suppress-ld-options',
				'dg-final-use',
				'dg-require-weak-override',
				'dg-require-host-local',
				'dg-output',
				'dg-timeout',
				'dg-warning'
				# ignored
			else
				p line
				raise "Unknown directive: #{a[0].inspect}"
			end
		end
	end
end

def isEffectiveTarget?(ret)
	if(INEFFECTIVE_TARGETS.include?(ret))
		return false
	end
	if(EFFECTIVE_TARGETS.include?(ret))
		return true
	end
	raise "Unknown require-effective-target: #{ret.inspect}"
end

def isAcceptableTarget?(t)
	if(t.is_an?(Array))
		acceptable = false
		t.each do |tt|
			acceptable |= isAcceptableTarget?(tt)
		end
		return acceptable
	end
	if(ACCEPTABLE_TARGETS.include?(t))
		return true
	end
	UNACCEPTABLE_TARGETS.each do |u|
		return false if(u === t)
	end
	raise "Unknown target: #{t.inspect}"
end

def char
	@line[@pos,1]
end

def skipWhitespace
	while(char == ' ')
		@pos += 1
	end
end

# returns an array or a string, or nil.
# raises exceptions on error.
def tokenize()
	skipWhitespace
	if(char == '{')
		@pos += 1
		array = []
		while(1)
			skipWhitespace
			if(char == '}')
				break
			end
			array << tokenize()
		end
		@pos += 1
		return array
	end

	if(char == '"')
		@pos += 1
		#p @pos
		# empty string
		if(char == '"')
			@pos += 1
			return ''
		end
		si = @line.index(/[^\\]"/, @pos)
		if(si)
			n = si+2
		else
			si = @line.index('"', @pos) - 1
			n = si+2
		end
	else
		si = @line.index(/[ }]/, @pos) - 1
		n = si+1
	end
	s = @line.slice(@pos..si).gsub('\"', '"')
	@pos = n
	return s
end

end
