--
-- testing case runner
--

local test_cases = 
{
    --'test_process',
    --'test_mq',
    --'test_utf8',
    'test_zmq'
}

for _, v in pairs(test_cases) do 
    require(v)
end