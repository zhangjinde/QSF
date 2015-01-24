-- Extensions to the table module

local table = require 'table'

local next, pairs, type, setmetatable, getmetatable
    = next, pairs, type, setmetatable, getmetatable
    
local tinsert, tsort = table.insert, table.sort

-- Return whether table is empty.
function table.empty(t)
    return not next(t)
end

-- Find the number of elements in a table.
function table.size(t)
    local n = 0
    for _ in pairs(t) do
        n = n + 1
    end
    return n
end

-- Make the list of keys in table.
function table.keys(t)
    local u = {}
    for k, _ in pairs(t) do
        tinsert(u, k)
    end
    return u
end

-- Make the list of values of a table.
function table.values(t)
    local u = {}
    for _, v in pairs(t) do
        u[#u + 1] = v
    end
    return u
end

-- Reverse a list.
function table.reverse (l)
    local m = {}
    for i = #l, 1, -1 do
        tinsert(m, l[i])
    end
    return m
end

-- Preserve core table sort function.
function table.sort(t, f)
    tsort(t, f)
    return t
end

local function copyTable(st)
    local tab = {}
    for k, v in pairs(st or {}) do
        if type(v) ~= "table" then
            tab[k] = v
        else
            tab[k] = copyTable(v)
        end
    end
    return tab
end

-- Make a shallow copy of a table
function table.copy(t, nometa)
    local u = copyTable(t)
    if not nometa then
        setmetatable(u, getmetatable(t))
    end
    return u
end

-- Make a deep copy of a tree, including any metatables
function table.clone(t, nometa)
    local r = {}
    if not nometa then
        setmetatable(r, getmetatable(t))
    end
    local d = {[t] = r}
    local function copy(o, x)
        for i, v in pairs (x) do
            if type (v) == "table" then
                if not d[v] then
                    d[v] = {}
                    if not nometa then
                        setmetatable (d[v], getmetatable (v))
                    end
                    o[i] = copy (d[v], v)
                else
                    o[i] = d[v]
                end
            else
                o[i] = v
            end
        end
        return o
    end
  return copy(r, t)
end
