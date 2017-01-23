//: Playground - noun: a place where people can play

import ShamirSecretSharing

var str = "123456789012345678901234567890123456789"

let result = ShamirSecretSharing.split(secret: str, threshold: 3, numberOfShare: 5)

//let combine = ShamirSecretSharing.combine(shares: [result![0], result![2], result![4]])

//let keys = [
//    "1-301a132b136b1d181d101d713e37680e25675c7f7b2d3720713e115b4e393a0b0f2e0b631f481b",
////    "2-1e01c81d8cd54281d4e522935244360b4cd54f5432f5940b9a51960b7481486b146f9f0bf44fbe",
////    "3-1f29e802aa8868a1f0c50ed05f476b335e8a2a1b78ea901fde59b068038843522875a15edc3f9a",
//    "3-dadb293893945bad74cf3aed1c0ed679bdac2612695bc117f626c2882cdf7f2d003c3b13ff8843",
//    "5-dbf30927b5c9718d50ef16ae110d8b41aff3435d2344c503b22ee4eb5bd674143c260546d7f875"
//]
//
//let combineTest = ShamirSecretSharing.combine(shares: keys)
