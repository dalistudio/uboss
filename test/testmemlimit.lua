local uboss = require "uboss"

local names = {"cluster", "dns", "mongo", "mysql", "redis", "sharedata", "socket", "sproto"}

-- set sandbox memory limit to 1M, must set here (at start, out of uboss.start)
uboss.memlimit(1 * 1024 * 1024)

uboss.start(function()
    local a = {}
    local limit
    local ok, err = pcall(function()
        for i=1, 12355 do
            limit = i
            table.insert(a, {})
        end
    end)
    local libs = {}
    for k,v in ipairs(names) do
        local ok, m = pcall(require, v)
        if ok then
            libs[v] = m
        end
    end
    uboss.error(limit, err)
    uboss.exit()
end)
