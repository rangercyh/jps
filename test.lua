local astar = require "astar"

local m = astar.new(10, 10)

m:set_start(0, 9)
m:set_end(9, 1)
m:add_block(1, 1)
for i = 3, 9 do
    m:add_block(4, i)
end
m:add_blockset({
    {1,1},{1,2},{1,3},{1,4},{1,5},{1,6},{1,7},{1,8}
})
for i = 0, 5 do
    m:add_block(8, i)
end
m:dump()
-- m:clear_block()
-- m:dump()
-- m:add_block(1,1)
-- m:dump()
local find
for i = 1, 1000 do
    find = m:find_path()
end
if find then
    m:dump()
else
    print('no path!')
end
