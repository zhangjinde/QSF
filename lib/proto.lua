local cmsgpack = require 'cmsgpack'
local table = table

local proto = {}

function proto.dispatch(service, msg)
    local request = cmsgpack.unpack(msg)
    local func = service[request.id]
    if func then
        local result = func(table.unpack(request.args))
        if result then
            return cmsgpack.pack{id=request.id, res=result}
        end
    else
        error('client method not found', request.id)
    end
end

return proto