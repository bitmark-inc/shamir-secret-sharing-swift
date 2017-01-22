//
//  Extensions.swift
//  ShamirSecretSharing
//
//  Created by Anh Nguyen on 1/16/17.
//  Copyright © 2016 Bitmark. All rights reserved.
//

import Foundation

extension NSData {
    func bytesPtr<T>() -> UnsafePointer<T>{
        let rawBytes = self.bytes
        return rawBytes.assumingMemoryBound(to: T.self);
    }
}

extension NSMutableData {
    func mutableBytesPtr<T>() -> UnsafeMutablePointer<T>{
        let rawBytes = self.mutableBytes
        return rawBytes.assumingMemoryBound(to: T.self)
    }
}

extension String {
    var utf8String: UnsafeMutablePointer<Int8> {
        return UnsafeMutablePointer(mutating: (self as NSString).utf8String!)
    }
}
