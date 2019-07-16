type Storage = {
    data: string
}

var M = Contract<Storage>()

function M:init()
    self.storage.data = ''
end

function M:set_data(arg: string)
    -- need check from_address() and it must be proxy address
    self.storage.data = arg
end

function M:hello(name: string)
    return "hello, name is " .. name .. " and data is " .. (self.storage.data)
end

function M:on_deposit_asset_by_call(arg: string)
    -- need check from_address() and it must be proxy address
    print("called by proxy with arg " .. arg)
end

return M
