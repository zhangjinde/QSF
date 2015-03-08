--
-- testing case runner
--

local testing_cases = 
{
    'test_process',
    'test_mq',
    'test_zmq',    
    'test_uuid',
    'test_crypto',
    --'test_timer',
}

for _, v in pairs(testing_cases) do 
    require(v)
end