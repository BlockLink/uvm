type Storage = {
    proxyAddr: string
}

var M = Contract<Storage>()

function M:init()
    self.storage.proxyAddr = ''
end

function M:set_proxy(target: string)
    self.storage.proxyAddr = target
end

function M:set_data(arg: string)
    let res = delegate_call(self.storage.proxyAddr, 'set_data', arg)
    return res
end

function M:hello(name: string)
    let res = delegate_call(self.storage.proxyAddr, 'hello', name)
    print("proxy target response", res)
    return res
end

function M:on_deposit_asset(arg: string)
    delegate_call(self.storage.proxyAddr, 'on_deposit_asset_by_call', arg)
end

function M:withdraw(arg: string)
    return delegate_call(self.storage.proxyAddr, 'withdraw', arg)
end

function M:query_balance(arg: string)
    return delegate_call(self.storage.proxyAddr, 'query_balance', arg)
end

return M
