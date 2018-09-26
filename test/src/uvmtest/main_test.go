package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"testing"

	"github.com/bitly/go-simplejson"
	"github.com/stretchr/testify/assert"
)

func findUvmSinglePath() string {
	dir, err := os.Getwd()
	if err != nil {
		panic(err)
	}
	dirAbs, err := filepath.Abs(dir)
	if err != nil {
		panic(err)
	}
	uvmDir := filepath.Dir(filepath.Dir(filepath.Dir(dirAbs)))
	if runtime.GOOS == "windows" {
		return filepath.Join(uvmDir, "x64", "Debug", "uvm_single.exe")
	} else {
		return filepath.Join(uvmDir, "uvm_single")
	}
}

func findUvmCompilerPath() string {
	dir, err := os.Getwd()
	if err != nil {
		panic(err)
	}
	dirAbs, err := filepath.Abs(dir)
	if err != nil {
		panic(err)
	}
	uvmDir := filepath.Dir(filepath.Dir(filepath.Dir(dirAbs)))
	if runtime.GOOS == "windows" {
		return filepath.Join(uvmDir, "test", "uvm_compiler.exe")
	} else {
		return filepath.Join(uvmDir, "test", "uvm_compiler")
	}
}

func findSimpleChainPath() string {
	dir, err := os.Getwd()
	if err != nil {
		panic(err)
	}
	dirAbs, err := filepath.Abs(dir)
	if err != nil {
		panic(err)
	}
	uvmDir := filepath.Dir(filepath.Dir(filepath.Dir(dirAbs)))
	if runtime.GOOS == "windows" {
		return filepath.Join(uvmDir, "x64", "Debug", "simplechain.exe")
	} else {
		return filepath.Join(uvmDir, "simplechain_runner")
	}
}

func findUvmAssPath() string {
	dir, err := os.Getwd()
	if err != nil {
		panic(err)
	}
	dirAbs, err := filepath.Abs(dir)
	if err != nil {
		panic(err)
	}
	uvmDir := filepath.Dir(filepath.Dir(filepath.Dir(dirAbs)))
	if runtime.GOOS == "windows" {
		return filepath.Join(uvmDir, "test", "uvm_ass.exe")
	} else {
		return filepath.Join(uvmDir, "test", "uvm_ass")
	}
}

var uvmSinglePath = findUvmSinglePath()
var uvmCompilerPath = findUvmCompilerPath()
var uvmAssPath = findUvmAssPath()
var simpleChainPath = findSimpleChainPath()
var simpleChainDefaultPort = 8080

func execCommand(program string, args ...string) (string, string) {
	cmd := exec.Command(program, args...)
	var outb, errb bytes.Buffer
	cmd.Stdin = os.Stdin
	cmd.Stdout = &outb
	cmd.Stderr = &errb
	err := cmd.Run()
	if err != nil {
		fmt.Printf("%v\n", err)
	}
	return outb.String(), errb.String()
}

func execCommandBackground(program string, args ...string) *exec.Cmd {
	cmd := exec.Command(program, args...)
	err := cmd.Start()
	if err != nil {
		fmt.Printf("%v\n", err)
	}
	return cmd
}

func TestHelp(t *testing.T) {
	out, _ := execCommand(uvmSinglePath, "-h") //
	// assert.Equal(t, err, "", "uvm -h should not fail")
	fmt.Println(out)
	// fmt.Println(err)
}

func TestRunTestScript(t *testing.T) {
	out, _ := execCommand(uvmSinglePath, "-t", "../../result.out")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, "say hi api called of contract to China"), "test script should call contract api")
	assert.True(t, strings.Contains(out, "testcase done"), "test script should run")
}

func TestRunContractApi(t *testing.T) {
	out, _ := execCommand(uvmSinglePath, "-k", "../../result.out", "sayHi", "China")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, "say hi api called of contract to China"), "uvm should call contract api")
	assert.True(t, strings.Contains(out, "result: HelloChina"), "contract api should return result")
}

func TestImportSimpleContract(t *testing.T) {
	out, _ := execCommand(uvmSinglePath, "../../load_simple_contract.lua.out")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, "start api called of simple contract"), "uvm should support import contract by script")
	assert.True(t, strings.Contains(out, "result1: 	hello world"), "contract api result error")
}

func TestChangeOtherContractProperty(t *testing.T) {
	out, _ := execCommand(uvmSinglePath, "../../change_other_contract_property.lua.out")
	fmt.Println(out)
	// change other contract's property should throw error
}

func TestDefineGlobalInContract(t *testing.T) {
	_, compileErr := execCommand(uvmCompilerPath, "../../tests_typed/test_define_global_in_contract_loader.lua")
	assert.Equal(t, compileErr, "")
	out, _ := execCommand(uvmSinglePath, "../../tests_typed/test_define_global_in_contract_loader.lua.out")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, `_ENV or _G set hello is forbidden`))
}

func TestManyStringOperations(t *testing.T) {
	out, _ := execCommand(uvmSinglePath, "../../test_many_string_operations.lua.out")
	fmt.Println(out)
}

func TestEmitEvents(t *testing.T) {
	_, compileErr := execCommand(uvmCompilerPath, "../../tests_typed/test_emit_events_valid.glua")
	assert.Equal(t, compileErr, "")
	out, _ := execCommand(uvmSinglePath, "../../tests_typed/test_emit_events_valid.glua.out")
	fmt.Println(out)
}

func TestTypes(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_types.lua")
	out, _ := execCommand(uvmSinglePath, "../../tests_lua/test_types.lua.out")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, "123	4.56	abc	true	[123,4.56,\"abc\",true]"))
	assert.True(t, strings.Contains(out, `b=table: 0`))
	assert.True(t, strings.Contains(out, `c={"b":"userdata","c":{"a":1,"b":"hi"},"name":1}`))
}

func TestThrowError(t *testing.T) {
	_, compilerErr := execCommand(uvmCompilerPath, "../../tests_lua/test_error.lua")
	assert.Equal(t, compilerErr, "")
	_, err := execCommand(uvmSinglePath, "../../tests_lua/test_error.lua.out")
	fmt.Println(err)
	assert.True(t, strings.Contains(err, `dummpy error message`))
}

func TestMath(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_math.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_math.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a1=	123`))
	assert.True(t, strings.Contains(out, `a2=	123.456000`))
	assert.True(t, strings.Contains(out, `a3=	123`))
	assert.True(t, strings.Contains(out, `a4=	nil`))
	assert.True(t, strings.Contains(out, `a5=	nil`))
	assert.True(t, strings.Contains(out, `a6=	123.456789`))
	assert.True(t, strings.Contains(out, `a7=	123`))
	assert.True(t, strings.Contains(out, `a8=	-124`))
	assert.True(t, strings.Contains(out, `a9=	4`))
	assert.True(t, strings.Contains(out, `a10=	2`))
	assert.True(t, strings.Contains(out, `a11=	integer`))
	assert.True(t, strings.Contains(out, `a12=	float`))
	assert.True(t, strings.Contains(out, `a13=	nil`))
	assert.True(t, strings.Contains(out, `a14=	3.141593`))
	assert.True(t, strings.Contains(out, `a15=	9223372036854775807`))
	assert.True(t, strings.Contains(out, `a16=	-9223372036854775808`))
}

func TestTooManyLocalVars(t *testing.T) {
	_, compilerErr := execCommand(uvmCompilerPath, "../../tests_lua/test_too_many_localvars.lua")
	fmt.Println(compilerErr)
	assert.True(t, strings.Contains(compilerErr, "too many local variables(129 variables), but limit count is 128"))
	_, compilerErr2 := execCommand(uvmCompilerPath, "../../tests_lua/test_max_amount_localvars.lua")
	assert.Equal(t, compilerErr2, "")
}

func TestStringOperators(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_string_operators.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_string_operators.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a1=	abc`))
	assert.True(t, strings.Contains(out, `a2=	def`))
	assert.True(t, strings.Contains(out, `a3=	abcdefghj`))
	assert.True(t, strings.Contains(out, `a4=	9`))
	assert.True(t, strings.Contains(out, `a5=	K`))
	assert.True(t, strings.Contains(out, `a6=	9`))
}

func TestTableOperators(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_typed/test_table_module.glua")
	out, err := execCommand(uvmSinglePath, "../../tests_typed/test_table_module.glua.out")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, `a=	[1,2,3,{"name":"hi"}]`))
	assert.True(t, strings.Contains(err, `invalid value (table) at index 4 in table for 'concat'`))
}

func TestPairs(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_typed/test_pairs.glua")
	out, _ := execCommand(uvmSinglePath, "../../tests_typed/test_pairs.glua.out")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, `[[100,200],["a",1],["m",234],["n",123],["ab",1]]`))
}


func TestStringGmatch(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_gmatch.lua")
	out, _ := execCommand(uvmSinglePath, "../../tests_lua/test_gmatch.lua.out")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, `func_itor is	function: 0`))
	assert.True(t, strings.Contains(out, `func_itor ret is 	12`))
	assert.True(t, strings.Contains(out, `2	world`))
	assert.True(t, strings.Contains(out, `to	Lua`))
	assert.True(t, strings.Contains(out, `numret ret is	nil`))
}

func TestContractImport(t *testing.T) {
	// TODO
}

func TestSafeMath(t *testing.T) {
	execCommand(uvmCompilerPath, "../../test_safemath.lua")
	out, err := execCommand(uvmSinglePath, "../../test_safemath.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a=	{"hex":"313233","type":"bigint"}`))
	assert.True(t, strings.Contains(out, `b=	{"hex":"343536","type":"bigint"}`))
	assert.True(t, strings.Contains(out, `hex(a)=	313233`))
	assert.True(t, strings.Contains(out, `int(a)=	123`))
	assert.True(t, strings.Contains(out, `str(a)=	123`))
	assert.True(t, strings.Contains(out, `c	579`))
	assert.True(t, strings.Contains(out, `d	-333`))
	assert.True(t, strings.Contains(out, `e	56088`))
	assert.True(t, strings.Contains(out, `f	3`))
	assert.True(t, strings.Contains(out, `g	792594609605189126649`))
	assert.True(t, strings.Contains(out, `123 > 456=false`))
	assert.True(t, strings.Contains(out, `123 >= 456=false`))
	assert.True(t, strings.Contains(out, `123 < 456=true`))
	assert.True(t, strings.Contains(out, `123 <= 456=true`))
	assert.True(t, strings.Contains(out, `123 == 456=false`))
	assert.True(t, strings.Contains(out, `123 != 456=true`))
	assert.True(t, strings.Contains(out, `123 == 123=true`))
}

func TestJsonModule(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_json.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_json.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a1=	[1,2,3,{"name":"hi"}]`))
	assert.True(t, strings.Contains(out, `a2=	[1,2,3,{"name":"hi"}]`))
	assert.True(t, strings.Contains(out, `a3=	123`))
	assert.True(t, strings.Contains(out, `a4=	123`))
	assert.True(t, strings.Contains(out, `a5=	{"name":"hello"}`))
	assert.True(t, strings.Contains(out, `a6=	{"name":"hello"}`))
}

func TestInvalidUpvalue(t *testing.T) {
	_, assErr := execCommand(uvmAssPath, "../../tests_lua/test_invalid_upvalue.uvms")
	fmt.Println(assErr)
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_invalid_upvalue.out")
	fmt.Println(out)
	fmt.Println(err)
	assert.True(t, strings.Contains(err, `upvalue error`))
}

func TestArrayOp(t *testing.T) {
	// TODO
}

func TestUndump(t *testing.T) {
	// TODO
}

func TestStorage(t *testing.T) {
	// TODO
}

func TestGlobalApis(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_global_apis.lua")
	out, err := execCommand(uvmSinglePath, "-k", "../../tests_lua/test_global_apis.lua.out", "start", "TEST_ADDRESS")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `now is 	0`))
	assert.True(t, strings.Contains(out, `TEST_ADDRESS	 is valid address? 	true`))
	assert.True(t, strings.Contains(out, `TEST_ADDRESS	 is valid contract address? 	true`))
	assert.True(t, strings.Contains(out, `systemAssetSymbol: 	COIN`))
	assert.True(t, strings.Contains(out, `blockNum: 	0`))
	assert.True(t, strings.Contains(out, `precision: 	10000`))
	assert.True(t, strings.Contains(out, `callFrameStackSize: 	0`))
	assert.True(t, strings.Contains(out, `random: 	0`))
	assert.True(t, strings.Contains(out, `prevContractAddr: 	nil`))
	assert.True(t, strings.Contains(out, `prevContractApiName: 	nil`))
	assert.True(t, strings.Contains(out, `result: {"_data":{"id":"../../tests_lua/test_global_apis.lua.out","name":"@self","storage":{"contract":"address"}},"start":"userdata"}`))
}

func TestCallContractItself(t *testing.T) {
	_, compileErr := execCommand(uvmCompilerPath, "../../tests_typed/test_call_contract_it_self.glua")
	assert.Equal(t, compileErr, "")
	out, _ := execCommand(uvmSinglePath, "-k", "../../tests_typed/test_call_contract_it_self.glua.out", "start", "test_arg")
	fmt.Println(out)
	assert.True(t, strings.Contains(out, `attempt to call a nil value (field 'init')`))
}

func TestImportNotFoundContract(t *testing.T) {
	_, compileErr := execCommand(uvmCompilerPath, "../../tests_lua/test_import_not_found.lua")
	assert.Equal(t, compileErr, "")
	out, err := execCommand(uvmSinglePath, "-k", "../../tests_lua/test_import_not_found.lua.out", "start", "test")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `successfully import not found contract as nil`))
}

func TestInvalidUpvalueInContract(t *testing.T) {
	_, compileErr := execCommand(uvmCompilerPath, "../../tests_lua/test_upval.lua")
	fmt.Println(compileErr)
	assert.True(t, strings.Contains(compileErr, `token c, type nil can't access property c`))
}

func TestContractWithoutInitApi(t *testing.T) {
	// TODO
}

func TestExitContractByError(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_exit_contract.lua")
	out, err := execCommand(uvmSinglePath, "-k", "../../tests_lua/test_exit_contract.lua.out", "start", "test")
	fmt.Println(out)
	fmt.Println("error:")
	fmt.Println(err)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `hello, exit message here`))
}

func TestTimeModule(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_time.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_time.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a1=	1234567890`))
	assert.True(t, strings.Contains(out, `a2=	1234654290`))
	assert.True(t, strings.Contains(out, `a3=	-86400`))
	assert.True(t, strings.Contains(out, `a4=	2009-02-14 07:31:30`))
	assert.True(t, strings.Contains(out, `a5=	2009-02-15 07:31:30`))
}

func TestForLoop(t *testing.T) {
	_, compileErr := execCommand(uvmCompilerPath, "../../tests_lua/test_for_loop_goto.lua")
	assert.Equal(t, compileErr, "")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_for_loop_goto.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `;123:test;age:25;name:uvm`))
}

func TestFastMap(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_fastmap.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_fastmap.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a1: 	nil`))
	assert.True(t, strings.Contains(out, `a2: 	123`))
	assert.True(t, strings.Contains(out, `a3: 	234`))
	assert.True(t, strings.Contains(out, `b1: 	world`))
	assert.True(t, strings.Contains(out, `b3: 	nil`))
	assert.True(t, strings.Contains(out, `a4: 	hello`))
}

func TestInvalidByteHeaders(t *testing.T) {
	_, err := execCommand(uvmSinglePath, "../../tests_lua/test_invalid_bytecode_header.bytecode")
	fmt.Println(err)
	assert.True(t, strings.Contains(err, `endianness mismatch in precompiled chunk`))
}

func TestCryptoPrimitivesApis(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_crypto_primitives.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_crypto_primitives.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `b tojson is 	{"1FNfecY7xnKmMQyUpc7hbgSHqD2pD8YDJ7":20000}`))
	assert.True(t, strings.Contains(out, `sha256("")=	e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`))
	assert.True(t, strings.Contains(out, `sha3("")=	c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470`))
	assert.True(t, strings.Contains(out, `sha1("")=	da39a3ee5e6b4b0d3255bfef95601890afd80709`))
	assert.True(t, strings.Contains(out, `ripemd160("")=	9c1185a5c5e9fc54612808977ee8f548b2258d31`))
	assert.True(t, strings.Contains(out, `sha256("a123")=	bcc7b3934d14f9e30c4c0981d5fb31a0685043f6adba6875e5d4cd1c6831b89e`))
	assert.True(t, strings.Contains(out, `sha3("a123")=	367394ba9eec3410aadc8f80eebf0164ff6476cb466fe9d0a5539715e36b7230`))
	assert.True(t, strings.Contains(out, `sha1("a123")=	160bce262c1c1cc1df9ff7c623d950f15861574b`))
	assert.True(t, strings.Contains(out, `ripemd160("a123")=	3c3ce98a97977e4276548854074c4482507c640d`))
	assert.True(t, strings.Contains(out, `hex_to_bytes("a123")=	[161,35]`))
	assert.True(t, strings.Contains(out, `a123=	a123`))
}

func kill(cmd *exec.Cmd) error {
	return cmd.Process.Kill()
	// kill := exec.Command("TASKKILL", "/T", "/F", "/PID", strconv.Itoa(cmd.Process.Pid)) // TODO: when in linux
	// kill.Stderr = os.Stderr
	// kill.Stdout = os.Stdout
	// return kill.Run()
}

type rpcRequest struct {
	Method string        `json:"method"`
	Params []interface{} `json:"params"`
}

func simpleChainRPC(method string, params ...interface{}) (*simplejson.Json, error) {
	reqObj := rpcRequest{Method: method, Params: params}
	reqBytes, err := json.Marshal(reqObj)
	if err != nil {
		return nil, err
	}
	url := "http://localhost:8080/api"
	fmt.Printf("req body: %s\n", string(reqBytes))
	httpRes, err := http.Post(url, "application/json", bytes.NewReader(reqBytes))
	if err != nil {
		return nil, err
	}
	if httpRes.StatusCode != 200 {
		return nil, errors.New("rpc error of " + httpRes.Status)
	}
	resJSON, err := simplejson.NewFromReader(httpRes.Body)
	if err != nil {
		return nil, err
	}
	return resJSON.Get("result"), nil
}

func testContractPath(contractPath string) string {
	dir, err := os.Getwd()
	if err != nil {
		panic(err)
	}
	dirAbs, err := filepath.Abs(dir)
	if err != nil {
		panic(err)
	}
	uvmDir := filepath.Dir(filepath.Dir(filepath.Dir(dirAbs)))
	return filepath.Join(uvmDir, "test", "test_contracts", contractPath)
}

func TestSimpleChainMintAndTransfer(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()

	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"
	caller2 := "SPLtest2"
	res, err = simpleChainRPC("mint", caller1, 0, 1000)
	assert.True(t, err == nil)
	res, err = simpleChainRPC("mint", caller2, 0, 100)
	assert.True(t, err == nil)
	simpleChainRPC("generate_block")

	res, err = simpleChainRPC("get_account_balances", caller1)
	assert.True(t, err == nil)
	fmt.Printf("%v\n", res)
	assert.True(t, res.GetIndex(0).GetIndex(1).MustInt() == 1000)
	res, err = simpleChainRPC("get_account_balances", caller2)
	assert.True(t, err == nil)
	fmt.Printf("%v\n", res)
	assert.True(t, res.GetIndex(0).GetIndex(1).MustInt() == 100)

	res, err = simpleChainRPC("transfer", caller1, caller2, 0, 200)
	assert.True(t, err == nil)
	simpleChainRPC("generate_block")

	res, err = simpleChainRPC("get_account_balances", caller1)
	assert.True(t, err == nil)
	fmt.Printf("%v\n", res)
	assert.True(t, res.GetIndex(0).GetIndex(1).MustInt() == 800)
	res, err = simpleChainRPC("get_account_balances", caller2)
	assert.True(t, err == nil)
	fmt.Printf("%v\n", res)
	assert.True(t, res.GetIndex(0).GetIndex(1).MustInt() == 300)
}

func TestSimpleChainTokenContract(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()

	var res *simplejson.Json
	var err error
	res, err = simpleChainRPC("get_chain_state")
	assert.True(t, err == nil)
	fmt.Printf("head_block_num: %d\n", res.Get("head_block_num").MustInt())
	caller1 := "SPLtest1"
	caller2 := "SPLtest2"
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("newtoken.gpc"), 50000, 10)
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_contract_info", contract1Addr)
	assert.True(t, err == nil)
	assert.True(t, res.Get("owner_address").MustString() == caller1 && res.Get("contract_address").MustString() == contract1Addr)
	simpleChainRPC("invoke_contract", caller1, contract1Addr, "init_token", []string{"test,TEST,10000,100"}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_storage", contract1Addr, "state")
	assert.True(t, res.MustString() == "COMMON")
	fmt.Printf("state after init_token of contract1 is: %s\n", res.MustString())
	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "balanceOf", []string{caller1}, 0, 0)
	assert.True(t, err == nil)
	fmt.Printf("caller1 balance: %s\n", res.Get("api_result").MustString())
	assert.True(t, res.Get("api_result").MustString() == "10000")
	simpleChainRPC("invoke_contract", caller1, contract1Addr, "transfer", []string{caller2 + "," + strconv.Itoa(100)}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "balanceOf", []string{caller2}, 0, 0)
	assert.True(t, err == nil)
	fmt.Printf("caller2 balance: %s\n", res.Get("api_result").MustString())
	assert.True(t, res.Get("api_result").MustString() == "100")
}

func TestSimpleChainContractCallContract(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()

	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"
	caller2 := "SPLtest2"

	// init token contract
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("token.gpc"), 50000, 10)
	assert.True(t, err == nil)
	tokenContractAddr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", tokenContractAddr)
	simpleChainRPC("generate_block")
	simpleChainRPC("invoke_contract", caller1, tokenContractAddr, "init_token", []string{"test,TEST,10000,100"}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")

	_, compileErr := execCommand(uvmCompilerPath, "-g", "../../test_contracts/token_caller.lua")
	assert.Equal(t, compileErr, "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("token_caller.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")

	// transfer token from owner to contract
	simpleChainRPC("invoke_contract", caller1, tokenContractAddr, "transfer", []string{contract1Addr + ",100"}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")

	// transfer token from contract to caller2
	simpleChainRPC("invoke_contract", caller1, contract1Addr, "transfer", []string{tokenContractAddr + "," + caller2 + ",30"}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")

	// check caller1, caller2, and contract's balance of token
	res, err = simpleChainRPC("invoke_contract_offline", caller1, tokenContractAddr, "balanceOf", []string{caller1}, 0, 0)
	assert.True(t, err == nil)
	caller1Balance := res.Get("api_result").MustString()
	fmt.Printf("caller1 balance: %s\n", caller1Balance)
	assert.True(t, caller1Balance == "9900")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, tokenContractAddr, "balanceOf", []string{caller2}, 0, 0)
	assert.True(t, err == nil)
	caller2Balance := res.Get("api_result").MustString()
	fmt.Printf("caller2 balance: %s\n", caller2Balance)
	assert.True(t, caller2Balance == "30")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, tokenContractAddr, "balanceOf", []string{contract1Addr}, 0, 0)
	assert.True(t, err == nil)
	contract1Balance := res.Get("api_result").MustString()
	fmt.Printf("contract1 balance: %s\n", contract1Balance)
	assert.True(t, contract1Balance == "70")
}

func TestSimpleChainContractChangeOtherContractProperties(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()

	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"

	fmt.Printf("simple_contract_path: %s\n", testContractPath("simple_contract.lua"))
	_, compile1Err := execCommand(uvmCompilerPath, "-g", testContractPath("simple_contract.lua"))
	assert.Equal(t, compile1Err, "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("simple_contract.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	simpleContractAddr := res.Get("contract_address").MustString()
	fmt.Printf("simple_contract address: %s\n", simpleContractAddr)
	simpleChainRPC("generate_block")

	_, compile2Err := execCommand(uvmCompilerPath, "-g", testContractPath("change_other_contract_property_contract.lua"))
	assert.Equal(t, compile2Err, "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("change_other_contract_property_contract.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	contract2Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract2Addr)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract2Addr, "start", []string{simpleContractAddr}, 0, 0)

	fmt.Printf("%v\n", res)
	assert.True(t, res.Get("exec_succeed").MustBool() == false)

}
