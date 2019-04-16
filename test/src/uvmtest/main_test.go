package main

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
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
	"time"
	"math/rand"
	"github.com/bitly/go-simplejson"
	"github.com/stretchr/testify/assert"
	"github.com/zoowii/ecdsatools"
	gosmt "github.com/zoowii/go_sparse_merkle_tree"
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
	}
	return filepath.Join(uvmDir, "uvm_single_exec")
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
	}
	return filepath.Join(uvmDir, "test", "uvm_compiler")
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
	}
	return filepath.Join(uvmDir, "simplechain_runner")
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
	}
	return filepath.Join(uvmDir, "test", "uvm_ass")
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
	var outb, errb bytes.Buffer
	cmd.Stdin = os.Stdin
	cmd.Stdout = &outb
	cmd.Stderr = &errb
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
	assert.True(t, strings.Contains(out, "123	4.56	abc	true	[123,4.560000,\"abc\",true]"))
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

func TestContractImport(t *testing.T) {
	// TODO
}

func TestSafeMath(t *testing.T) {
	execCommand(uvmCompilerPath, "../../test_safemath.lua")
	out, err := execCommand(uvmSinglePath, "../../test_safemath.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a=	{"hex":"7b","type":"bigint"}`))
	assert.True(t, strings.Contains(out, `b=	{"hex":"1c8","type":"bigint"}`))
	assert.True(t, strings.Contains(out, `hex(a)=	7b`))
	assert.True(t, strings.Contains(out, `int(a)=	123`))
	assert.True(t, strings.Contains(out, `str(a)=	123`))
	assert.True(t, strings.Contains(out, `c	579`))
	assert.True(t, strings.Contains(out, `d	-333`))
	assert.True(t, strings.Contains(out, `e	56088`))
	assert.True(t, strings.Contains(out, `f	3`))
	assert.True(t, strings.Contains(out, `g	792594609605189126649`))
	assert.True(t, strings.Contains(out, `h	3333333333333333333333333333333333332`))
	assert.True(t, strings.Contains(out, `j	0`))
	assert.True(t, strings.Contains(out, `try parse a1 is:	nil`))
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
	assert.True(t, strings.Contains(out, `callFrameStackSize: 	1`))
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
	assert.True(t, strings.Contains(out, `a4=	2009-02-13 23:31:30`))
	assert.True(t, strings.Contains(out, `a5=	2009-02-14 23:31:30`))
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

func TestSignatureRecover(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_signature_recover.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_signature_recover.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `a1=	024a75f222e1e1bc39c9b47d826ca5518655563c506c1e19aa13603e4bc8dcc591`))
}

func TestCborEncodeDecode(t *testing.T) {
	execCommand(uvmCompilerPath, "../../tests_lua/test_cbor.lua")
	out, err := execCommand(uvmSinglePath, "../../tests_lua/test_cbor.lua.out")
	fmt.Println(out)
	assert.Equal(t, err, "")
	assert.True(t, strings.Contains(out, `encoded1: 	8B187B6362617219014119014163666F6FF5F4F681187BA0A263616765126568656C6C6F65776F726C64`))
	assert.True(t, strings.Contains(out, `encoded2: 	187B`))
	assert.True(t, strings.Contains(out, `encoded3: 	6568656C6C6F`))
	assert.True(t, strings.Contains(out, `encoded4: 	F5`))
	assert.True(t, strings.Contains(out, `encoded5: 	1B000000174876E800`))
	assert.True(t, strings.Contains(out, `encoded6: 	F6`))
	assert.True(t, strings.Contains(out, `encoded7: 	80`))
	assert.True(t, strings.Contains(out, `[123,"bar",321,321,"foo",true,false,null,[123],[],{"age":18,"hello":"world"}]	 = 	[123,"bar",321,321,"foo",true,false,null,[123],[],{"age":18,"hello":"world"}]`))
	assert.True(t, strings.Contains(out, `123	 = 	123`))
	assert.True(t, strings.Contains(out, `hello	 = 	hello`))
	assert.True(t, strings.Contains(out, `true	 = 	true`))
	assert.True(t, strings.Contains(out, `100000000000	 = 	100000000000`))
	assert.True(t, strings.Contains(out, `nil	 = 	nil`))
	assert.True(t, strings.Contains(out, `[]	 = 	[]`))
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
	if method == "get_tx_receipt" {
		resJSONBytes, err := json.Marshal(resJSON)
		if err != nil {
			return nil, err
		}
		println("get_tx_receipt res: ", string(resJSONBytes))
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

func getAccountBalanceOfAssetID(caller string, assetID int) (int, error) {
	res, err := simpleChainRPC("get_account_balances", caller)
	if err != nil {
		return 0, err
	}
	return res.GetIndex(assetID).GetIndex(1).MustInt(), nil
}

func TestSimpleChainMintAndTransfer(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)

	var res *simplejson.Json
	var err error
	var balanceValueRes int
	caller1 := "SPLtest1"
	caller2 := "SPLtest2"
	res, err = simpleChainRPC("mint", caller1, 0, 1000)
	println(res)
	assert.True(t, err == nil)
	res, err = simpleChainRPC("mint", caller2, 0, 100)
	assert.True(t, err == nil)
	simpleChainRPC("generate_block")

	balanceValueRes, err = getAccountBalanceOfAssetID(caller1, 0)
	assert.True(t, balanceValueRes == 1000)
	balanceValueRes, err = getAccountBalanceOfAssetID(caller2, 0)
	assert.True(t, balanceValueRes == 100)

	res, err = simpleChainRPC("transfer", caller1, caller2, 0, 200)
	assert.True(t, err == nil)
	simpleChainRPC("generate_block")

	balance1, _ := getAccountBalanceOfAssetID(caller1, 0)
	assert.True(t, balance1 == 800)
	balance2, _ := getAccountBalanceOfAssetID(caller2, 0)
	assert.True(t, balance2 == 300)
}

type TxEvent struct {
	EventName string `json:"event_name"`
	EventArg  string `json:"event_arg"`
}

func getTxReceiptEvents(txid string) (result []*TxEvent, err error) {
	result = make([]*TxEvent, 0)
	res, err := simpleChainRPC("get_tx_receipt", txid)
	if err != nil {
		return
	}
	eventsArrayJson := res.Get("events")
	eventsBytes, err := eventsArrayJson.Encode()
	if err != nil {
		return
	}
	err = json.Unmarshal(eventsBytes, &result)
	if err != nil {
		return
	}
	return
}

func testTokenContractInSimplechain(t *testing.T, contract1Addr string) {
	caller1 := "SPLtest1"
	caller2 := "SPLtest2"
	caller3 := "SPLtest3"
	var res *simplejson.Json
	var err error
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
	res, err = simpleChainRPC("invoke_contract", caller1, contract1Addr, "transfer", []string{caller2 + "," + strconv.Itoa(100)}, 0, 0, 50000, 10)
	assert.True(t, err == nil)
	transferTxid := res.Get("txid").MustString()
	println("transfer txid: ", transferTxid)
	simpleChainRPC("generate_block")
	// check emited event
	res, err = simpleChainRPC("get_tx_receipt", transferTxid)
	assert.True(t, err == nil)
	transferEvents, err := getTxReceiptEvents(transferTxid)
	assert.True(t, err == nil)
	assert.True(t, len(transferEvents) == 1)
	transferEvent := transferEvents[0]
	assert.True(t, transferEvent.EventName == "Transfer")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "balanceOf", []string{caller2}, 0, 0)
	assert.True(t, err == nil)
	fmt.Printf("caller2 balance: %s\n", res.Get("api_result").MustString())
	assert.True(t, res.Get("api_result").MustString() == "100")

	// test approve and transferFrom
	simpleChainRPC("invoke_contract", caller1, contract1Addr, "approve", []string{caller2 + "," + strconv.Itoa(30)}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "approvedBalanceFrom", []string{caller2 + "," + caller1}, 0, 0)
	assert.True(t, err == nil)
	fmt.Printf("caller2 approvedBalanceFrom caller1 balance: %s\n", res.Get("api_result").MustString())
	assert.True(t, res.Get("api_result").MustString() == "30")

	simpleChainRPC("invoke_contract", caller2, contract1Addr, "transferFrom", []string{caller1 + "," + caller3 + "," + strconv.Itoa(12)}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "balanceOf", []string{caller3}, 0, 0)
	assert.True(t, err == nil)
	fmt.Printf("caller2 balance: %s\n", res.Get("api_result").MustString())
	assert.True(t, res.Get("api_result").MustString() == "12")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "approvedBalanceFrom", []string{caller2 + "," + caller1}, 0, 0)
	assert.True(t, err == nil)
	fmt.Printf("caller2 approvedBalanceFrom caller1 balance: %s\n", res.Get("api_result").MustString())
	assert.True(t, res.Get("api_result").MustString() == "18")

}

func TestSimpleChainTokenContract(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	time.Sleep(1 * time.Second)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)

	var res *simplejson.Json
	var err error
	res, err = simpleChainRPC("get_chain_state")
	if err != nil {
		fmt.Printf("error: %s\n", err.Error())
	}
	assert.True(t, err == nil)
	fmt.Printf("head_block_num: %d\n", res.Get("head_block_num").MustInt())
	caller1 := "SPLtest1"
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("token.gpc"), 50000, 10)
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")
	testTokenContractInSimplechain(t, contract1Addr)
}

func depositToPlasmaContract(caller string, plasmaContractAddress string, amount int, assetID int) (txID string, coinSlotHex string, err error) {
	var res *simplejson.Json
	res, err = simpleChainRPC("invoke_contract", caller, plasmaContractAddress, "on_deposit_asset", []string{""}, assetID, amount, 50000, 10)
	if err != nil {
		return
	}
	txID = res.Get("txid").MustString()
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_tx_receipt", txID)
	if err != nil {
		println("error: ", err.Error())
		return
	}
	deposit1TxReceipt := res
	eventsArray, err := deposit1TxReceipt.Get("events").Array()
	if err != nil {
		return
	}
	if len(eventsArray) < 1 {
		err = errors.New("not enough events of deposit to plasma contract tx")
		return
	}
	coin1EventArg := deposit1TxReceipt.Get("events").GetIndex(0).Get("event_arg").MustString()
	coin1EventArgJSON, _ := simplejson.NewJson([]byte(coin1EventArg))
	coinSlotHex = coin1EventArgJSON.Get("slot").MustString()
	return
}

func createEmptyPlasmaCoin(caller string, plasmaContractAddress string) (txID string, coinSlotHex string, err error) {
	var res *simplejson.Json
	res, err = simpleChainRPC("invoke_contract", caller, plasmaContractAddress, "create_empty_coin", []string{"0"}, 0, 0, 50000, 10)
	if err != nil {
		return
	}
	txID = res.Get("txid").MustString()
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_tx_receipt", txID)
	if err != nil {
		return
	}
	emptyCoinTxReceipt := res
	coinEventArg := emptyCoinTxReceipt.Get("events").GetIndex(0).Get("event_arg").MustString()
	coinEventArgJSON, err := simplejson.NewJson([]byte(coinEventArg))
	if err != nil {
		return
	}
	coinSlotHex = coinEventArgJSON.Get("slot").MustString()
	return
}

type PlasmaCoin struct {
	Slot         string
	Balance      int
	Denomination int
}

func getPlasmaCoin(caller string, plasmaContractAddress string, coinSlotHex string) (coin *PlasmaCoin, err error) {
	var res *simplejson.Json
	res, err = simpleChainRPC("invoke_contract_offline", caller, plasmaContractAddress, "get_plasma_coin", []string{coinSlotHex}, 0, 0)
	if err != nil {
		return
	}
	coinCreatedStr := res.Get("api_result").MustString()
	coinCreated, _ := simplejson.NewJson([]byte(coinCreatedStr))
	coin = new(PlasmaCoin)
	coin.Slot = coinSlotHex
	coin.Balance = coinCreated.Get("balance").MustInt()
	coin.Denomination = coinCreated.Get("denomination").MustInt()
	return
}

func provideLiquidityToPlasmaCoin(caller string, plasmaContractAddress string, coinSlotHex string, amount int) {
	simpleChainRPC("invoke_contract", caller, plasmaContractAddress, "provide_liquidity", []string{fmt.Sprintf("%s,%d", coinSlotHex, amount)}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")
}

func submitBlockToPlasma(caller string, plasmaContractAddress string, childBlockRootHex string) {
	simpleChainRPC("invoke_contract", caller, plasmaContractAddress, "submit_block", []string{childBlockRootHex}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")
}

func checkMembershipInPlasma(caller string, plasmaContractAddress string, txHashHex string, childBlockRoot string, coinSlotHex string, txProofHex string) bool {
	coinSlotInt := HexToBigInt(coinSlotHex)
	res, err := simpleChainRPC("invoke_contract_offline", caller, plasmaContractAddress, "checkMembership", []string{txHashHex + "," + childBlockRoot + "," + coinSlotInt.String() + "," + txProofHex}, 0, 0)
	if err != nil {
		return false
	}
	return res.Get("api_result").MustString() == "true"
}

func TestPlasmaRootChain(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)
	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"
	compileOut, compileErr := execCommand(uvmCompilerPath, "-g", testContractPath("sparse_merkle_tree.lua"))
	fmt.Printf("compile out: %s\n", compileOut)
	assert.True(t, compileErr == "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("sparse_merkle_tree.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	smtContractAddress := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", smtContractAddress)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_contract_info", smtContractAddress)
	assert.True(t, err == nil)
	assert.True(t, res.Get("owner_address").MustString() == caller1 && res.Get("contract_address").MustString() == smtContractAddress)

	compileOut2, compileErr2 := execCommand(uvmCompilerPath, "-g", testContractPath("plasma_root_chain.lua"))
	fmt.Printf("compile out: %s\n", compileOut2)
	assert.True(t, compileErr2 == "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("plasma_root_chain.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	plasmaContractAddress := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", plasmaContractAddress)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_contract_info", plasmaContractAddress)
	assert.True(t, err == nil)
	assert.True(t, res.Get("owner_address").MustString() == caller1 && res.Get("contract_address").MustString() == plasmaContractAddress)

	compileOut3, compileErr3 := execCommand(uvmCompilerPath, "-g", testContractPath("validator_manager_contract.lua"))
	fmt.Printf("compile out: %s\n", compileOut3)
	assert.True(t, compileErr3 == "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("validator_manager_contract.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	vmcContractAddress := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", vmcContractAddress)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_contract_info", vmcContractAddress)
	assert.True(t, err == nil)
	assert.True(t, res.Get("owner_address").MustString() == caller1 && res.Get("contract_address").MustString() == vmcContractAddress)

	simpleChainRPC("invoke_contract", caller1, plasmaContractAddress, "set_config", []string{caller1 + "," + vmcContractAddress + "," + smtContractAddress + ",1000"}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, plasmaContractAddress, "get_config", []string{" "}, 0, 0)
	assert.True(t, err == nil)
	configJSONStr := res.Get("api_result").MustString()
	fmt.Printf("plasma root chain config: %s\n", configJSONStr)

	privateKey, err := ecdsatools.GenerateKey()
	if err != nil {
		panic(err)
	}
	pubKey := ecdsatools.PubKeyFromPrivateKey(privateKey)
	pubKeyData := ecdsatools.CompactPubKeyToBytes(pubKey)
	fmt.Println("privateKey: ", ecdsatools.BytesToHexWithoutPrefix(ecdsatools.PrivateKeyToBytes(privateKey)))
	fmt.Println("pubKey: ", ecdsatools.BytesToHexWithoutPrefix(pubKeyData[:]))

	// deposit
	simpleChainRPC("mint", caller1, 0, 100000)
	simpleChainRPC("generate_block")

	deposit1TxID, coin1Slot, err := depositToPlasmaContract(caller1, plasmaContractAddress, 50000, 0)
	assert.True(t, err == nil)
	println("deposit1TxID: ", deposit1TxID)
	balance1, _ := getAccountBalanceOfAssetID(plasmaContractAddress, 0)
	fmt.Printf("plasma contract balance: %d\n", balance1)
	fmt.Println("coin1 slot: ", coin1Slot)

	coin1, err := getPlasmaCoin(caller1, plasmaContractAddress, coin1Slot)
	println("coin1 after created: ", coin1)
	assert.True(t, coin1.Denomination == 50000)
	assert.True(t, coin1.Balance == 50000)

	// create empty coin
	emptyCoinTxID, coin2Slot, err := createEmptyPlasmaCoin(caller1, plasmaContractAddress)
	assert.True(t, err == nil)
	println("emptyCoinTxID: ", emptyCoinTxID)
	println("coin2 slot: ", coin2Slot)

	coin2, err := getPlasmaCoin(caller1, plasmaContractAddress, coin2Slot)
	assert.True(t, err == nil)
	println("coin2 after created: ", coin2)
	assert.True(t, coin2.Denomination == 0)
	assert.True(t, coin2.Balance == 0)

	// provide liquidity
	provideLiquidityToPlasmaCoin(caller1, plasmaContractAddress, coin2Slot, 10000)
	coin2AfterLiquidity, err := getPlasmaCoin(caller1, plasmaContractAddress, coin2Slot)
	assert.True(t, err == nil)
	println("coin2 after provided liquidity: ", coin2AfterLiquidity)
	assert.True(t, coin2AfterLiquidity.Denomination == 10000)
	assert.True(t, coin2AfterLiquidity.Balance == 0)

	// make txs
	tx1 := CreateChildChainTx("ADDR_02e9e0da80e519c937294e7d9ed26786e25a6f6adbdf9952de8e9b2c68b0644c6c",
		"02e9e0da80e519c937294e7d9ed26786e25a6f6adbdf9952de8e9b2c68b0644c6c", coin1Slot, 100, 0)
	tx1Bytes, err := EncodeChildChainTx(tx1)
	if err != nil {
		assert.True(t, false)
		println("Error decoding ", err.Error())
	}
	fmt.Printf("tx1 Hex = %x\n", tx1Bytes)
	tx1Hash := sha256.Sum256(tx1Bytes)
	fmt.Printf("tx1 hash: %x\n", tx1Hash)
	tx1HashHex := fmt.Sprintf("%x", tx1Hash)
	tx1["sigHash"] = "00" + tx1HashHex // use "00" + tx hash as sigHash(signature = sig(sigHash, privateKey))
	tx1WithSigHashBytes, err := EncodeChildChainTx(tx1)
	if err != nil {
		assert.True(t, false)
		println("Error decoding ", err.Error())
	}
	tx1WithSigHashHex := fmt.Sprintf("%x", tx1WithSigHashBytes)
	fmt.Println("tx1 with sig hash: ", tx1WithSigHashHex)
	tx1Sig := "20fa8cd18cf7223840c6cf823b66c6200e5b18fd5eec86f094742a6930a6c4d0ba316ee3f08f0a38697cf6454f5f3ca4fe72ee9d9d62306f744d934d84c995b702"
	fmt.Println("tx1 sig: ", tx1Sig)
	// submitBlock
	coin1SlotInt := HexToBigInt(coin1Slot)
	fmt.Println("coin1SlotInt: ", coin1SlotInt)
	smt1 := CreateSMTBySingleTxTree(coin1Slot, tx1Hash[:])
	blockTxsMerkleRoot1 := gosmt.BytesToHex(smt1.Root.Bytes())
	fmt.Println("blockTxsMerkleRoot1: ", blockTxsMerkleRoot1)
	tx1ProofHex := gosmt.BytesToHex(smt1.CreateMerkleProof(coin1SlotInt))
	fmt.Println("tx1 proof: ", tx1ProofHex)

	submitBlockToPlasma(caller1, plasmaContractAddress, blockTxsMerkleRoot1)

	res, err = simpleChainRPC("invoke_contract_offline", caller1, plasmaContractAddress, "get_config", []string{" "}, 0, 0)
	assert.True(t, err == nil)
	configJSONStr = res.Get("api_result").MustString()
	configJSON, _ := simplejson.NewJson([]byte(configJSONStr))
	println("config after submit block: ", configJSONStr)
	assert.True(t, configJSON.Get("currentBlockNum").MustInt() == 1000)
	// check tx membership
	tx1MemberShip := checkMembershipInPlasma(caller1, plasmaContractAddress, tx1HashHex, blockTxsMerkleRoot1, coin1Slot, tx1ProofHex)
	// res, err = simpleChainRPC("invoke_contract_offline", caller1, plasmaContractAddress, "checkMembership", []string{tx1HashHex + "," + blockTxsMerkleRoot1 + "," + coin1SlotInt.String() + "," + tx1ProofHex}, 0, 0)
	// tx1MemberShip := res.Get("api_result").MustString() == "true"
	fmt.Println("tx1MemberShip: ", tx1MemberShip)
	assert.True(t, tx1MemberShip)

	// query childChain blocks
	res, err = simpleChainRPC("invoke_contract_offline", caller1, plasmaContractAddress, "getChildBlockByHeight", []string{"1000"}, 0, 0)
	block1StrGot := res.Get("api_result").MustString()
	fmt.Println("block1 got: ", block1StrGot)
	block1Got, _ := simplejson.NewJson([]byte(block1StrGot))
	assert.True(t, block1Got.Get("root").MustString() == blockTxsMerkleRoot1)

	// normal exit just after deposit
	coin1SlotBytes, decodeHexErr := hex.DecodeString(coin1Slot)
	assert.True(t, decodeHexErr == nil)

	// tx: {ownerPubKey: string, owner: string, sigHash: string, hash: string, slot: string, balance: int, prevBlock: int}
	var coin1DepositTx = make(map[string]interface{})
	coin1DepositTx["ownerPubKey"] = ecdsatools.BytesToHexWithoutPrefix(pubKeyData[:])
	coin1DepositTx["owner"] = caller1
	coin1DepositTx["slot"] = string(coin1SlotBytes)
	coin1DepositTx["balance"] = 50000
	coin1DepositTx["prevBlock"] = 0
	coin1DepositTxBytes, err := EncodeChildChainTx(coin1DepositTx)
	if err != nil {
		assert.True(t, false)
		println("Error decoding ", err.Error())
	}
	coin1DepositTxHex := fmt.Sprintf("%x", coin1DepositTxBytes)
	coin1DepositTxHash, err := ComputeChildChainDepositTxHash(coin1Slot) // sha256.Sum256(coin1DepositTxBytes) // deposit tx's hash is coin's slot
	if err != nil {
		assert.True(t, false)
		println("Error decoding ", err.Error())
	}
	coin1DepositTx["hash"] = string(coin1DepositTxHash[:])
	coin1DepositTx["sigHash"] = string(coin1DepositTxHash[:])
	coin1DepositTxBytesWithHash, err := EncodeChildChainTx(coin1DepositTx)
	if err != nil {
		assert.True(t, false)
		println("Error decoding ", err.Error())
	}
	coin1DepositTxHexWithHash := fmt.Sprintf("%x", coin1DepositTxBytesWithHash)
	fmt.Println("coin1DepositTxHex: ", coin1DepositTxHex)
	fmt.Println("coin1DepositTxHash: ", ecdsatools.BytesToHexWithoutPrefix(coin1DepositTxHash[:]))
	coin1DepositBlockSMT := CreateSMTBySingleTxTree(coin1Slot, coin1DepositTxHash[:])
	coin1DepositTxProofHex := ecdsatools.BytesToHexWithoutPrefix(coin1DepositBlockSMT.CreateMerkleProof(coin1SlotInt))
	fmt.Println("coin1DepositTxProofHex: ", coin1DepositTxProofHex)
	coin1DepositTxSignatureHex, _ := TrySignRecoverableSignature(privateKey, coin1DepositTxHash[:])
	coin1DepositTxSignatureHex = coin1DepositTxSignatureHex[2:]
	coin1DepositTxSignatureHex = EthSignatureToFcSignature(coin1DepositTxSignatureHex)
	fmt.Println("coin1DepositTxSignatureHex: ", coin1DepositTxSignatureHex)
	fmt.Println("pubKey: ", ecdsatools.BytesToHexWithoutPrefix(pubKeyData[:]))
	coin1DepositTxBlockNum := 1
	res, err = simpleChainRPC("invoke_contract", caller1, plasmaContractAddress, "startExit", []string{fmt.Sprintf("%s,%s,%s,%s,%s,%s,%d,%d", coin1Slot, "0", coin1DepositTxHexWithHash, "0", coin1DepositTxProofHex, coin1DepositTxSignatureHex, coin1DepositTxBlockNum, coin1DepositTxBlockNum)}, 0, 0, 50000, 10)
	exitTxID := res.Get("txid").MustString()
	fmt.Println("exit tx id: ", exitTxID, res)
	assert.True(t, res.Get("exec_succeed").MustBool())
	simpleChainRPC("generate_block")
	time.Sleep(time.Duration(7) * time.Second)
	simpleChainRPC("generate_block")
	// normal exit started
	res, err = simpleChainRPC("invoke_contract_offline", caller1, plasmaContractAddress, "getExit", []string{coin1Slot}, 0, 0)
	exit1 := res.Get("api_result").MustString()
	println("exit for slot1: ", exit1)

	exitorBalanceBeforeFinalize, _ := getAccountBalanceOfAssetID(caller1, 0)

	res, err = simpleChainRPC("invoke_contract", caller1, plasmaContractAddress, "finalizeExit", []string{coin1Slot}, 0, 0, 50000, 10)
	finalizeExit1Result := res.Get("api_result").MustString() == "true"
	println("finalizeExit1Result: ", finalizeExit1Result)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("invoke_contract", caller1, plasmaContractAddress, "withdraw", []string{coin1Slot}, 0, 0, 50000, 10)
	assert.True(t, res.Get("exec_succeed").MustBool())
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("invoke_contract_offline", caller1, plasmaContractAddress, "getExit", []string{coin1Slot}, 0, 0)
	exit1AfterFinalize := res.Get("api_result").MustString()
	println("exit1AfterFinalize: ", exit1AfterFinalize)
	assert.True(t, exit1AfterFinalize == "null")
	// check exitor's balance
	exitorBalanceAfterFinalize, _ := getAccountBalanceOfAssetID(caller1, 0)
	fmt.Println("exitorBalanceBeforeFinalize: ", exitorBalanceBeforeFinalize)
	fmt.Println("exitorBalanceAfterFinalize: ", exitorBalanceAfterFinalize)
	assert.True(t, (exitorBalanceBeforeFinalize+50000) == exitorBalanceAfterFinalize)

	// TODO: normal exit after child chain transfer

	// TODO: start normal exit
	// TODO: query exit
	// TODO: challenge normal exit
	// TODO: query challenge
	// TODO: check membership
	// TODO: respond challenge
	// TODO: start evil exit
	// TODO: challenge evil exit
	// TODO: withdraw
	// TODO: check operator's balance and withdrawer's balance
}

func TestSparseMerkleTreeContract(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)

	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"

	compileOut, compileErr := execCommand(uvmCompilerPath, "-g", testContractPath("sparse_merkle_tree.lua"))
	fmt.Printf("compile out: %s\n", compileOut)
	assert.True(t, compileErr == "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("sparse_merkle_tree.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	contractAddr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contractAddr)
	simpleChainRPC("generate_block")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, contractAddr, "verify", []string{"303,747833,9da6c64db4a74efca5fe3c6979c992ece8fa88660f1bf8e273508612f77d9fc3,0000000000000190d10d96f5d5d50f79d299bff2c49827b594ff484c7ee4dd40f7b4c4cedefa23b4bf5021f0261bd1a5c13ed23d622799a91b86ac09b6180ebc4d550863813f1241474dd6e0117dd1ed3effe5e35105716ec9ea8c926489094c34417d04dd51b30b"}, 0, 0)
	assert.True(t, err == nil)
	verifyResult := res.Get("api_result").MustString()
	fmt.Printf("verify result: %s\n", verifyResult)
	assert.True(t, verifyResult == "true")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, contractAddr, "verify", []string{"303,747833,9da6c64db4a74efca5fe3c6979c992ece8fa88660f1bf8e273508612f77d9fc3,aaaa"}, 0, 0)
	assert.True(t, err == nil)
	verifyResult2 := res.Get("api_result").MustString()
	fmt.Printf("verify error result: %s\n", verifyResult2)
	assert.True(t, verifyResult2 == "false")
}

func TestSimpleChainContractCallContract(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)

	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"
	caller2 := "SPLtest2"

	// init token contract
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("token.gpc"), 50000, 10)
	if err != nil {
		fmt.Printf("error: %s\n", err.Error())
	}
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
	time.Sleep(1 * time.Second)

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

func TestManyObjects(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)
	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"

	_, compileErr := execCommand(uvmCompilerPath, "-g", "../../test_contracts/test_many_objects.lua")
	assert.Equal(t, compileErr, "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("test_many_objects.lua.gpc"), 50000, 10)
	if err != nil {
		println(err.Error())
	}
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")

	res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "hello", []string{" "}, 0, 0)
	println(res)
	if err != nil {
		println("error: ", err.Error())
	}
}

func TestCallContractWithIdNumberStorage(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)
	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"

	_, compileErr := execCommand(uvmCompilerPath, "-g", "../../test_contracts/test_number_storage.lua")
	assert.Equal(t, compileErr, "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("test_number_storage.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")

}

func TestCallContractManyTimes(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)
	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"

	_, compileErr := execCommand(uvmCompilerPath, "-g", "../../test_contracts/test_simple_storage_change.lua")
	assert.Equal(t, compileErr, "")
	res, err = simpleChainRPC("create_contract_from_file", caller1, testContractPath("test_simple_storage_change.lua.gpc"), 50000, 10)
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")

	maxRunCount := 1
	for i := 0; i < maxRunCount; i++ {
		//simpleChainRPC("invoke_contract", caller1, contract1Addr, "update", []string{" "}, 0, 0, 50000, 10)
		//simpleChainRPC("generate_block")
		res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "query", []string{" "}, 0, 0)
		assert.True(t, err == nil)
		num := res.Get("api_result").MustString()
		println("num: ", num)
		time.Sleep(time.Duration(1) * time.Second)
	}
	println("TestCallContractManyTimes")
}

func TestNativeTokenContract(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)
	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"

	res, err = simpleChainRPC("create_native_contract", caller1, "token", 50000, 10)
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")

	testTokenContractInSimplechain(t, contract1Addr)
}


func makeOrder(t *testing.T, signer_prik string, purchaseAsset string,purchaseNum int,payAsset string,payNum int,ordertype string) map[string]string{
	var res *simplejson.Json
	var err error
	orderInfo := make(map[string]interface{})
	order := make(map[string]string)
	orderInfo["nonce"]= time.Now().Format("Jan _2 15:04:05.000")+strconv.Itoa(rand.Intn(1000))
	orderInfo["purchaseAsset"] = purchaseAsset
    orderInfo["purchaseNum"] = purchaseNum
    orderInfo["payAsset"] = payAsset
    orderInfo["payNum"] = payNum
    orderInfo["relayer"] = "SL_relayer"
    orderInfo["type"] = ordertype
	orderInfo["fee"] = "0.1"
	orderInfo["expiredAt"] = 9999999999
	orderInfo["version"] = 1
	
	str, err := json.Marshal(orderInfo)
	assert.True(t, err == nil)
    orderInfoStr := string(str)
	
	res, err = simpleChainRPC("sign_info", signer_prik, orderInfoStr)
	assert.True(t,err == nil)
	
	sig_hex := res.Get("sig_hex").MustString()
	id := res.Get("id").MustString()
	assert.True(t, sig_hex != "")
	order["orderInfo"] = orderInfoStr
	order["sig"] = sig_hex
	order["id"] = id
	return order
}


func makeFillOrder(t *testing.T, signer_prik string, purchaseAsset string,purchaseNum int,payAsset string,payNum int,getNum int,spentNum int,ordertype string) map[string]interface{}{
	order := makeOrder(t,signer_prik, purchaseAsset, purchaseNum, payAsset, payNum,ordertype)
	fillorder := make(map[string]interface{})
	fillorder["order"] = order
    fillorder["getNum"] = getNum
    fillorder["spentNum"] = spentNum
    return fillorder
}

func makeMatchedOrders(t *testing.T, user1 map[string]string, user2 map[string]string) map[string]interface{}{
	matchedOrders := make(map[string]interface{})
	takerGetNum := rand.Intn(100)
	takerSpentNum := rand.Intn(100)
	fillTakerOrder := makeFillOrder(t,user1["prik"],"HC",takerGetNum,"COIN",takerSpentNum,takerGetNum,takerSpentNum,"buy")
	fillMakerOrder := makeFillOrder(t,user2["prik"], "COIN", takerSpentNum, "HC", takerGetNum, takerSpentNum, takerGetNum,"sell")
	var orders [1]map[string]interface{}
	orders[0] = fillMakerOrder
	matchedOrders["fillTakerOrder"] = fillTakerOrder
    matchedOrders["fillMakerOrders"] = orders
    return matchedOrders
}




func test0xExchangeContractInSimplechain(t *testing.T, contract1Addr string) {
	var accounts = [...]map[string]string{{"prik": "71be40602648864966a0b7956edef37117d34b03e27dd35663eaaed58b5f0182",
                "addr": "SLNasXuVXWpwrxe38h6we8m51BCsP6eUibL7"},
               {"prik": "be16fe777ed4670e06db7a754da0b46aeea2ce267819274a447ed47e387e7700",
                "addr": "SLNhHWv5KS7e9Ab8LzXMdcSyLw6qUvVW2wWn"},
               {"prik": "7df8fe64fbbf347f5f18be1f8ec1a276cb64869261b5bcd9d58ba6e869dd4959",
                "addr": "SLNZgyTAxiDLLmsjwQ5hcVuS3Ks65wKrYSiE"},
               {"prik": "e4e34089cdeb11f470b56fd933d27b53846bdecf050bb97cbea1dd88fd276ddb",
                "addr": "SLNZmUEvsybDrjJC8aHpLaHmZsEKVwkmEHAa"},
               {"prik": "27e0bdfb1736d5e80379c07fe9b44a5f9d5a35d2ec92efa01092a85374dca2ea",
                "addr": "SLNbtMZ9zucVAFtyaNT35pbLJUscS8thCwaA"},
               {"prik": "ece4a800e8d096b09b3e777a06dd5d44dbd88ac3430730e58c4f93a8b51ad5ad",
                "addr": "SLNY8Q846p7PuGyBPmEm1G2ULU99HnyKmtGC"},
               {"prik": "96f19dfe4143635f710dba44fcaf76b375b1b8c959d5ea7a819e8af7d17c2ece",
                "addr": "SLNjBSKEAuQWsyWUuHB6ENURmgHbSXM9XQ6q"},
               {"prik": "c629091c5736ab646dfe0d073a52886d641b04c22cd3891dcc25fe02808da75b",
                "addr": "SLNawrikbH1iGhzFxGPrCiorLT9mGcYWwJYL"},
               {"prik": "f1777f455bf9a803598d3f15203cf3f8a244d481f7d016a59b7a3b1557d3b0a9",
                "addr": "SLNS3xj5rU2xZqd7UJta6ksW7fwMUEvvXNZA"},
               {"prik": "407e4822e927d393868f89775d998782472d0b6897d80ab06cb49f5713861b06",
                "addr": "SLNebuEoRLHPspvxjA2UtjSV9DKYd6evwMw3"}}
               
	//fmt.Printf("%s",accounts)
	caller1 := "SPLtest1"
	fmt.Printf("%s",caller1)
	//caller1 := "SPLtest1"
	caller2 := accounts[0]["addr"] 
	fmt.Printf("%s",caller2)
	//caller3 := "SPLtest3"
	var res *simplejson.Json
	var err error
	res, err = simpleChainRPC("get_contract_info", contract1Addr)
	assert.True(t, err == nil)
	assert.True(t, res.Get("owner_address").MustString() == caller1 && res.Get("contract_address").MustString() == contract1Addr)
	
	simpleChainRPC("invoke_contract", caller1, contract1Addr, "init_config", []string{"SL_feeReceiver"}, 0, 0, 50000, 10)
	simpleChainRPC("generate_block")
	res, err = simpleChainRPC("get_storage", contract1Addr, "state")
	assert.True(t, res.MustString() == "COMMON")
	fmt.Printf("state after init_config of contract1 is: %s\n", res.MustString())
	
	
	//deposit.......
	simpleChainRPC("add_asset", "HC", 8)
	simpleChainRPC("add_asset", "BTC", 8)
	simpleChainRPC("generate_block")
	for i := 0; i < len(accounts); i++ {
		num := 5000000
		user := accounts[i]["addr"]
		simpleChainRPC("mint", user,0,num+500)
		simpleChainRPC("invoke_contract", user, contract1Addr, "on_deposit_asset", []string{""}, 0, num, 50000, 10)
		simpleChainRPC("mint", user,1,num+500)
		simpleChainRPC("invoke_contract", user, contract1Addr, "on_deposit_asset", []string{""}, 1, num, 50000, 10)
		simpleChainRPC("generate_block")
		
		res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "balanceOf", []string{user+",COIN"}, 0, 0)
		assert.True(t, err == nil)
		assert.True(t, res.Get("api_result").MustString() == "5000000")
		res, err = simpleChainRPC("invoke_contract_offline", caller1, contract1Addr, "balanceOf", []string{user+",HC"}, 0, 0)
		assert.True(t, err == nil)
		assert.True(t, res.Get("api_result").MustString() == "5000000")
	}
	
	//test fillOrder
	matchedOrders := makeMatchedOrders(t,accounts[0],accounts[1])
	str, err := json.Marshal(matchedOrders)
	assert.True(t, err == nil)
    apiarg := string(str)
	fmt.Printf("fill Order apiarg: %s\n", apiarg)
	res, err = simpleChainRPC("invoke_contract", caller1, contract1Addr, "fillOrder", []string{apiarg}, 0, 0, 50000, 10)
	assert.True(t, err == nil)

	assert.True(t, res.Get("api_result").MustString() == "OK")
	fmt.Printf("fillOrder Result: %s\n",res.Get("api_result").MustString())
	simpleChainRPC("generate_block")
	
	//test cancel orders
	order1 := makeOrder(t,accounts[0]["prik"], "HC", 10, "COIN", 20,"buy")
    order2 := makeOrder(t,accounts[0]["prik"], "HC", 10, "COIN", 20,"sell")
	var orders [2]map[string]string
    orders[0] = order1
	orders[1] = order2
	str, err = json.Marshal(orders)
	assert.True(t, err == nil)
    apiargs := string(str)
	
    print(apiargs)
	res, err = simpleChainRPC("invoke_contract", accounts[0]["addr"], contract1Addr, "cancelOrders", []string{apiargs}, 0, 0, 50000, 10)
	assert.True(t, err == nil)
	str, err = json.Marshal(res)
	assert.True(t, err == nil)
	resstr := string(str)
	fmt.Printf("cancel res:%s\n",resstr)
	assert.True(t, strings.Contains(resstr,order1["id"]))
	assert.True(t, strings.Contains(resstr,order2["id"]))
    simpleChainRPC("generate_block")
}

func TestNativeExchangeContract(t *testing.T) {
	cmd := execCommandBackground(simpleChainPath)
	assert.True(t, cmd != nil)
	fmt.Printf("simplechain pid: %d\n", cmd.Process.Pid)
	defer func() {
		kill(cmd)
	}()
	time.Sleep(1 * time.Second)
	var res *simplejson.Json
	var err error
	caller1 := "SPLtest1"

	res, err = simpleChainRPC("create_native_contract", caller1, "exchange", 50000, 10)
	assert.True(t, err == nil)
	contract1Addr := res.Get("contract_address").MustString()
	fmt.Printf("contract address: %s\n", contract1Addr)
	simpleChainRPC("generate_block")

	test0xExchangeContractInSimplechain(t, contract1Addr)
}
