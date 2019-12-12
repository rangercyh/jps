local mtype = math.type
local msqrt = math.sqrt
local mpow = math.pow

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

local function dist(x1, y1, x2, y2)
    return msqrt(mpow(x1 - x2, 2) + mpow(y1 - y2, 2))
end

local function h_calc(self, x, y)
    return dist(self._e[1], self._e[2], x, y)
end

local function g_calc(x1, y1, x2, y2)
    return dist(x1, y1, x2, y2)
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
    assert(in_range(self, x, y))
    self._s = { x, y }
end

function mt:set_end(x, y)
    assert(mtype(x) == 'integer' and mtype(y) == 'integer')
    assert(in_range(self, x, y))
    self._e = { x, y }
end

function mt:add_block(x, y)
    assert(mtype(x) == 'integer' and mtype(y) == 'integer')
    assert(in_range(self, x, y))
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
    self._temp_path = nil
    assert(self._s, 'no start pos')
    assert(self._e, 'no end pos')
    local sx, sy = self._s[1], self._s[2]
    local ex, ey = self._e[1], self._e[2]
    assert(not(is_block(self, sx, sy)), 'start pos in block')
    assert(not(is_block(self, ex, ey)), 'end pos in block')
    if is_same(sx, sy, ex, ey) then
        return true, {}
    end
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
            self._temp_path = path
            return true, path
        end
        for i = node.x - 1, node.x + 1 do
            for j = node.y - 1, node.y + 1 do
                if in_range(self, i, j) and not(is_same(i, j, node.x, node.y)) and
                   not(is_block(self, i, j)) and not(get_set_node(closeset, i, j)) then
                    local pg = g_score[node] + g_calc(node.x, node.y, i, j)
                    local ph = h_calc(self, i, j)
                    local open_node = get_set_node(openset, i, j)
                    if open_node then
                        if pg < g_score[open_node] then
                            fresh_path(g_score, open_node, pg, found_path, node)
                        end
                    else
                        local new_node = {
                            x = i,
                            y = j,
                            h = ph,
                        }
                        insert_openset(openset, g_score, pg + ph, new_node)
                        fresh_path(g_score, new_node, pg, found_path, node)
                    end
                end
            end
        end
    end
    return false
end

function mt:dump()
    print('S====================')
    local function _in_path(self, x, y)
        if not(self._temp_path) then
            return
        end
        for _, v in pairs(self._temp_path) do
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
function M.new(w, h)
    assert(mtype(w) == 'integer' and mtype(h) == 'integer' and w > 0 and h > 0)
    local astar = {
        w = w,
        h = h,
        block = {},
    }
    return setmetatable(astar, mt)
end
return M
