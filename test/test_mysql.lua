local mysql = require 'mysql'

print('mysql client version:', mysql._VERSION)

local host = '192.168.0.112'
local user = 'starraider'
local pwd = 'GLuQyueGZVDqjpZN'
local db = 'world'
local conf = 
{
    host = host,
    user = user,
    passwd = pwd,
    db = db,
}

local function test_option()
    local flag = mysql.CLIENT_MULTI_STATEMENTS | mysql.CLIENT_MULTI_RESULTS
    conf.client_flag = flag

    local client = mysql.createClient()
    client:setCharset('utf8')
    client:setReconnect(true)
    client:setConnectTimeout(3)
    client:setReadTimeout(3)
    client:setWriteTimeout(3)
    client:setProtocol(mysql.PROTOCOL_TCP)
    client:setCompress()
    client:connect(conf)
    client:close()
end

local function test_insert() 
    local stmt = [[CREATE TABLE IF NOT EXISTS `mytest`(
        `id` int NOT NULL UNIQUE KEY,
        `name` varchar(32) NOT NULL,
        `score` int DEFAULT 0
        ) ]]

    local client = mysql.createClient()
    client:connect(conf)
    local r = client:execute(stmt)

    local total = 10000
    for n=1, total do
        local fmt = [[insert into mytest values(%d, '%s', %d)]]
        local stmt = string.format(fmt, n, 'nn' .. n, math.random(100))
        assert(client:execute(stmt) == 1)
        print(stmt)
    end

    r = client:execute('delete from mytest')
    assert(r == total)
end

local stmt = [[select * from country where continent='asia' limit 2]]

local function query_use_result()
    print('query_use_result:')
    local client = mysql.createClient()
    client:connect(conf)
    local cur = client:execute(stmt, 'use')
    local result = {}
    while true do
         local r = cur:fetch() -- numeric table index
         if not r then break end
         for k, v in ipairs(r) do
            print(v)
         end
         print('-----------------------')
    end
    print('-----------------------')
end

local function query_use_result_alpha_index()
    print('query_use_result_alpha_index:')
    local client = mysql.createClient()
    client:connect(conf)
    local cur = client:execute(stmt, 'use')
    while true do
         local r = cur:fetch('a') -- alphabetic table index
         if not r then break end
         for k, v in pairs(r) do
            print(k, v)
         end
         print('-----------------------')
    end
    print('-----------------------')
end

local function query_store_result()
    print('query_store_result:')
    local client = mysql.createClient()
    client:connect(conf)
    local cur = client:execute(stmt)
    local result = cur:fetchAll()
    for _, row in ipairs(result) do
        for _, v in ipairs(row) do
            print(v)
        end
        print('-----------------------')
    end
    print('-----------------------')
end

local function query_store_result_alpha_index()
    print('query_store_result_alpha_index:')
    local client = mysql.createClient()
    client:connect(conf)
    local cur = client:execute(stmt)
    local num_rows = cur:numrows()
    for n=1, num_rows do
        local r = cur:fetch('a') -- alphabetic table index
        for k, v in pairs(r) do
            print(k, v)
        end
        print('-----------------------')
    end
    print('-----------------------')
end

test_option()
query_use_result()
query_use_result_alpha_index()
query_store_result()
query_store_result_alpha_index()
