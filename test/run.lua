local qsf = require 'qsf'

local function handle_message(from, msg)
    local from, msg = qsf.recv()
    if from == 'sys' and msg == 'exit' then
        qsf.send(from, 'roger')
        return false
    end
    return true
end

local function main(args)
    print(args[1] .. ' started')
    while true do
        if not handle_message() then
            break
        end
    end
end

main{...}