type State = "DEPOSITED" | "EXITING" | "EXITED"

type Storage = {
    operatorAuthority: string,
    vmcContractAddress: string,
    smtContractAddress: string,
    numCoins: int,
    currentBlockNum: int,
    childBlockInterval: int
}

let MATURITY_PERIOD = 604800 -- 7 days
let BOND_AMOUNT = 100000


type Exit = {
    prevOwner: string,
    owner: string,
    ownerPubKey: string,
    createdAt: int,
    bond: int,
    balance: int, -- balance is the balance of the coin that is being
    prevBlock: int,
    exitBlock: int
}

type Coin = {
    state: State,
    owner: string, -- who owns that nft
    contractAddress: string, -- which contract does the coin belong to
    exit: Exit,
    uid: string, -- asset symbol in root chain
    denomination: int,
    balance: int,
    depositBlock: int
}

type ChildBlock = {
    root: string,
    createdAt: int
}

type Balance = {
    bonded: int,
    withdrawable: int
}

type Challenge = {
    owner: string,
    challenger: string,
    txHash: string,
    challengingBlockNumber: int
}

-- fast map coins: slotId => Coin
-- fast map childChain: blockNum => ChildBlock
-- fast map: exitSlots: slotId => bool
-- fast map: balances: address => Balance

-- events: Deposit, SubmittedBlock, StartedExit, ChallengedExit, RespondedExitChallenge, FinalizedExit, FreedBond, WithdrewBonds, SlashedBond, Withdrew

-- ValidatorManagerContract: checkValidator(address)
-- SparseMerkleTreeContract: verify("uid,leave_hash,tree_root,proof")

var M = Contract<Storage>()


-- parse a,b,c format string to [a,b,c]
let function parse_args(arg: string, count: int, error_msg: string)
    if not arg then
        return error(error_msg)
    end
    let parsed = string.split(arg, ',')
    if (not parsed) or (#parsed ~= count) then
        return error(error_msg)
    end
    return parsed
end

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

let function arrayContains(col: Array<object>, item: object)
    if not item then
        return false
    end
    var value: object
    for _, value in ipairs(col) do
        if value == item then
            return true
        end
    end
    return false
end

let function indexOfArray(col: Array<object>, item: object)
    if not item then
        return -1
    end
    var value: object
    var idx: int = 0
    for idx, value in ipairs(col) do
        if value == item then
            return idx
        end
    end
    return -1
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

var vmc: table = nil

let function getVmc(M: table)
    if not vmc then
        vmc = import_contract_from_address(tostring(M.storage.vmcContractAddress))
        if not vmc then
            error("can't find the ValidatorManagerContract instance")
        end
    end
    return vmc
end

var smt: table = nil

let function getSmt(M: table)
    if not smt then
        smt = import_contract_from_address(tostring(M.storage.smtContractAddress))
        if not smt then
            error("can't find the SparseMerkleTree contract instance")
        end
    end
    return smt
end

let function checkIsValiator(M: table) 
    let from = get_from_address()
    let vmc = getVmc(M)
    if vmc:checkValidator(from) then
        return true
    else
        error("only validator can call this api")
        return false
    end
end

function M:init()
    -- TODO
end

function M:set_config(arg: string)
    -- TODO
end

let function create_coin(M: table, from: string, uid: string, denomination: int)
    let from_caller = get_from_address()
    M.storage.currentBlock = tointeger(M.storage.currentBlock) + 1
    let slotInfoToPack: Array<object> = [tointeger(M.storage.numCoins), from_caller, from]
    let slotInfoPackedHex = sha256_hex(cbor_encode(slotInfoToPack))
    let slot = string.sub(slotInfoPackedHex, 0, 16)
    let coinStr = tostring(fast_map_get("coins", slot))
    var coin: Coin
    if not coinStr then
        coin = Coin()
    else
        coin = totable(json.loads(coinStr))
    end
    coin.uid = uid
    coin.contractAddress = ""
    coin.denomination = denomination
    coin.depositBlock = get_header_block_num() + 1
    coin.owner = from
    coin.state = "DEPOSITED"

    -- save signed transaction hash as root
    -- hash for deposit transactions is the hash of its slot
    let childBlock = ChildBlock()
    childBlock.root = sha256_hex(cbor_encode(slot))
    childBlock.createdAt = get_chain_now()
    fast_map_set("childChain", tostring(M.storage.currentBlockNum), json.dumps(childBlock))

    let vmc = getVmc(M)
    if vmc:checkValidator(from) then
        -- the coin is an empty coin that is owned by the validators
        coin.balance = 0
    else
        -- otherwise it's wholy owned by the user
        coin.balance = denomination
    end
      
    -- create a utxo at `slot`
    let eventArg = {
        'slot': slot,
        'currentBlockNum': M.storage.currentBlockNum,
        'denomination': coin.balance,
        'from': from,
        'caller': from_caller
    }
    emit Deposit(json.dumps(eventArg))

    M.storage.numCoins = tointeger(M.storage.numCoins) + 1
    fast_map_set("coins", slot, json.dumps(coin))
end

-- arg format: assetSymbol,amount
function M:on_deposit_asset(arg: string)
    let parsed = parse_at_least_args(arg, 2, "arg need format assetSymbol,amount")
    let uid = tostring(parsed[1])
    let amount = tointeger(parsed[2])    
    let from_caller = get_from_address()
    let from = from_caller
    create_coin(self, from, uid, amount)
end

-- validator to create empty coin
-- arg format: assetSymbol
function M:create_empty_coin(arg: string)
    checkIsValiator(self)
    let assetSymbol = arg
    let from_caller = get_from_address()
    let from = from_caller
    create_coin(self, from, assetSymbol, 0)
end

-- called by a Validator to append a Plasma block to the Plasma chain
-- arg format: block_txs_merkle_root
function M:submitBlock(arg: string)
    checkIsValiator(self)
    let root = arg
    self.storage.currentBlockNum = (self.storage.currentBlockNum + self.storage.childBlockInterval) // self.storage.childBlockInterval * self.storage.childBlockInterval
    let childBlock = ChildBlock()
    childBlock.root = root
    childBlock.createdAt = get_chain_now()
    fast_map_set("childChain", tostring(self.storage.currentBlockNum), json.dumps(childBlock))
    let eventArg = {
        'currentBlock': currentBlock,
        'root': root,
        'timestamp': get_chain_now()
    }
    emit SubmittedBlock(json.dumps(eventArg));
end

let function checkMembership(M:table, txHash: string, root: string, slot: string, proofHex: string)
    let smt = getSmt(M)
    return smt:verify(slot .. "," .. txHash .. "," .. root .. "," .. proof)
end

let function checkTxIncluded(M: table, slot: string, txHash: string, blockNumber: int, proofHex: string)
    let childBlockStr = tostring(fast_map_get("childChain", tostring(blockNumber)))
    if not childBlockStr then
        return error("invalid blocknumber in submited childChain")
    end
    let childBlock: ChildBlock = totable(json.loads(childBlockStr))
    if not childBlock then
        return error("invalid child block info")
    end
    let root = childBlock.root
    if (blockNumber % tointeger(M.storage.childBlockInterval)) ~= 0 then
        -- Check against block root for deposit block numbers
        if txHash ~= root then
            return error("deposit block number's root should be tx hash")
        end
    else
        -- Check against merkle tree for all other block numbers
        if not checkMembership(M, txHash, root, slot, proofHex) then
            return error("Tx not included in claimed block")
        end
    end
end

let function checkIncludedAndSigned(M: table, exitingTxHex: string, exitingTxInclusionProofHex: string, signatureHex: string, blkNum: int)
    let txData = totable(cbor_decode(exitingTxHex))
    let txOwnerPubKey = tostring(txData["ownerPubKey"])
    let txSigHash = tostring(txData["sigHash"])
    let txHash = tostring(txData["hash"])
    -- Deposit transactions need to be signed by their owners
    if txOwnerPubKey ~= signature_recover(signatureHex, txSigHash) then
        return error("invalid signature")
    end
    let slot = tostring(txData["slot"])
    checkTxIncluded(M, slot, txHash, blkNum, exitingTxInclusionProofHex)
end

let function checkBothIncludedAndSigned(M: table, prevTxHex: string, exstingTxHex: string, prevTxInclusionProofHex: string, exitingTxInclusionProofHex: string,
    signatureHex: string, block1Num: int, block2Num: int)
    if block1Num >= block2Num then
        return error("invalid blocks numbers")
    end
    let exitingTxData = totable(cbor_decode(existingTxHex))
    let prevTxData = totable(cbor_decode(prevTxHex))
    if (not exitingTxData) or (not prevTxData) then
        return error("decode tx data failed")
    end
    -- Both transactions need to be referring to the same slot
    if tostring(exitingTxData["slot"]) ~= tostring(prevTxData["slot"]) then
        return error("exit tx and prev tx have not same slot")
    end

    -- The exiting transaction must be signed by the previous transaciton's owner
    let prevTxOwnerPubKey = tostring(prevTxData["ownerPubKey"])
    let exitingTxSigHash = tostring(exitingTxData["sigHash"])
    if signature_recover(signatureHex, exitingTxSigHash) ~= prevTxOwnerPubKey then
        return error("Invalid signature")
    end

    -- Both transactions must be included in their respective blocks
    checkTxIncluded(M, tostring(prevTxData["slot"]), tostring(prevTxData["hash"]), block1Num, prevTxInclusionProofHex);
    checkTxIncluded(M, tostring(exitingTxData["slot"]), tostring(exitingTxData["hash"]), block2Num, exitingTxInclusionProofHex);
end

let function doInclusionChecks(M: table, prevTxHex: string, exitingTxHex: string, prevTxInclusionProofHex: string, exitingTxInclusionProofHex: string,
     signatureHex: string, block1Num: int, block2Num: int)
    if (block1Num % childBlockInterval ~= 0) then
        checkIncludedAndSigned(M, 
            exitingTxHex,
            exitingTxInclusionProofHex,
            signatureHex,
            block1Num
        )
    else
        checkBothIncludedAndSigned(M, 
            prevTxHex, exitingTxHex, prevTxInclusionProofHex,
            exitingTxInclusionProofHex, signatureHex,
            block1Num, block2Num
        )
    end
end

let function pushExit(slot: string, prevOwner: string, exitOwner: string, exitOwnerPubKey: string, balance: int, block1Num: int, block2Num: int)
    let from_caller = get_from_address()

    -- Push exit to list
    fast_map_set("exitSlots", slot, true)

    -- Create exit
    let coinStr = tostring(fast_map_get("coins", slot))
    let c: Coin = totable(json.loads(coinStr))
    let exit = Exit()
    exit.prevOwner = prevOwner
    exit.owner = exitOwner
    exit.ownerPubKey = exitOwnerPubKey
    exit.createdAt = get_chain_now()
    exit.balance = balance
    exit.bond = 0 -- msg.value
    exit.prevBlock = block1Num
    exit.exitBlock = block2Num
    c.exit = exit
    -- Update coin state
    c.state = "EXITING";
    fast_map_set("coins", slot, json.dumps(c))
    let eventArg = {
        'slot': slot,
        'caller': from_caller
    }
    emit StartedExit(json.dumps(eventArg))
end


-- arg format: slot,prevTxHex,existingTxHex,prevTxInclusionProofHex,exitingTxInclusionProofHex,signatureHex,block1Num,block2Num
function M:startExit(arg: string)
    let parsed = parse_at_least_args(arg, 8, "need arg format: slot,prevTxHex,existingTxHex,prevTxInclusionProofHex,exitingTxInclusionProofHex,signatureHex,block1Num,block2Num")
    let slot = tostring(parsed[1])
    let prevTxHex = tostring(parsed[2])
    let existingTxHex = tostring(parsed[3])
    let prevTxInclusionProofHex = tostring(parsed[4])
    let exitingTxInclusionProofHex = tostring(parsed[5])
    let signatureHex = tostring(parsed[6])
    let block1Num = tointeger(parsed[7])
    let block2Num = tointeger(parsed[8])

    let from_caller = get_from_address()
    let from_caller_pubkey = caller

    let existingTx = totable(cbor_decode(existingTxHex))
    if (not existingTx) or (not existingTx['owner']) then
        return error("invalid existing tx structure")
    end
    if (tostring(existingTx['owner'])) ~= from_caller then
        return error("caller is not ths existing tx's owner")
    end
    let prevTx = totable(cbor_decode(prevTxHex))

    doInclusionChecks(self,
        prevTxHex, exitingTxHex,
        prevTxInclusionProofHex, exitingTxInclusionProofHex,
        signatureHex,
        block1Num, block2Num
    )
    pushExit(slot,
        tostring(prevTx["owner"]),
        from_caller, from_caller_pubkey,
        tointeger(existingTx["balance"]),
        block1Num, block2Num
    )
end
-- bond related

let function freeBond(from: string)
    let balanceStr = tostring(fast_map_get("balances", from))
    if not balanceStr then
        return
    end
    let balance: Balance = totable(json.loads(balanceStr))
    balance.bonded = balance.bonded - BOND_AMOUNT
    balance.withdrawable = balance.withdrawable + BOND_AMOUNT
    let eventArg = {
        'from': from,
        'band_amount': BOND_AMOUNT
    }
    emit FreedBond(json.dumps(eventArg))
end

let function withdrawBonds()
    -- Can only withdraw bond if the msg.sender
    let from_caller = get_from_address()
    let balanceStr = tostring(fast_map_get("balances", from_caller))
    if not balanceStr then
        return
    end
    let balance: Balance = totable(json.loads(balanceStr))
    let amount = balance.withdrawable;
    balance.withdrawable = 0 -- no reentrancy
    if is_valid_contract_address(from_caller) then
        return error("only can withdraw to not contract address now")
    end
    let res = transfer_from_contract_to_address(from_caller, get_system_asset_symbol(), amount)
    if res ~= 0 then
        return error("transfer to address error with code ".. tostring(res))
    end
    let eventArg = {
        'caller': from_caller,
        'amount': amount
    }
    fast_map_set("balances", from_caller, json.dumps(balance))
    emit WithdrewBonds(json.dumps(eventArg))
end

let function slashBond(from: string, to: string)
    let fromBalanceStr = tostring(fast_map_get("balances", from))
    if not fromBalanceStr then
        return
    end
    let fromBalance: Balance = totable(json.loads(fromBalanceStr))
    let toBalanceStr = tostring(fast_map_get("balances", to))
    if not toBalanceStr then
        return
    end
    let toBalance: Balance = totable(json.loads(toBalanceStr))
    fromBalance.bonded = fromBalance.bonded - BOND_AMOUNT
    toBalance.withdrawable = toBalance.withdrawable + BOND_AMOUNT
    let eventArg = {
        'from': from,
        'to': to,
        'bond_amount': BOND_AMOUNT
    }
    emit SlashedBond(json.dumps(eventArg))
end

let function checkPendingChallenges(slot: string)
    let slotChallengesStr = tostring(fast_map_get('challenges', slot))
    if not slotChallengesStr then
        return false
    end
    let slotChallenges = totable(json.loads(slotChallengesStr))
    let length = #slotChallenges
    var slashed = false
    var i: int = 1
    for i = 1,length do
        let challenge = slotChallenges[i]
        if tostring(challenge["txHash"]) and (tostring(challenge["txHash"]) ~= "") then
            -- Penalize the exitor and reward the first valid challenger.
            if (not slashed) then
                let coin: Coin = totable(fast_map_get("coins", slot))
                slashBond(tostring(coin.exit.owner), tostring(challenge.challenger))
                slashed = true
            end
            -- Also free the bond of the challenger.
            freeBond(tostring(challenge.challenger))

            -- Challenge resolved, delete it
            table.remove(slotChallenges, i)
            fast_map_set("challenges", slot, slotChallenges)
            hasChallenges = true
        end
    end
end

-- Finalizes an exit, i.e. puts the exiting coin into the EXITED
-- state which will allow it to be withdrawn, provided the exit has
-- matured and has not been successfully challenged
-- arg format: slot
function M:finalizeExit(arg: string)
    let slot = arg
    let coinStr = tostring(fast_map_get("coins", slot))
    if not coinStr then
        return error("can't find coin of this slot")
    end
    let coin: Coin = totable(json.loads(coinStr))

    -- If a coin is not under exit/challenge, then ignore it
    if (coin.state ~= 'EXITING') then
        return error("only EXITING coin state can be finalized")
    end

    -- If an exit is not matured, ignore it
    if ((get_chain_now() - tointeger(coin.exit.createdAt)) <= MATURITY_PERIOD) then
        return error("this exit is not matured")
    end

    -- Check if there are any pending challenges for the coin.
    -- `checkPendingChallenges` will also penalize
    -- for each challenge that has not been responded to
    let hasChallenges = checkPendingChallenges(slot);

    if (not hasChallenges) then
        -- Update coin's owner and balance
        coin.owner = coin.exit.owner
        coin.balance = coin.exit.balance
        coin.state = "EXITED"

        -- Allow the exitor to withdraw their bond
        freeBond(coin.owner)

        let finalizeExitEventArg = {
            'slot': slot,
            'owner': coin.owner
        }
        emit FinalizedExit(json.dumps(finalizeExitEventArg))
    else
        -- Reset coin state since it was challenged
        coin.state = "DEPOSITED"
    end
    coin.exit = nil
    fast_map_set("exitSlots", slot, nil)
    fast_map_set("coins", slot, json.dumps(coin))
end

let function isState(slot: string, state: State)
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    if (not coin) or (coin.state ~= state) then
        return error("Wrong state")
    end
end

-- Withdraw a UTXO that has been exited
-- arg format: slot
function M:withdraw(arg: string)
    let from_caller = get_from_address()
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    if (not coin) or (coin.owner ~= from_caller) then
        return error("You do not own that UTXO")
    end
    if not isState(slot, "EXITED") then
        return "this slot is not EXITED state"
    end
    let uid = coin.uid
    let toUser = coin.balance
    let toAuthority = coin.denomination - coin.balance

    -- Delete the coin that is being withdrawn
    fast_map_set("coins", slot, nil)
    let c = coin
    if (toUser > 0) then
        let res = transfer_from_contract_to_address(from_caller, get_system_asset_symbol(), toUser)
        if res ~= 0 then
            return error("error when transfer to address with error code " .. tostring(res))
        end
    end
    if (toAuthority > 0) then
        let res2 = transfer_from_contract_to_address(self.storage.operatorAuthority, get_system_asset_symbol(), toAuthority)
        if res2 ~= 0 then
            return error("error when transfer to address with error code " .. tostring(res2))
        end
    end
    let withdrawEventArg = {
        'caller': from_caller,
        'uid': uid,
        'contractAddress': c.contractAddress,
        'toUser': toUser,
        'toAuthority': toAuthority
    }
    emit Withdrew(json.dumps(withdrawEventArg));
end


-- @param slot The slot of the coin being challenged
-- @param owner The user claimed to be the true ower of the coin
let function setChallenged(slot: string, owner: string, challengingBlockNumber: int, txHash: string)
    -- Require that the challenge is in the first half of the challenge window
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", "slot"))))
    if get_chain_now() > (coin.exit.createdAt + CHALLENGE_WINDOW) then
        return error("challenge window expired")
    end
    let slotChallenges = totable(json.loads(tostring(fast_map_get("challenges", slot))))
    if arrayContains(slotChallenges, txHash) then
        return error("Transaction used for challenge already")
    end

    let from_caller = get_from_address()

    -- Need to save the exiting transaction's owner, to verify
    -- that the response is valid
    let challenge = Challenge()
    challenge.owner = owner
    challenge.challenger = from_caller
    challenge.txHash = txHash
    challenge.challengingBlockNumber = challengingBlockNumber
    table.append(slotChallenges, challenge);
    fast_map_set("challenges", slot, json.dumps(slotChallenges))

    let eventArg = {
        'slot': slot,
        'txHash': txHash
    }
    emit ChallengedExit(json.dumps(eventArg))
end


-- Submits proof of a transaction before prevTx as an exit challenge
-- @notice Exitor has to call respondChallengeBefore and submit a
--         transaction before prevTx or prevTx itself.
-- arg format: slot,prevTxHex,txHex,prevTxInclusionProofHex,txInclusionProofHex,signatureHex,block1Num,block2Num
-- @param signature The signature of the txBytes by the coin owner indicated in prevTx
-- @param block1Num: the block containing the prevTx
-- @param block2Num: the block containing the exitingTx
function M:challengeBefore(arg: string)
    -- TODO: check bond amount
    let parsed = parse_at_least_args(arg, 8, "need arg format: slot,prevTxHex,txHex,prevTxInclusionProofHex,txInclusionProofHex,signatureHex,block1Num,block2Num")
    let slot = tostring(parsed[1])
    let prevTxHex = tostring(parsed[2])
    let txHex = tostring(parsed[3])
    let prevTxInclusionProofHex = tostring(parsed[4])
    let txInclusionProofHex = tostring(parsed[5])
    let signatureHex = tostring(parsed[6])
    let block1Num = tointeger(parsed[7])
    let block2Num = tointeger(parsed[8])
    if (not isState(slot, "EXITING")) then
        return error("only slot with EXITING state can be challenged")
    end
    doInclusionChecks(
        self,
        prevTxHex, txHex,
        prevTxInclusionProofHex, txInclusionProofHex,
        signatureHex,
        block1Num, block2Num
    )
    let txData = totable(cbor_decode(txHex))
    setChallenged(slot, tostring(txData.owner), block2Num, tostring(txData.hash))
end

let function checkResponse(M: table, slot: string, index: int, blockNumber: int, txHex: string, signatureHex: string, proofHex: string)
    let txData = totable(cbor_decode(txHex))
    let sigHash = tostring(txData["sigHash"])
    let slotChallenges = totable(json.loads(tostring(fast_map_get("challenges", slot))))
    if signature_recover(signatureHex, sigHash) ~= slotChallenges[index]["ownerPubKey"] then
        return error("invalid signature")
    end
    if tostring(txData.slot) ~= slot then
        return error("Tx is referencing another slot")
    end
    if blockNumber <= tointeger(slotChallenges[index].challengingBlockNumber) then
        return error("block number should be greater than this challenge's challenging block number")
    end
    checkTxIncluded(M, tostring(txData.slot), tostring(txData.hash), blockNumber, proofHex)
end

--- Submits proof of a later transaction that corresponds to a challenge
-- @notice Can only be called in the second window of the exit period.
-- arg format: slot,challengingTxHash,respondingBlockNumber,respondingTransactionHex,proofHex,signatureHex
-- @param slot The slot corresponding to the coin whose exit is being challenged
-- @param challengingTxHash The hash of the transaction
--        corresponding to the challenge we're responding to
-- @param respondingBlockNumber The block number which included the transaction
--        we are responding with
-- @param respondingTransactionHex The RLP-encoded transaction involving a particular
--        coin which took place directly after challengingTransaction
-- @param proofHex An inclusion proof of respondingTransaction
-- @param signatureHex The signature which proves a direct spend from the challenger
function M:respondChallengeBefore(arg: string)
    let parsed = parse_at_least_args(arg, 6, "need arg format: slot,challengingTxHash,respondingBlockNumber,respondingTransactionHex,proofHex,signatureHex")
    let slot = tostring(parsed[1])
    let challengingTxHash = tostring(parsed[2])
    let respondingBlockNumber = tointeger(parsed[3])
    let respondingTransactionHex = tostring(parsed[4])
    let proofHex = tostring(parsed[5])
    let signatureHex = tostring(parsed[6])

    let from_caller = get_from_address()

    -- Check that the transaction being challenged exists
    let slotChallenges: Array<Challenge> = totable(json.loads(tostring(fast_map_get("challenges", slot))))
    if (not slotChallenges) or (not arrayContains(slotChallenges, challengingTxHash)) then
        return error("Responding to non existing challenge")
    end

    -- Get index of challenge in the challenges array
    let index = tointeger(indexOfArray(slotChallenges, challengingTxHash))

    checkResponse(self, slot, index, respondingBlockNumber, respondingTransactionHex, signatureHex, proofHex)

    -- If the exit was actually challenged and responded, penalize the challenger and award the responder
    slashBond(slotChallenges[index].challenger, from_caller)

    --Put coin back to the exiting state
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    coin.state = "EXITING"
    table.remove(slotChallenges, index)
    fast_map_set("challenges", slot, json.dumps(slotChallenges))
    let eventArg = {
        'slot': slot
    }
    emit RespondedExitChallenge(json.dumps(eventArg));
end

let function cleanupExit(slot: string)
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    coin.exit = nil
    fast_map_set("coins", slot, json.dumps(coin))
    fast_map_set("exitSlots", slot, nil)
end


-- Must challenge with a tx in between
-- Check that the challenging transaction has been signed
-- by the attested previous owner of the coin in the exit
let function checkBetween(M: table, slot: string, txHex: string, blockNumber: int, signatureHex: string, proofHex: string)
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    if (not coin) or (coin.exit.exitBlock <= blockNumber) or (coin.exit.prevBlock >= blockNumber) then
        return error("Tx should be between the exit's blocks")
    end
    let txData = totable(cbor_decode(txHex))
    if signature_recover(signatureHex, tostring(txData.sigHash)) ~= tostring(coin.exit.prevOwner) then
        return error("Invalid signature")
    end
    if tostring(txData.slot) ~= slot then
        return error("Tx is referencing another slot")
    end
    checkTxIncluded(M, slot, tostring(txData.hash), blockNumber, proofHex)
end

let function checkAfter(M: table, slot: string, txHex: string, blockNumber: int, signatureHex: string, proofHex: string)
    let txData = totable(cbor_decode(txHex))
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    if signature_recover(signatureHex, tostring(txData.sigHash)) ~= coin.exit.ownerPubKey then
        return error("Invalid signature")
    end
    if tostring(txData.slot) ~= slot then
        return error("Tx is referencing another slot")
    end
    if tostring(txData.prevBlock) ~= coin.exit.exitBlock then
        return error("Not a direct spend")
    end
    checkTxIncluded(M, slot, tostring(txData.hash), blockNumber, proofHex)
end

let function applyPenalties(slot: string)
    -- Apply penalties and change state
    let coin: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    let from_caller = get_from_address()
    slashBond(coin.exit.owner, from_caller)
    coin.state = "DEPOSITED"
    fast_map_set("coins", slot, json.dumps(coin))
end

-- arg format: slot,challengingBlockNumber,challengingTransactionHex,proofHex,signatureHex
function M:challengeBetween(arg: string)
    let parsed = parse_at_least_args(arg, 5, "need arg format: slot,challengingBlockNumber,challengingTransactionHex,proofHex,signatureHex")
    let slot = tostring(parsed[1])
    let challengingBlockNumber = tointeger(parsed[2])
    let challengingTransactionHex = tostring(parsed[3])
    let proofHex = tostring(parsed[4])
    let signatureHex = tostring(parsed[5])
    cleanupExit(slot)
    if not isState(slot, "EXITING") then
        return error("require slot with EXITING state")
    end
    checkBetween(self, slot, challengingTransactionHex, challengingBlockNumber, signatureHex, proofHex)
    applyPenalties(slot)
end

-- arg format: slot,challengingBlockNumber,challengingTransactionHex,proofHex,signatureHex
function M:challengeAfter(arg: string)
    let parsed = parse_at_least_args(arg, 5, "need arg format: slot,challengingBlockNumber,challengingTransactionHex,proofHex,signatureHex")
    let slot = tostring(parsed[1])
    let challengingBlockNumber = tointeger(parsed[2])
    let challengingTransactionHex = tostring(parsed[3])
    let proofHex = tostring(parsed[4])
    let signatureHex = tostring(parsed[5])
    if not isState(slot, "EXITING") then
        return error("require slot with EXITING state")
    end
    cleanupExit(slot)
    checkAfter(self, slot, challengingTransactionHex, challengingBlockNumber, signatureHex, proofHex)
    applyPenalties(slot)
end

-- arg format: txHash,root,slot,proofHex
function M:checkMembership(arg: string)
    let parsed = parse_at_least_args(arg, 4, "need arg format: txHash,root,slot,proofHex")
    let txHash = tostring(parsed[1])
    let root = tostring(parsed[2])
    let slot = tostring(parsed[3])
    let proofHex = tostring(parsed[4])
    let smt = getSmt(self)
    return smt:checkMembership(txHash, root, slot, proofHex)
end

-- arg format: slot
offline function M:getPlasmaCoin(arg: string)
    let slot = arg
    let c: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    return c
end

-- arg format: slot
offline function M:getExit(arg: string)
    let slot = arg
    let c: Coin = totable(json.loads(tostring(fast_map_get("coins", slot))))
    let e = c.exit
    let info = {
        'owner': e.owner,
        'prevBlock': e.prevBlock,
        'exitBlock': e.exitBlock,
        'coin_state': c.state
    }
    return info
end

-- arg format: blockNumber
offline function M:getBlockRoot(arg: string)
    let blockNumber = tointeger(arg)
    let childBlock: ChildBlock = totable(json.loads(tostring(fast_map_get("childChain", tostring(blockNumber)))))
    if childBlock then
        return childBlock.root
    else
        return nil
    end
end

function M:on_destroy()
    error("destroy this contract is disabled")
end

return M
