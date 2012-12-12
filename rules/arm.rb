
module MoSyncArmGccMod
	def gcc; ARM_DRIVER_NAME; end
	def target_flags
		flags = super
		flags << ' -fno-exceptions -mfloat-abi=hard'
		flags << ' -DUSE_NEWLIB' if(USE_NEWLIB)
		return flags
	end
	def set_defaults
		default(:BUILDDIR_PREFIX, "")
		default(:COMMOM_BUILDDDIR_PREFIX, "")
		if(USE_NEWLIB)
			@BUILDDIR_PREFIX << "newlib_"
			@COMMOM_BUILDDDIR_PREFIX << "newlib_"
		end
		super
	end
end
