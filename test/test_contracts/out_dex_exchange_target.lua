-- out dex exchange real contract
type Storage = {
    name: string,
    feeReceiver: string,
    state: string,
    admin: string
}

let NATIVE_EXCHANGE_ORDER_STATE_CANCELED = 0

let NATIVE_EXCHANGE_ORDER_STATE_COMPLETED = 1

var M = Contract<Storage>()

function M:init()
    self.storage.name = ''
    self.storage.feeReceiver = ''
    self.storage.state = "NOT_INITED"
    self.storage.admin = caller_address
end


let function get_from_address()
    var from_address: string
    let prev_contract_id = get_prev_call_frame_contract_address()
    if prev_contract_id and is_valid_contract_address(prev_contract_id) then
        from_address = prev_contract_id
    else
        from_address = caller_address
    end
    return from_address
end

let function checkAdmin(self: table)
    if self.storage.admin ~= get_from_address() then
        return error("you are not admin, can't call this function")
    end
end

let function parse_at_least_args(arg: string, count: int, error_msg: string)
    if not arg then
        return error(error_msg)
    end
    let parsed = string.split(arg, ',')
    let parsed_count = #parsed
    if (not parsed) or (parsed_count < count) then
        return error(error_msg)
    end
    return parsed
end

let function getAddrByPubk(self: table, pubkeyHex: string)
    return pubkey_to_address(pubkeyHex)
end

let function onAssetOrTokenBalanceDeposit(self: table, from_addr: string, symbol: string, amount: int)
    let balance = tointeger(fast_map_get(from_addr, symbol) or 0)
    fast_map_set(from_addr, symbol, balance + amount)
    let eventArg = {
        from_address: from_addr,
        symbol: symbol,
        amount: amount
    }
    emit Deposited(json.dumps(eventArg))
    eventArg.from_address = nil
    eventArg.address = from_addr
    emit UserBalanceChange(json.dumps(eventArg))
end

function M:init_config(arg: string)
    checkAdmin(self)
    if self.storage.state ~= 'NOT_INITED' then
        return error('this exchange contract inited before')
    end
    let feeReceiver = arg
    if not is_valid_address(feeReceiver) then
        return error("invalid fee receiver address")
    end
    self.storage.feeReceiver = feeReceiver
    self.storage.state = 'COMMON'
    emit Inited(feeReceiver)
end

-- return [addr, id, ok]
let function getOrderOwnerAddressAndId(o: table)
    let sig_hex = tostring(o.sig)
    let infostr = tostring(o.orderInfo)
    let orderinfoDigest = sha256_hex(str_to_hex(infostr))
    let temp = infostr .. sig_hex
    let orderID = sha256_hex(str_to_hex(temp))
    let id = orderID
    if o.id ~= id then
        return ['', '', 'id not match']
    end
    let recoved_public_key = signature_recover(sig_hex, orderinfoDigest)
    if (not recoved_public_key) or (#recoved_public_key < 1) then
        return error("invalid signature")
    end
    let addr = getAddrByPubk(self, recoved_public_key)
    return [addr, id, 'OK']
end

-- return [addr, id, eventOrder, isCompleted, orderInfo]
let function checkOrder(self: table, fillOrder: table)
    let addrAndId = getOrderOwnerAddressAndId(totable(fillOrder.order))
    if addrAndId[3] ~= 'OK' then
        return error("fillOrder wrong")
    end
    let addr = tostring(addrAndId[1])
    let id = tostring(addrAndId[2])
    let orderInfoV = json.loads(tostring(fillOrder.order.orderInfo))
    if not orderInfoV then
        return error("fillOrderInfo not map str")
    end
    let orderInfo = totable(orderInfoV)
    let purchaseNum = tointeger(orderInfo.purchaseNum)
    let payNum = tointeger(orderInfo.payNum)
    let getNum = tointeger(fillOrder.getNum)
    let spentNum = tointeger(fillOrder.spentNum)
    var minFee = 0
    let now = get_chain_now()
    if tointeger(orderInfo.expiredAt or 0) <= now then
        return error("order expired, order id: " .. id)
    end
    var validNum = true
    if purchaseNum <= 0 then
        validNum = false
    end
    if payNum <= 0 then
        validNum = false
    end
    if getNum <= 0 then
        validNum = false
    end
    if spentNum <= 0 then
        validNum = false
    end
    if not validNum then
        return error("num must > 0")
    end
    if spentNum > payNum then
        return error("spentNum must < payNum")
    end
    var feeReceiver = tostring(orderInfo.relayer)
    if tostring(orderInfo.relayer) == "" then
        feeReceiver = tostring(self.storage.feeReceiver)
    else
        if not is_valid_address(tostring(orderInfo.relayer)) then
            return error("relayer is invalid address")
        end
        feeReceiver = tostring(orderInfo.relayer)
    end
    if not tonumber(orderInfo.fee) then
        return error("invalid fee percentage")
    end
    var spentFee = safemath.number_toint(safemath.number_multiply(safemath.safenumber(spentNum), safemath.safenumber(orderInfo.fee)))
    let temp_minFee = fast_map_get(tostring(orderInfo.payAsset), 'minFee')
    if temp_minFee ~= nil then
        minFee = tointeger(temp_minFee)
    end
    if spentFee < 0 then
        return error("fee percentage must positive")
    elseif spentFee < minFee then
        spentFee = minFee
    elseif spentFee >= spentNum then
        return error("fee percentage must < 100%")
    end
    var isBuyOrder = false
    let orderInfoType = tostring(orderInfo['type'])
    if orderInfoType == 'buy' then
        isBuyOrder = true
    elseif orderInfoType == 'sell' then
        isBuyOrder = false
    else 
        return error("order type wrong")
    end

    let sn_purchaseNum = safemath.safenumber(purchaseNum)
    let sn_payNum = safemath.safenumber(payNum)
    let a = safemath.number_multiply(safemath.safenumber(spentNum), sn_purchaseNum)
    let b = safemath.number_multiply(safemath.safenumber(getNum), sn_payNum)

    if safemath.number_gt(a, b) then
        return error("fill order price not satify")
    end

    -- check balance
    var balance = tointeger(fast_map_get(addr, tostring(orderInfo.payAsset)) or 0)
    if balance < tointeger(spentNum + spentFee) then -- add fee
        return error("not enough balance of user " .. addr)
    end

    -- check order  canceled , remain num
    let orderStore = totable(fast_map_get(id, 'info'))
    var lastSpentNum = 0
    var lastGotNum = 0
    if orderStore then
        let o = orderStore
        if o.state ~= nil then
            let state = tointeger(o.state or 0)
            if state == 0 then -- NATIVE_EXCHANGE_ORDER_STATE_CANCELED
                return error("order has been canceled")
            elseif state == 1 then -- NATIVE_EXCHANGE_ORDER_STATE_COMPLETED
                return error("order has been completely filled")
            else
                return error("wrong order state")
            end
        end
        if o.spentNum ~= nil then
            lastSpentNum = tointeger(o.spentNum)
            let remainNum = payNum - lastSpentNum
            if remainNum < spentNum then
                return error("order remain not enough")
            end
            -- write
            o.spentNum = lastSpentNum + spentNum
            if o.gotNum == nil then
                return error("wrong order info stored")
            end
            lastGotNum = tointeger(o.gotNum)
            o.gotNum = lastGotNum + getNum
        end
        fast_map_set(id, 'info', o)
    else
        let m = { gotNum: getNum, spentNum: spentNum }
        fast_map_set(id, 'info', m)
    end

    -- write bal
    fast_map_set(addr, tostring(orderInfo.payAsset), balance - spentNum - spentFee)
    balance = tointeger(fast_map_get(addr, tostring(orderInfo.purchaseAsset)) or 0)
    fast_map_set(addr, tostring(orderInfo.purchaseAsset), balance + getNum)

    -- fee
    if spentFee > 0 then
        let feeReceiverBal = tointeger(fast_map_get(feeReceiver, tostring(orderInfo.payAsset)) or 0)
        fast_map_set(feeReceiver, tostring(orderInfo.payAsset), feeReceiverBal + spentFee) 
    end

    var ss = ''
    var baseNum = 0
    var quoteNum = 0

    if isBuyOrder then
        baseNum = purchaseNum - getNum - lastGotNum
        quoteNum = payNum - spentNum - lastSpentNum
        if baseNum > 0 then
            -- quoteNum = baseNum * (payNum / purchaseNum);
            quoteNum = safemath.number_toint(safemath.number_multiply(safemath.safenumber(baseNum), safemath.number_div(sn_payNum, sn_purchaseNum)))
        end
        ss = ss .. tostring(baseNum) .. ',' .. tostring(quoteNum) .. ',' .. addr .. ',' .. id .. ',' .. tostring(lastGotNum+getNum) .. ',' .. tostring(lastSpentNum+spentNum) .. ',' .. tostring(getNum) .. ',' .. tostring(spentNum) .. ',' .. tostring(spentFee)
    else
        baseNum = payNum - spentNum - lastSpentNum
        quoteNum = purchaseNum - getNum - lastGotNum
        if baseNum > 0 then
            -- quoteNum = baseNum * (purchaseNum / payNum);
            let sn_quoteNum = safemath.number_multiply(safemath.safenumber(baseNum), safemath.number_div(sn_purchaseNum, sn_payNum))
            quoteNum = safemath.number_toint(sn_quoteNum)
            if safemath.number_ne(sn_quoteNum, safemath.safenumber(quoteNum)) then
                quoteNum = quoteNum + 1
            end
            if quoteNum == 0 then
                quoteNum = 1
            end
        end
        ss = ss .. tostring(baseNum) .. ',' .. tostring(quoteNum) .. ',' .. addr .. ',' .. id .. ',' .. tostring(lastSpentNum + spentNum) .. ',' .. tostring(lastGotNum+getNum) .. ',' .. tostring(spentNum) .. ',' .. tostring(getNum) .. ',' .. tostring(spentFee)
    end
    
    var isCompleted = false

    if baseNum <= 0 then
        isCompleted = true
    else
        isCompleted = false
    end
    let eventOrder = ss


    -- UserBalanceChange event
    let event_arg = {}
    -- spent
    event_arg["symbol"] = orderInfo.payAsset
    event_arg["address"] = addr
    event_arg["amount"] = - tointeger(spentNum + spentFee)
    emit UserBalanceChange(json.dumps(event_arg))

    if spentFee > 0 then
        event_arg["symbol"] = orderInfo.payAsset
        event_arg["address"] = feeReceiver
        event_arg["amount"] = spentFee
        emit UserBalanceChange(json.dumps(event_arg))
    end
    
    event_arg["symbol"] = orderInfo.purchaseAsset
    event_arg["address"] = addr
    event_arg["amount"] = getNum
    emit UserBalanceChange(json.dumps(event_arg))

    return [addr, id, eventOrder, isCompleted, orderInfo]
end

let function checkMatchedOrders(self: table, takerFillOrder: table, makerFillOrders: table)
    let orderInfoWithExtra = checkOrder(self, takerFillOrder) -- [addr, id, eventOrder, isCompleted, orderInfo]
    let takerAddr = tostring(orderInfoWithExtra[1])
    let takerOrderId = tostring(orderInfoWithExtra[2])
    let takerEventOrder = tostring(orderInfoWithExtra[3])
    var isCompleted: bool
    if orderInfoWithExtra[4] then
        isCompleted = true
    else
        isCompleted = false
    end
    let orderInfo = totable(orderInfoWithExtra[5])

    let asset1 = tostring(orderInfo.purchaseAsset)
    let asset2 = tostring(orderInfo.payAsset)

    let takerOrderType = tostring(orderInfo['type'])
    let purchaseNum = tointeger(orderInfo.purchaseNum)
    let payNum = tointeger(orderInfo.payNum)

    let takerGetNum = tointeger(takerFillOrder.getNum)
    let takerSpentNum = tointeger(takerFillOrder.spentNum)

    var totalMakerGetNum = 0
    var totalMakerSpentNum = 0

    let takerType = tostring(orderInfo['type'])
    
    --{"OrderType":"sell","putOnOrder":"128,5,test11,2a96ff7713b7e8c6e34dab2
    --de5c2a5844bbc60435b12da5495324d19f0d61853","exchangPair":"HC/COIN","transactionB
    --uys":["0,0,test12,bd3bbc9fbc9a09eb43fd952cac840104f4fa0c9f54795e14af8938fff5a10f
    --de,125,6,125,6"],"transactionSells":["0,0,test11,2a96ff7713b7e8c6e34dab2de5c2a5
    --844bbc60435b12da5495324d19f0d61853,125,6,125,6"],"transactionResult":"FULL_COM
    --PLETED","totalExchangeBaseAmount":125,"totalExchangeQuoteAmount":6}
    
    var takerIsBuy = false
    if takerOrderType == "buy" then
        takerIsBuy = true
    end
    let event_arg = {}
    let transactionBuys = []
    let transactionSells = []
    let changedOrders = []
    event_arg["OrderType"] = takerOrderType
    -- char temp[1024] = {0};
    var eventName = ''
    var ss = ''
    if takerIsBuy then
        eventName = "BuyOrderPutedOn"
        table.append(transactionBuys, takerEventOrder)
        event_arg["totalExchangeBaseAmount"] = takerGetNum
        event_arg["totalExchangeQuoteAmount"] = takerSpentNum
        event_arg["exchangPair"] = tostring(orderInfo.purchaseAsset) .. "/" .. tostring(orderInfo.payAsset)
        ss = ss .. tostring(orderInfo.purchaseNum) .. "," .. tostring(orderInfo.payNum) .. "," .. takerAddr .. "," .. takerOrderId
    else
        eventName = "SellOrderPutedOn"
        table.append(transactionSells, takerEventOrder)
        event_arg["totalExchangeBaseAmount"] = takerSpentNum
        event_arg["totalExchangeQuoteAmount"] = takerGetNum
        event_arg["exchangPair"] = tostring(orderInfo.payAsset) .. "/" .. tostring(orderInfo.purchaseAsset)
        ss = ss .. tostring(orderInfo.payNum) .. "," .. tostring(orderInfo.purchaseNum) .. "," .. takerAddr .. "," .. takerOrderId
    end
    event_arg["putOnOrder"] = ss
    if isCompleted then
        event_arg["transactionResult"] = "FULL_COMPLETED"
    else
        event_arg["transactionResult"] = "PARTLY_COMPLETED"
    end

    table.append(changedOrders, takerOrderId)

    var item: object
    for _, item in ipairs(makerFillOrders) do
        let tempInfo = checkOrder(self, totable(item)) -- [addr, id, eventOrder, isCompleted, orderInfo]
        let address = tostring(tempInfo[1])
        let id = tostring(tempInfo[2])
        let eventOrder = tostring(tempInfo[3])
        var isCompleted: bool
        if tempInfo[4] then
            isCompleted = true
        else
            isCompleted = false
        end
        let orderInfo = totable(tempInfo[5])

        if asset1 ~= tostring(orderInfo.payAsset) or asset2 ~= tostring(orderInfo.purchaseAsset) then
            return error("asset not match")
        end
        if takerOrderType == orderInfo['type'] then
            return error("order type not match")
        end

        totalMakerGetNum = tointeger(item.getNum) + totalMakerGetNum
        totalMakerSpentNum = tointeger(item.spentNum) + totalMakerSpentNum

        if (takerIsBuy) then
            table.append(transactionSells, eventOrder)
        else
            table.append(transactionBuys, eventOrder)
        end
        table.append(changedOrders, id)
    end

    if (totalMakerGetNum ~= takerSpentNum) or (totalMakerSpentNum ~= takerGetNum) then
        return error("total exchange num not match")
    end
    event_arg["transactionSells"] = transactionSells
    event_arg["transactionBuys"] = transactionBuys
    
    if eventName == 'BuyOrderPutedOn' then
        emit BuyOrderPutedOn(json.dumps(event_arg))
    elseif eventName == 'SellOrderPutedOn' then
        emit SellOrderPutedOn(json.dumps(event_arg))
    end

    emit FillOrders(json.dumps(changedOrders))
end

function M:fillOrder(arg: string)
    if self.storage.state ~= 'COMMON' then
        return error("this exchange contract state is not common")
    end
    let argInfo = totable(json.loads(arg))
    let matchInfo = argInfo
    matchInfo.fillTakerOrder = matchInfo.fillTakerOrder or []
    matchInfo.fillMakerOrders = matchInfo.fillMakerOrders or []
    checkMatchedOrders(self, totable(matchInfo.fillTakerOrder), totable(matchInfo.fillMakerOrders))
    return 'OK'
end

function M:cancelOrders(arg: string)
    if self.storage.state ~= 'COMMON' then
        return error("this exchange contract state is not common")
    end
    let canceledOrderIds = []
    let argInfo = totable(json.loads(arg))
    let argOrders = argInfo
    let callerAddr = get_from_address()
    var i: int
    for i = 1, #argOrders, 1 do
        let addrAndId = getOrderOwnerAddressAndId(totable(argOrders[i]))
        let addr = tostring(addrAndId[1])
        let id = tostring(addrAndId[2])
        let ok = addrAndId[3]
        if ok == 'OK' then
            if callerAddr == addr then
                let orderInfo = fast_map_get(id, 'info')
                if orderInfo then
                    var doIt = true
                    if orderInfo.state ~= nil then
                        let state = tointeger(orderInfo.state)
                        if state == 0 then
                            doIt = false
                        end
                    end
                    if doIt then
                        orderInfo.state = 0
                        fast_map_set(id, 'info', orderInfo)
                        table.append(canceledOrderIds, id)
                    end
                else
                    let m = {state: 0}
                    fast_map_set(id, 'info', m)
                    table.append(canceledOrderIds, id)
                end
            end
        end
    end
    let result = json.dumps(canceledOrderIds)
    if #canceledOrderIds > 0 then
        emit CancelOrders(result)
    end
    return result
end

offline function M:balanceOf(arg: string)
    let parsed_args = parse_at_least_args(arg, 2, "argument format error, need format: address,symbol")
    let balanceObj = fast_map_get(tostring(parsed_args[1]), tostring(parsed_args[2]))
    var balance = 0
    if balanceObj then
        balance = tointeger(balanceObj)
    end
    return tostring(balance)
end

offline function M:getOrder(orderId: string)
    let info = fast_map_get(orderId, "info")
    if info then
        return json.dumps(info)
    else
        return '{}'
    end
end

offline function M:state(arg: string)
    return self.storage.state
end

-- api_arg: asset_name
offline function M:minFee(arg: string)
    let tempMinFee = fast_map_get(arg, 'minFee')
    return tointeger(tempMinFee)
end

function M:setMinFee(arg: string)
    checkAdmin(self)
    if self.storage.state ~= 'COMMON' then
        return error("this exchange contract state is not common")
    end
    let parsed_args = parse_at_least_args(arg, 2, 'argument format error, need format: asset_symbol,minFee')
    let asset_symbol = tostring(parsed_args[1])
    let minFee = tointeger(parsed_args[2])
    if (not asset_symbol) or (#asset_symbol < 1) then
        return error("invalid asset symbol/address")
    end
    if (not minFee) or (minFee < 0) then
        return error("argument format error, minFee must be non-negative integral")
    end
    fast_map_set(asset_symbol, 'minFee', minFee)
end

-- args:amount,symbol
function M:withdraw(arg: string)
    if self.storage.state ~= 'COMMON' then
        return error("this exchange contract state is not common")
    end
    let parsed = parse_at_least_args(arg, 2, "argument format error, need format: amount,symbol")
    let amount = tointeger(parsed[1])
    if (not amount) or (amount <= 0) then
        return error("invalid amount")
    end
    let symbol = tostring(parsed[2])
    let from = get_from_address()
    let balance = tointeger(fast_map_get(from, symbol) or 0)
    if amount > balance then
        return error("not enough balance to withdraw")
    end
    let newBalance = balance - amount
    if newBalance == 0 then
        fast_map_set(from, symbol, nil)
    else 
        fast_map_set(from, symbol, newBalance)
    end
    if is_valid_contract_address(symbol) then
        let tokenContract = import_contract_from_address(symbol)
        tokenContract:transfer(from .. "," .. tostring(amount))
    else
        let res = transfer_from_contract_to_address(from, symbol, amount)
        if res ~= 0 then
            return error("transfer from contract to address " .. from .. " error, code is " .. tostring(res))
        end
    end

    let eventArg = {
        symbol: symbol,
        to_address: from,
        amount: amount
    }
    emit Withdrawed(json.dumps(eventArg))

    eventArg.to_address = nil
    eventArg.amount = -amount
    eventArg.address = from
    emit UserBalanceChange(json.dumps(eventArg))
end

offline function M:getAddrByPubk(pubkeyHex: string)
    return getAddrByPubk(self, pubkeyHex)
end

-- args: publicKey_hexString,symbol
offline function M:balanceOfPubk(arg: string)
    let parsed = parse_at_least_args(arg, 2, "argument format error, need format: publicKey_hexString,symbol")
    let addr = tostring(self:getAddrByPubk(tostring(parsed[1])))
    if (not addr) or (#addr < 1) then
        return error("invalid publickey to hex")
    end
    let balanceObj = fast_map_get(addr, tostring(parsed[2]))
    var balance = 0
    if balanceObj then
        balance = tointeger(balanceObj)
    end
    return tostring(balance)
end

function M:on_deposit_asset_by_call(arg: string)
    -- only called by caller/proxy contract
    var ok: bool = false
    if in_delegate_call() then
        ok = true
    else
        if get_prev_call_frame_api_name() == "on_deposit_asset" then
            ok = true
        end
    end
    if not ok then
        return error("only can be called by user or proxy contract")
    end
    if self.storage.state ~= 'COMMON' then
        return error("this exchange contract state is not common")
    end
    let argInfo = totable(json.loads(arg))
    let amount = tointeger(argInfo.num)
    let symbol = tostring(argInfo.symbol)
    if amount <= 0 then
        return error("invalid amount")
    end
    let addr = caller_address
    return onAssetOrTokenBalanceDeposit(self, addr, symbol, amount)
end

function M:on_deposit_contract_token(amountStr: string)
    if self.storage.state ~= 'COMMON' then
        return error("this exchange contract state is not common")
    end
    let addr = get_from_address()
    let amount = tointeger(amountStr)
    if (not amount) or (amount <= 0) then
        return error("invalid amount")
    end
    let token_addr = get_prev_call_frame_contract_address()
    
    -- only called by token/proxy contract
    var ok: bool = false
    if in_delegate_call() then
        ok = true
    else
        if get_prev_call_frame_contract_address() == token_addr then
            ok = true
        end
    end
    if not ok then
        return error("only can be called by user or proxy contract")
    end
    return onAssetOrTokenBalanceDeposit(self, addr, token_addr, amount)
end

function M:on_deposit_asset(arg: string)
    self:on_deposit_asset_by_call(arg) -- TODO: change it
    -- error("only supported called by proxy contract")
end

function M:on_upgrade()
end

function M:on_destory()
    error("not supported")
end

return M
