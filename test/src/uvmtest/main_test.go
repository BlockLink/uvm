package main

import (
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
	"rsc.io/quote"
)

func findUvmSinglePath() (string, error) {
	dir, err := os.Getwd()
	if err != nil {
		return "", err
	}
	dir_abs, err := filepath.Abs(dir)
	if err != nil {
		return "", err
	}
	uvm_dir := filepath.Dir(filepath.Dir(filepath.Dir(dir_abs)))
	if runtime.GOOS == "windows" {
		return filepath.Join(uvm_dir, "x64", "Debug", "uvm_single.exe"), nil
	} else {
		return filepath.Join(uvm_dir, "uvm"), nil
	}
}

var uvmSinglePath, find_uvm_err = findUvmSinglePath()

func execCommand(program string, args ...string) (exec_out string, exec_err string) {
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

func TestUvmPath(t *testing.T) {
	fmt.Println(quote.Hello())
	if find_uvm_err != nil {
		panic(find_uvm_err)
	}

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
	// TODO: change other contract's property should throw error
}