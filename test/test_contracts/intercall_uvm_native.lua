

type Storage = {
}

-- events: Transfer, Paused, Resumed, Stopped, AllowedLock, Locked, Unlocked

var M = Contract<Storage>()

let function parse_at_least_args(arg: string, count: int, error_msg: string)
    if not arg then
        return error(error_msg)
    end
    let parsed = string.split(arg, ',')
    if (not parsed) or (#parsed < count) then
        return error(error_msg)
    end
    return parsed
end

function M:init()
    print("token contract created")
end


--- args : exchangContract,tokenContract,amount
function M:depositToken2ExchangeContract(args: string)
	let parsed = parse_at_least_args(args, 3, "argument format error, need format is exchangContract,tokenContract,amount")
    let nativecontract = tostring(parsed[1])
    let tokenContract = tostring(parsed[2])
	let amount = tointeger(parsed[3])

	fast_map_set("depositToken2ExchangeContract","state1","callnative1")
	
    let contractobj = import_contract_from_address(nativecontract)
	let r = tostring(contractobj:depositToken(tostring(amount)..","..tokenContract))
	print("call nativecontract r:"..r)
	fast_map_set("depositToken2ExchangeContract","state2",r)
end

--- args : exchangContract,tokenContract,amount
function M:depositToken2ExchangeContractTwice(args: string)
	let parsed = parse_at_least_args(args, 3, "argument format error, need format is exchangContract,tokenContract,amount")
    let nativecontract = tostring(parsed[1])
    let tokenContract = tostring(parsed[2])
	let amount = tointeger(parsed[3])

	fast_map_set("depositToken2ExchangeContractTwice","state1","callnative1")
	
    let contractobj = import_contract_from_address(nativecontract)
	let r = tostring(contractobj:depositToken(tostring(amount)..","..tokenContract))
	print("call nativecontract r:"..r)
	fast_map_set("depositToken2ExchangeContractTwice","state2",r)
	
	let r2 = tostring(contractobj:depositToken(tostring(amount)..","..tokenContract))
	print("call nativecontract r2:"..r2)
	fast_map_set("depositToken2ExchangeContractTwice","state3",r2)
end


--- args : exchangContract,tokenContract,amount,tokenContract2,amount2
function M:depositToken2ExchangeAndTransfer(args: string)
	let parsed = parse_at_least_args(args, 5, "argument format error, need format is exchangContract,tokenContract,amount,tokenContract2,amount2")
    let nativecontract = tostring(parsed[1])
    let tokenContract = tostring(parsed[2])
	let amount = tointeger(parsed[3])
	let tokenContract2 = tostring(parsed[4])
	let amount2 = tointeger(parsed[5])

	fast_map_set("depositToken2ExchangeContractAndTransfer","state1","callnative1")
	
	
    let contractobj = import_contract_from_address(nativecontract)
	let r = tostring(contractobj:depositToken(tostring(amount)..","..tokenContract))
	print("call nativecontract r:"..r)
	fast_map_set("depositToken2ExchangeContractAndTransfer","state2",r)
	
	let tokencontract2obj = import_contract_from_address(tokenContract2)
	let r2 = tostring(tokencontract2obj:transfer(caller_address ..","..tostring(amount2)))
	print("call token transfer r2:"..r2)
	fast_map_set("depositToken2ExchangeContractAndTransfer","state3",r2)
end





--- args : exchangContract,tokenContract,amount
function M:withdrawTokenFromExchangeContract(args: string)
	let parsed = parse_at_least_args(args, 3, "argument format error, need format is exchangContract,tokenContract,amount")
    let nativecontract = tostring(parsed[1])
    let tokenContract = tostring(parsed[2])
	let amount = tointeger(parsed[3])

	fast_map_set("withdrawTokenFromExchangeContract","state1","callnative1")
	
    let contractobj = import_contract_from_address(nativecontract)
	let r = tostring(contractobj:withdraw(tostring(amount)..","..tokenContract))
	print("call nativecontract r:"..r)
	fast_map_set("withdrawTokenFromExchangeContract","state2",r)
end


--- args : tokenContract,toAddress,amount
function M:transferToken(args: string)
	let parsed = parse_at_least_args(args, 3, "argument format error, need format is tokenContract,toAddress,amount")
    let tokenContract = tostring(parsed[1])
    let toAddress = tostring(parsed[2])
	let amount = tointeger(parsed[3])

	fast_map_set("transferToken","state1","call token")
	
    let contractobj = import_contract_from_address(tokenContract)
	let r = tostring(contractobj:transfer(toAddress..","..tostring(amount)))
	print("call contract r:"..r)
	fast_map_set("transferToken","state2",r)
end


--- args : tokenContract,spentAddress,amount
function M:approveToken(args: string)
	let parsed = parse_at_least_args(args, 3, "argument format error, need format is tokenContract,spentAddress,amount")
    let tokenContract = tostring(parsed[1])
    let spentAddress = tostring(parsed[2])
	let amount = tointeger(parsed[3])

	fast_map_set("approveToken","state1","call token")
	
    let contractobj = import_contract_from_address(tokenContract)
	let r = tostring(contractobj:approve(spentAddress..","..tostring(amount)))
	print("call contract r:"..r)
	fast_map_set("approveToken","state2",r)
end



function M:setr(args: string)
	fast_map_set("setr","bb",args)

	return "OKK"
end



return M
