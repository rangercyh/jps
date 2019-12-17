-- =========== test a* ===============
local astar = require "astar"

local m = astar.new(20, 20, true)

m:set_start(0, 19)
m:set_end(19, 1)
m:add_block(1, 1)
for i = 3, 19 do
    m:add_block(4, i)
end
for i = 1, 18 do
    m:add_block(1, i)
end
m:add_blockset({
    {1,1},{1,2},{1,3},{1,4},{1,5},{1,6},{1,7},{1,8}
})
for i = 0, 10 do
    m:add_block(18, i)
end
m:dump()
-- m:clear_block()
-- m:dump()
-- m:add_block(1,1)
-- m:dump()
local t1, n = 0, 100
for i = 1, n do
    local _, _, t = m:find_path()
    t1 = t1 + t
end
-- if find then
    m:dump()
    print('avg time:', (t1 / n) * 1000, 'ms')
-- else
    -- print('no path!')
-- end

-- =========== test jps ===============
-- local jps = require "jps"
