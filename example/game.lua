local C = require 'qsf.c'

local function main(args)
    print(args[1] .. ' started')
    --C.launch('SharedService', 'sdkauth', 'sdkauth')
    while true do
        local from, msg = C.recv()
        if from == 'sys' then
            if msg == 'exit' then 
                C.send(from, 'ok')
                break 
            end
        end
    end
end

main{...}