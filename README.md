# path_finding

### if you prefer to across conner diagonal grid then just make.

### if you prefer to avoid across conner diagonal grid:

    make CFLAG="-D__CONNER_SOLVE__"

avoid entry conner barriers

    make CFLAG="-D__CONNER_SOLVE__"

record path in dump

    make CFLAG="-D__RECORD_PATH__"

print detail search

    make CFLAG="-D__PRINT_DEBUG__"

## to do:

https://runzhiwang.github.io/2019/06/21/jps/

1. prune mid jump point
~~  2. mark_connected first to avoid unreachable point like astar.lua  ~~

### map dump

![](https://github.com/rangercyh/path_finding/blob/master/img/4.jpg)

### mark connected

![](https://github.com/rangercyh/path_finding/blob/master/img/3.jpg)

### 10000 times -D__RECORD_PATH__

![](https://github.com/rangercyh/path_finding/blob/master/img/1.jpg)

### 10000 times -D__RECORD_PATH__ -D__CONNER_SOLVE__

![](https://github.com/rangercyh/path_finding/blob/master/img/2.jpg)
