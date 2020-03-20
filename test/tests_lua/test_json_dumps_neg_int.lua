type Record = {
    assetName: string,
    flow: int,
    profit: int
}

let data = Record()
data.assetName = 'TEST'
data.flow = 100
data.profit = tointeger(-10310548528)

let dataStr = json.dumps(data)
print("dumps done")

let data2 = json.loads(tostring(dataStr))
print("loads done")
