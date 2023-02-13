# jps
luabinding for jps

see the Daniel Harabor's blog for detail: https://harablog.wordpress.com/2011/09/07/jump-point-search/

see the algorithm article for detail: http://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-aaai11.pdf

# how to use for lua
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
j:mark_connected()      -- mark map connected for speed up search path to unreachable point(now auto done by find_path)
j:dump_connected()      -- print connected mark of map
--[[
    find_path(path_type) : search for path from start to end, return the jump points list in table
    path_type: default 1
    OBS_CONNER_OK = 1 : the path can across conner diagonal grid
    OBS_CONNER_AVOID = 2 : avoid across conner diagonal grid
]]
local path = j:find_path()
j:dump()                -- print map, if make with DEBUG, it will show the path result
```

# use for c
```c
#include "jps.h"

void main() {
    int w = 20; int h = 20;
    struct map *m = jps_create(w, h);

    for (int i = 0; i < w; i++) {
        jps_set_obstacle(m, i, 5, 1);
        jps_set_obstacle(m, i, 15, 1);
    }
    for (int j = 0; j < h; j++) {
        jps_set_obstacle(m, 5, j, 1);
        jps_set_obstacle(m, 15, j, 1);
    }

    jps_set_obstacle(m, 5, 3, 0);
    jps_set_obstacle(m, 15, 3, 0);
    jps_set_obstacle(m, 2, 5, 0);
    jps_set_obstacle(m, 7, 5, 0);
    jps_set_obstacle(m, 17, 5, 0);
    jps_set_obstacle(m, 5, 10, 0);
    jps_set_obstacle(m, 15, 10, 0);
    jps_set_obstacle(m, 2, 15, 0);
    jps_set_obstacle(m, 8, 15, 0);
    jps_set_obstacle(m, 18, 15, 0);
    jps_set_obstacle(m, 5, 18, 0);
    jps_set_obstacle(m, 15, 18, 0);

    jps_mark_connected(m);
    jps_dump_connected(m);

    jps_set_start(m, 1, 1);
    jps_set_end(m, 18, 18);

    IntList *il = il_create(2);
    jps_path_finding(m, 1, il);
    jps_dump(m);

    jps_destory(m);
    il_destroy(il);
    free(m);
}
```

# complie
    make or make CFLAG="-DDEBUG" to dump the result path

    testc need linux asan to check memory
    yum install libasan

# to do

https://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-icaps14.pdf

https://runzhiwang.github.io/2019/06/21/jps/


~~1. mark_connected first to avoid unreachable point like astar.lua~~

2. use bit calc to speed up jump point finding

```c
__builtin_clz(((B->>1) && !B-) ||((B+>>1) && !B+))
```

~~3. mem use optimize~~

~~4. prune mid jump point~~

5. final path optimize


## map dump

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/7.jpg)

## mark connected

![](https://github.com/rangercyh/path_finding/blob/master/screenshots/4.jpg)
![](https://github.com/rangercyh/path_finding/blob/master/screenshots/3.jpg)
