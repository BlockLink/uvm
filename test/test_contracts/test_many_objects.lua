type Storage = {

}

var M = Contract<Storage>()

function M:init()

end

function M:hello(arg: string)

    let loop_count = 10000

    for i=1,loop_count do
        let a = {name: "hello", age: i, memo: "helloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworld"}
        let b = json.dumps(a)
    end
    print("done")

end

return M
