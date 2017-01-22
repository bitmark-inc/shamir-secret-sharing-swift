//: Playground - noun: a place where people can play

import ShamirSecretSharing

var str = "123456789012345678901234567890123456789"

let result = ShamirSecretSharing.split(secret: str, threshold: 3, numberOfShare: 5)

let combine = ShamirSecretSharing.combine(shares: [result![0], result![2], result![4]])
