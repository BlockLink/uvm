type Storage = {
    name: string,
    feeReceiver: string,
    state: string,
    admin: string,
    util_contract_addr: string,
    target_contract: string
}

type Contract<S> = {
    storage: S
}

var M = Contract<Storage>()

-- type State = 'NOT_INITED' | 'COMMON'

function M:init()
    self.storage.name = ''
    self.storage.feeReceiver = ''
    self.storage.state = 'NOT_INITED'
    self.storage.admin = caller_address
    self.storage.util_contract_addr = ''
    self.storage.target_contract = ''
end

function M:set_target(target_contract_addr: string)
    self.storage.target_contract = target_contract_addr
end

let function split_first_arg(arg: string)
    var i: int
    var first_arg_end_index = -1
    for i = 1,#arg, 1 do
        if string.sub(arg, tointeger(i), i+1) == ',' then
            first_arg_end_index = i
            break
        end
    end
    var real_api_name = ''
    var real_api_arg = ''
    if first_arg_end_index <= 0 then
        real_api_name = arg
        real_api_arg = ''
    else
        real_api_name = string.sub(arg, 1, first_arg_end_index-1)
        real_api_arg = string.sub(arg, tointeger(first_arg_end_index+1))
    end
    return [real_api_name, real_api_arg]
end

-- arg: api_name,args
function M:call_api(arg: string)
    let args_splited = split_first_arg(arg)
    let real_api_name = tostring(args_splited[1])
    let real_api_arg = tostring(args_splited[2])
    return delegate_call(self.storage.target_contract, real_api_name, real_api_arg)
end

-- arg: api_name,args
offline function M:offline_call_api(arg: string)
    let args_splited = split_first_arg(arg)
    let real_api_name = tostring(args_splited[1])
    let real_api_arg = tostring(args_splited[2])
    return delegate_call(self.storage.target_contract, real_api_name, real_api_arg)
end

function M:on_deposit_contract_token(amountStr: string)
    return delegate_call(self.storage.target_contract, 'on_deposit_contract_token', amountStr)
end

function M:on_deposit_asset(arg: string)
    delegate_call(self.storage.target_contract, 'on_deposit_asset_by_call', arg)
end

function M:on_upgrade()
end

function M:on_destory()
    error("not supported")
end

return M
