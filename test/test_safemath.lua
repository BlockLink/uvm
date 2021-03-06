print('hello')
var a = safemath.bigint('123')
pprint('a=', a)
var b = safemath.bigint(456)
pprint('b=', b)
pprint('hex(a)=', safemath.tohex(a))
pprint('int(a)=', safemath.toint(a))
pprint('str(a)=', safemath.tostring(a))
var c = safemath.add(a, b)
var d = safemath.sub(a, b)
var e = safemath.mul(a, b)
var f = safemath.div(b, a)
var g = safemath.pow(a, safemath.bigint(10))
var h = safemath.bigint('3333333333333333333333333333333333332')
var j = safemath.rem(h, safemath.bigint(2))
pprint('c', safemath.tostring(c))
pprint('d', safemath.tostring(d))
pprint('e', safemath.tostring(e))
pprint('f', safemath.tostring(f))
pprint('g', safemath.tostring(g))
pprint('h', safemath.tostring(h))
pprint('j', safemath.tostring(j))
pprint("try parse a1 is:", safemath.bigint('a1'))

var c1 = safemath.gt(a, b)
var c2 = safemath.ge(a, b)
var c3 = safemath.lt(a, b)
var c4 = safemath.le(a, b)
var c5 = safemath.eq(a, b)
var c6 = safemath.ne(a, b)
var c7 = safemath.eq(a, a)

pprint(safemath.tostring(a) .. ' > ' .. safemath.tostring(b) .. '=' .. tostring(c1))
pprint(safemath.tostring(a) .. ' >= ' .. safemath.tostring(b) .. '=' .. tostring(c2))
pprint(safemath.tostring(a) .. ' < ' .. safemath.tostring(b) .. '=' .. tostring(c3))
pprint(safemath.tostring(a) .. ' <= ' .. safemath.tostring(b) .. '=' .. tostring(c4))
pprint(safemath.tostring(a) .. ' == ' .. safemath.tostring(b) .. '=' .. tostring(c5))
pprint(safemath.tostring(a) .. ' != ' .. safemath.tostring(b) .. '=' .. tostring(c6))
pprint(safemath.tostring(a) .. ' == ' .. safemath.tostring(a) .. '=' .. tostring(c7))
