//: Playground - noun: a place where people can play

import ShamirSecretSharing

var str = "Hello, Bitmark"

let splitResult = ShamirSecretSharing.split(secret: str, shares: 5, threshold: 3)
let secret = ShamirSecretSharing.join(shares: splitResult)

let testJSClient = ["0203AA13CECD74B852011335B40330851A142CCA2BFADCEE98",
"0303AA54731355DE5FD55277DBA2D5BB355FF428C8E13EE20F",
"0403AA1FFE560C9CC19C1C3BF00E92A415E7EEEBD177548FB5"]

let secretfromJS = ShamirSecretSharing.join(shares: testJSClient)