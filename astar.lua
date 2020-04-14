local mtype = math.type
local msqrt = math.sqrt
local mpow = math.pow
local mabs = math.abs
local min = math.min

local mt = {}
mt.__index = mt

local function in_range(self, x, y)
    return x >= 0 and x < self.w and y >= 0 and y < self.h
end

local function is_block(self, x, y)
    return self.block and self.block[x] and self.block[x][y]
end

local function is_same(x1, y1, x2, y2)
    return x1 == x2 and y1 == y2
end

local function dist(x1, y1, x2, y2, diagonal_walk)
    if diagonal_walk then
        -- return msqrt(mpow(x1 - x2, 2) + mpow(y1 - y2, 2))
        local dx = mabs(x1 - x2)
        local dy = mabs(y1 - y2)
        return 10*(dx + dy) + (14-20)*min(dx, dy)
    else
        return mabs(x1 - x2) + mabs(y1 - y2)
    end
end

local function h_calc(self, x, y)
    return dist(self._e[1], self._e[2], x, y, self.diagonal_walk)
end

local function g_calc(self, x1, y1, x2, y2)
    return dist(x1, y1, x2, y2, self.diagonal_walk)
end

local function get_set_node(set, x, y)
    for p, v in ipairs(set) do
        if v.x == x and v.y == y then
            return v
        end
    end
end

local function unwind_path(path, found_path, node)
    if found_path[node] then
        table.insert(path, 1, {found_path[node].x, found_path[node].y})
        return unwind_path(path, found_path, found_path[node])
    else
        return path
    end
end


-- x,y start from 0
function mt:set_start(x, y)
    assert(mtype(x) == 'integer' and mtype(y) == 'integer')
    assert(in_range(self, x, y), 'start not in range')
    self._s = { x, y }
end

function mt:set_end(x, y)
    assert(mtype(x) == 'integer' and mtype(y) == 'integer')
    assert(in_range(self, x, y), 'end not in range')
    self._e = { x, y }
end

function mt:add_block(x, y)
    assert(mtype(x) == 'integer' and mtype(y) == 'integer')
    assert(in_range(self, x, y), 'block not in range')
    if not(self.block[x]) then
        self.block[x] = {}
    end
    self.block[x][y] = true
end

function mt:add_blockset(block_set)
    for _, v in pairs(block_set) do
        self:add_block(v[1], v[2])
    end
end

function mt:clear_block()
    self.block = {}
end

function mt:find_path()
    local t1 = os.clock()
    self._print_path = nil
    assert(self._s, 'no start pos')
    assert(self._e, 'no end pos')
    local sx, sy = self._s[1], self._s[2]
    local ex, ey = self._e[1], self._e[2]
    assert(not(is_block(self, sx, sy)), 'start pos in block')
    assert(not(is_block(self, ex, ey)), 'end pos in block')
    if is_same(sx, sy, ex, ey) then
        return true, {}, os.clock() - t1
    end
    -- check connected
    local function insert_openset(openset, g_score, pf, new_node)
        local in_pos = #openset + 1
        for k = 1, #openset do
            if pf < g_score[openset[k]] + openset[k].h then
                in_pos = k
                break
            end
        end
        table.insert(openset, in_pos, new_node)
    end
    local function fresh_path(g_score, key_node, g, found_path, node)
        g_score[key_node] = g
        found_path[key_node] = node
    end
    local function get_neighbors(self, x, y)
        local neighbors = {}
        local check_neighbors = {
            { x, y - 1 },
            { x, y + 1 },
            { x - 1, y },
            { x + 1, y },
        }
        if self.diagonal_walk then
            check_neighbors[#check_neighbors + 1] = { x - 1, y - 1 }
            check_neighbors[#check_neighbors + 1] = { x + 1, y - 1 }
            check_neighbors[#check_neighbors + 1] = { x - 1, y + 1 }
            check_neighbors[#check_neighbors + 1] = { x + 1, y + 1 }
        end
        for _, v in pairs(check_neighbors) do
            if in_range(self, v[1], v[2]) and not(is_block(self, v[1], v[2])) then
                neighbors[#neighbors + 1] = { x = v[1], y = v[2] }
            end
        end
        return neighbors
    end
    local first_node = {
        x = sx,
        y = sy,
        h = h_calc(self, sx, sy)
    }
    local g_score = {
        [first_node] = 0
    }
    local openset, closeset, found_path = {first_node}, {}, {}
    while #openset > 0 do
        local node = table.remove(openset, 1)
        table.insert(closeset, node)
        if is_same(node.x, node.y, ex, ey) then
            local path = unwind_path({}, found_path, node)
            table.insert(path, {node.x, node.y})
            self._print_path = path
            return true, path, os.clock() - t1
        end
        local neighbors = get_neighbors(self, node.x, node.y)
        for _, v in pairs(neighbors) do
            if not(get_set_node(closeset, v.x, v.y)) then
                local pg = g_score[node] + g_calc(self, node.x, node.y, v.x, v.y)
                local open_node = get_set_node(openset, v.x, v.y)
                if open_node then
                    if pg < g_score[open_node] then
                        fresh_path(g_score, open_node, pg, found_path, node)
                    end
                else
                    local ph = h_calc(self, v.x, v.y)
                    local new_node = {
                        x = v.x,
                        y = v.y,
                        h = ph,
                    }
                    insert_openset(openset, g_score, pg + ph, new_node)
                    fresh_path(g_score, new_node, pg, found_path, node)
                end
            end
        end
    end
    return false
end

function mt:mark_connected()
    self._connect_map = {}
    local visited = {}
    local around = {}
    local connect_num = 0

    local mark_visited = function(visited, x, y)
        if not(visited[x]) then
            visited[x] = {}
        end
        visited[x][y] = true
    end
    local mark_connected = function(connected, x, y, connect_num)
        if not(connected[x]) then
            connected[x] = {}
        end
        connected[x][y] = connect_num
    end

    for i = 0, self.w - 1 do
        for j = 0, self.h - 1 do
            if not(visited[i] and visited[i][j]) then
                table.insert(around, {x = i, y = j})
                connect_num = connect_num + 1
                while #around > 0 do
                    local node = table.remove(around, 1)
                    local x, y = node.x, node.y
                    mark_visited(visited, x, y)
                    if not(is_block(self, x, y)) then
                        mark_connected(self._connect_map, x, y, connect_num)
                        local check_neighbors = {
                            { x, y - 1 },
                            { x, y + 1 },
                            { x - 1, y },
                            { x + 1, y },
                            { x - 1, y - 1 },
                            { x + 1, y - 1 },
                            { x - 1, y + 1 },
                            { x + 1, y + 1 },
                        }
                        for _, v in pairs(check_neighbors) do
                            if in_range(self, v[1], v[2]) then
                                if not(visited[v[1]] and visited[v[1]][v[2]]) then
                                    mark_visited(visited, v[1], v[2])
                                    if not(is_block(self, v[1], v[2])) then
                                        mark_connected(self._connect_map, x, y, connect_num)
                                        table.insert(around, {x = v[1], y = v[2]})
                                    end
                                end
                            end
                        end
                    end
                end
            end
        end
    end
end

function mt:dump_connected()
    print('S====================')
    for i = 0, self.h - 1 do
        local s = ""
        for j = 0, self.w - 1 do
            if self._connect_map[j] and self._connect_map[j][i] then
                s = s .. self._connect_map[j][i] .. ' '
            else
                s = s .. '* '
            end
        end
        print(string.sub(s, 1, -2))
    end
    print('====================E')
end

function mt:dump()
    print('S====================')
    local function _in_path(self, x, y)
        if not(self._print_path) then
            return
        end
        for _, v in pairs(self._print_path) do
            if v[1] == x and v[2] == y then
                return true
            end
        end
    end
    for i = 0, self.h - 1 do
        local s = ""
        for j = 0, self.w - 1 do
            local mark = false
            if self._s and is_same(self._s[1], self._s[2], j, i) then
                s = s .. 'S'
                mark = true
            end
            if self._e and is_same(self._e[1], self._e[2], j, i) then
                s = s .. 'E'
                mark = true
            end
            if is_block(self, j, i) then
                s = s .. '*'
                mark = true
            end
            if _in_path(self, j, i) then
                s = s .. '0'
                mark = true
            end
            if mark then
                s = s .. ' '
            else
                s = s .. '. '
            end
        end
        print(string.sub(s, 1, -2))
    end
    print('====================E')
end

local M = {}
function M.new(w, h, diagonal_walk)
    if type(w) == "table" then
        assert(#w > 0 and #w[1] > 0)
        local matrix = w
        local height = #matrix
        local width = #matrix[1]
        local rt = setmetatable({
            w = width,
            h = height,
            diagonal_walk = not not h,
            block = {},
        }, mt)
        for i = 1, height do
            for j = 1, width do
                if matrix[i][j] == 1 then
                    rt:add_block(j - 1, i - 1)
                end
            end
        end
        return rt
    else
        assert(mtype(w) == 'integer' and mtype(h) == 'integer' and w > 0 and h > 0)
        local astar = {
            w = w,
            h = h,
            diagonal_walk = diagonal_walk,
            block = {},
        }
        return setmetatable(astar, mt)
    end
end
return M
