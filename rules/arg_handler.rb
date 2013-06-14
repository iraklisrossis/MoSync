# Make sure to use this file before targets.rb,
# or it will be too late to call Targets.registerArgHandler().

class Targets
private
	@@handlers = {}
public
	def Targets.registerArgHandler(a, &block)
		raise "Need a block!" if(!block)
		a = a.to_s
		if(!@@handlers[a])
			@@handlers[a] = []
		end
		@@handlers[a] << block
	end
end
