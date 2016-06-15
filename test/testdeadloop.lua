local uboss = require "uboss"
local function dead_loop()
    while true do
        uboss.sleep(0)
    end
end

uboss.start(function()
    uboss.fork(dead_loop)
end)
