var e = {name="uvm", age=25}
e["123"] = "test"
var s = ''
var k
for k in pairs(e) do
	s = s .. ';' .. k .. ':' .. tostring(e[k])
end

print(s)
