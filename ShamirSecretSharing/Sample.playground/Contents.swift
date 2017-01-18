//: Playground - noun: a place where people can play

import ShamirSecretSharing

var str = "Hello, Bitmark"

let splitResult = ShamirSecretSharing.split(secret: str, shares: 5, threshold: 3)
let secret = ShamirSecretSharing.join(shares: splitResult)
