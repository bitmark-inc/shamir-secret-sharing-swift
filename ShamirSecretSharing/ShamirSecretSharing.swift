//
//  ShamirSecretSharing.swift
//  ShamirSecretSharing
//
//  Created by Anh Nguyen on 1/16/17.
//  Copyright © 2017 Bitmark. All rights reserved.
//

import Foundation
import CSSSS


public class ShamirSecretSharing {
    
    fileprivate static let maxDegree = 1024
    
    public static func split(secret: String, threshold t: Int, numberOfShare n: Int) -> [String]? {
        guard let randomByteString = generateRandomBytes(length: t * 128) else {
            return nil
        }
        
        let randomBytesPointer = randomByteString.utf8String
        guard let sharesCPointer = CSSSS.wrapped_allocate_shares(Int32(n)) else {
            return nil
        }
        
        let error = CSSSS.wrapped_split(sharesCPointer, secret.utf8String, 0, Int32(t), Int32(n), false, nil, false, randomBytesPointer, randomByteString.characters.count)

        if error.rawValue == 0 {
            var i = 0
            var shares = [String]()
            while sharesCPointer[i] != nil {
                let shareCPointer = sharesCPointer[i]!
                
                let share = String(cString: shareCPointer)
                shares.append(share)
                
                i += 1
            }
            
            // Free memory
            _ = CSSSS.wrapped_free_shares(sharesCPointer, Int32(n))
            
            return shares
        }
        
        return nil
    }
    
    public static func combine(shares: [String]) -> String? {
        
        let tempSize = maxDegree
        
        let resultCPointer = UnsafeMutablePointer<Int8>.allocate(capacity: tempSize)
        
        let error = CSSSS.wrapped_combine(resultCPointer, tempSize, arrayToPointer(shares), Int32(shares.count), false, false)
        
        if error.rawValue == 0 {
            return String(cString: resultCPointer)
        }
        
        return nil
    }
}

extension ShamirSecretSharing {
    fileprivate static func createArrayPointer(capacity: Int) -> UnsafeMutablePointer<UnsafeMutablePointer<Int8>?> {
        let buffer = UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>.allocate(capacity: capacity)
        for index in 0..<capacity {
            buffer[index] = UnsafeMutablePointer<Int8>.allocate(capacity: 100)
        }
        return buffer
    }
    
    fileprivate static func arrayToPointer(_ array: [String]) -> UnsafeMutablePointer<UnsafePointer<Int8>?> {
        let buffer = UnsafeMutablePointer<UnsafePointer<Int8>?>.allocate(capacity: array.count)
        
        for(index, value) in array.enumerated() {
            buffer[index] = (value as NSString).utf8String
        }
        
        return buffer
    }
    
    fileprivate static func generateRandomBytes(length: Int) -> String? {
        
        var keyData = Data(count: length)
        let result = keyData.withUnsafeMutableBytes {
            SecRandomCopyBytes(kSecRandomDefault, keyData.count, $0)
        }
        if result == errSecSuccess {
            return keyData.base64EncodedString()
        } else {
            print("Problem generating random bytes")
            return nil
        }
    }
    
//    fileprivate static func trimBytes(pointer: UnsafePointer<Int8>) -> UnsafePointer<Int8> {
//        var bytes = [Int8]()
//        var i = 0
//        
//        while pointer[i] > 0 {
//            bytes.append(pointer[i])
//            i += 1
//        }
//        
//        let result = UnsafeMutablePointer<Int8>.allocate(capacity: bytes)
//        
//    }
}
