# path_finding

if you prefer to across conner diagonal grid then just make.
if you prefer to avoid across conner diagonal grid:
make CFLAG="-D__CONNER_SOLVE__"

1. avoid entry conner barriers:
make CFLAG="-D__CONNER_SOLVE__"

2. record path in dump
make CFLAG="-D__RECORD_PATH__"

3. print detail search
make CFLAG="-D__PRINT_DEBUG__"

make CFLAG="-D__RECORD_PATH__ -D__PRINT_DEBUG__ -D__CONNER_SOLVE__"

to do:

https://runzhiwang.github.io/2019/06/21/jps/

1. prune mid jump point
2. mark_connected first to avoid unreachable point like astar.lua
