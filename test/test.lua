-- =========== test a* ===============
--[[
local astar = require "astar"

-- [=[
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
--[===[
m:add_blockset({
    {1,0},{1,19},{5,3},{6,3},{6,4},{6,5},{5,5},
    {9,9},{10,9},{11,9},{12,9},{13,9},
    {9,10},{9,11},{9,12},{9,13},{9,14},
    {14,9},{14,10},{14,11},{14,12},{14,13},{14,14},
    {9,14},{10,14},{11,14},{12,14},{13,14},
})
m:dump()
m:mark_connected()
m:dump_connected()
-- ]===]
m:dump()
-- m:clear_block()
-- m:dump()
-- m:add_block(1,1)
-- m:dump()
local total_time, n = 0, 100
for i = 1, n do
    local _, _, t = m:find_path()
    total_time = total_time + t
end
-- if find then
    m:dump()
    print('avg time:', (total_time / n) * 1000, 'ms')
-- else
    -- print('no path!')
-- end
-- ]=]
-- [[
local m1 = astar.new({
    {0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 1, 1, 0},
    {1, 0, 0, 0, 0},
}, true)
m1:set_start(4, 3)
m1:set_end(0, 0)
m1:dump()
local _, _, cost_time = m1:find_path()
m1:dump()
print('cost time:', cost_time * 1000, 'ms')
-- ]]
-- =========== test jps ===============
-- [====[
local jps = require "jps"
local t1 = os.clock()
local j = jps.new({     -- create 2D grid map
    w = 20,             -- width of map
    h = 20,             -- height of map
    obstacle = {        -- obstacle in map
        {1,1},{1,2},{1,3},{1,4},{1,5},{1,6}
    },
})
assert(j:is_block(1, 1))
assert(not j:is_block(0, 0))
j:set_start(0,10)
j:set_end(19,1)
--- j:clear_allblock()
--- j:dump()
j:add_block(1, 1)
for i = 3, 19 do
    j:add_block(4, i)
end
for i = 1, 18 do
    j:add_block(1, i)
end
for i = 0, 10 do
    j:add_block(18, i)
end
j:add_blockset({
    {1,0},{1,19},{5,3},{6,3},{6,4},{6,5},{5,5},
    {9,9},{10,9},{11,9},{12,9},{13,9},
    {9,10},{9,11},{9,12},{9,13},{9,14},
    {14,9},{14,10},{14,11},{14,12},{14,13},{14,14},
    {9,14},{10,14},{11,14},{12,14},{13,14},
})
j:clear_block(1,1)      -- clear one obstacle point
-- j:clear_allblock()      -- clear all obstacle
j:mark_connected()      -- mark map connected for speed up search path to unreachable point
j:dump_connected()      -- print connected mark of map
--[[
    search for path from start to end, return the jump points list in table
    if make with CFLAG="-D__CONNER_SOLVE__", it will avoid across conner diagonal grid
]]
j:dump()
for i = 1, 10000 do
    j:find_path()
end
j:dump()                -- print map, if make with CFLAG="-D__RECORD_PATH__", it will show the path result
print('cost time:', os.clock() - t1, 's')
-- ]====]
local j1 = jps.new({
    w = 10,
    h = 6,
    obstacle = {
        {2,1},{4,1},{5,1},{6,1},{6,2},{6,4},{3,5},{4,5}
    },
})
j1:set_start(0,5)
j1:set_end(3,0)
j1:mark_connected()
j1:find_path()
j1:dump()
local j2 = jps.new({
    w = 10,
    h = 10,
})
j2:set_start(0,0)
for i = 0, 8 do
    j2:add_block(i, 1)
end
for i = 1, 9 do
    j2:add_block(i, 3)
end
for i = 0, 8 do
    j2:add_block(i, 5)
end
for i = 1, 9 do
    j2:add_block(i, 7)
end
for i = 0, 8 do
    j2:add_block(i, 9)
end
j2:set_end(9,9)
j2:mark_connected()
j2:find_path()
j2:dump()
local j3 = jps.new({
    w = 10,
    h = 10,
})
j3:set_start(0,0)
for i = 0, 8 do
    j3:add_block(i, 1)
end
for i = 2, 8 do
    j3:add_block(8, i)
end
for i = 1, 7 do
    j3:add_block(i, 8)
end
for i = 3, 7 do
    j3:add_block(1, i)
end
for i = 2, 6 do
    j3:add_block(i, 3)
end
for i = 4, 6 do
    j3:add_block(6, i)
end
for i = 3, 5 do
    j3:add_block(i, 6)
end
j3:add_block(3, 5)

j3:set_end(5,5)
j3:add_block(3, 5)
local path = j3:find_path()
j3:dump()
j3:dump_connected()
j3:add_block(3, 5)
j3:dump_connected()
local path = j3:find_path()
j3:dump_connected()
for k, v in ipairs(path) do
    print(k, v[1], v[2])
end

local j4 = jps.new({ w = 20, h = 20 })
local l1, l2 = 5, 15
for i = 0, 19 do
    j4:add_block(i, l1)
    j4:add_block(i, l2)
    j4:add_block(l1, i)
    j4:add_block(l2, i)
end
j4:mark_connected()
j4:dump_connected()



local w = 20
local h = 20
local nav = jps.new ({w = w, h = h})

for x = 0, w - 1 do
    nav:add_block(x,  5)
    nav:add_block(x, 15)
end
for y = 0, h - 1 do
    nav:add_block(5, y)
    nav:add_block(15, y)
end

nav:clear_block(3,5)
nav:clear_block(3,15)
nav:clear_block(5,2)
nav:clear_block(5,7)
nav:clear_block(5,17)
nav:clear_block(10, 5)
nav:clear_block(10, 15)
nav:clear_block(15, 2)
nav:clear_block(15, 8)
nav:clear_block(15, 18)
nav:clear_block(18, 5)
nav:clear_block(18, 15)

nav:mark_connected()
nav:dump()
nav:dump_connected()

local t1 = os.clock()
nav:set_start(1, 1)
nav:set_end(18, 18)
for i = 1, 1 do
    local cpath = nav:find_path()
    print(#cpath)
    for k, v in ipairs(cpath) do
        print(k, table.unpack(v))
    end
end
nav:dump()
print('cost time:', os.clock() - t1, 's')


nav:set_start(1, 1)
nav:set_end(1, 1)
print(nav:find_path())
nav:dump()
nav:find_path(2)
nav:dump()
nav:add_block(3,5)
nav:add_block(5,2)
nav:mark_connected()
nav:dump_connected()
local t = nav:find_path()
print(#t)
for k, v in pairs(t) do
    print(k, table.unpack(v))
end

nav:set_start(1, 1)
nav:set_end(6, 1)
local t = nav:find_path()
print(t)
