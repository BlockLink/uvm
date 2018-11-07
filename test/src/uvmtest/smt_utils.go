package main

import (
	"crypto/ecdsa"
	"encoding/hex"
	"math/big"

	"github.com/zoowii/ecdsatools"
	gosmt "github.com/zoowii/go_sparse_merkle_tree"
	// "github.com/ethereum/go-ethereum/crypto"
)

func HexToBytes(hexStr string) ([]byte, error) {
	return hex.DecodeString(hexStr)
}

func HexToBigInt(hexStr string) *big.Int {
	n := new(big.Int)
	n, ok := n.SetString(hexStr, 16)
	if !ok {
		return nil
	}
	return n
}

func CreateSMTByTxHashes(txs map[gosmt.Uint256]gosmt.TreeItemHashValue) *gosmt.SMT {
	smt := gosmt.NewSMT(txs, gosmt.DefaultSMTDepth)
	return smt
}

func CreateSMTBySingleTxTree(slotHex string, txHash []byte) *gosmt.SMT {
	blockTxs := make(map[gosmt.Uint256]gosmt.TreeItemHashValue)
	coinSlotInt := HexToBigInt(slotHex)
	blockTxs[coinSlotInt] = txHash
	smt := gosmt.NewSMT(blockTxs, gosmt.DefaultSMTDepth)
	return smt
}

func BigIntToBytes32(value *big.Int) [32]byte {
	var result [32]byte
	src := value.Bytes()[0:len(result)]
	for i := 0; i < len(result); i++ {
		if i < (len(result) - len(src)) {
			result[i] = 0
		} else {
			result[i] = src[i-len(result)+len(src)]
		}
	}
	return result
}

func TrySignRecoverableSignature(privateKey *ecdsa.PrivateKey, digestHash []byte) (string, error) {
	signature, err := ecdsatools.SignSignatureRecoverable(privateKey, ecdsatools.ToHashDigest(digestHash))
	if err != nil {
		return "", err
	}
	signatureHex := ecdsatools.BytesToHex(signature[:])
	return signatureHex, nil
}

func EthSignatureToFcSignature(signatureHex string) string {
	signatureBytes, err := ecdsatools.HexToBytes(signatureHex)
	if err != nil {
		return ""
	}
	fcSig := ecdsatools.EthSignatureToFcFormat(ecdsatools.ToCompactSignature(signatureBytes))
	return ecdsatools.BytesToHexWithoutPrefix(fcSig[:])
}
