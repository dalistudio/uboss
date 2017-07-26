local uboss = require "uboss"


uboss.start(function()
	print("Main Server start")
	local console = uboss.newservice("testmysql")
	
	print("Main Server exit")
	uboss.exit()
end)
