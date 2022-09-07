# jps
http://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-aaai11.pdf

## if you prefer to across conner diagonal grid then just make.

## if you prefer to avoid across conner diagonal grid:

    make CFLAG="-D__CONNER_SOLVE__"
    
## if you prefer to see all the detail and avoid across conner diagonal grid:

    make CFLAG="-D__CONNER_SOLVE__ -D__RECORD_PATH__ -D__PRINT_DEBUG__"

# CFLAG explain

avoid entry conner barriers

    make CFLAG="-D__CONNER_SOLVE__"

record path in dump

    make CFLAG="-D__RECORD_PATH__"

print detail search

    make CFLAG="-D__PRINT_DEBUG__"

# to do

https://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-icaps14.pdf

https://runzhiwang.github.io/2019/06/21/jps/


~~1. mark_connected first to avoid unreachable point like astar.lua~~

2. use bit calc to speed up jump point finding

```c
__builtin_clz(((B->>1) && !B-) ||((B+>>1) && !B+))
```

3. mem use optimize

~~4. prune mid jump point~~

5. final path optimize

# how to use

```lua
local jps = require "jps"
local j = jps.new({     -- create 2D grid map
    w = 20,             -- width of map
    h = 20,             -- height of map
    obstacle = {        -- obstacle in map
        {1,1},{1,2},{1,3},{1,4},{1,5},{1,6}
    },
})
j:set_start(0,0)        -- set start point
j:set_end(10,10)        -- set end point
j:add_block(1, 1)       -- set one obstacle point
j:add_blockset({        -- batch set obstacle points
    {1,0},{1,19},{5,3},{6,3},{6,4},{6,5},{5,5},
    {9,9},{10,9},{11,9},{12,9},{13,9},
    {9,10},{9,11},{9,12},{9,13},{9,14},
    {14,9},{14,10},{14,11},{14,12},{14,13},{14,14},
    {9,14},{10,14},{11,14},{12,14},{13,14},
})
j:clear_block(1,1)      -- clear one obstacle point
j:clear_allblock()      -- clear all obstacle
j:mark_connected()      -- mark map connected for speed up search path to unreachable point
j:dump_connected()      -- print connected mark of map
--[[
    search for path from start to end, return the jump points list in table
    if make with CFLAG="-D__CONNER_SOLVE__", it will avoid across conner diagonal grid
]]
local path = j:find_path()
j:dump()                -- print map, if make with CFLAG="-D__RECORD_PATH__", it will show the path result
```

## map dump

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/7.jpg)

## mark connected

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/4.jpg)
![](https://github.com/rangercyh/path_finding/blob/master/screenshots/3.jpg)

## 10000 times path finding

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/1.jpg)

## 10000 times path finding with -D__CONNER_SOLVE__

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/2.jpg)

# mid jump point prune almost doubled performace

## 10000 times path finding with mid jump point prune

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/5.jpg)

## 10000 times path finding  with mid jump point prune with -D__CONNER_SOLVE__

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/6.jpg)
