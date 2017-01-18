//
//  ShamirSecretSharing.swift
//  ShamirSecretSharing
//
//  Created by Anh Nguyen on 1/10/17.
//  Copyright © 2017 Bitmark. All rights reserved.
//

import Foundation
import CShamir

public class ShamirSecretSharing {
    
    public static func split(secret: String, shares: Int, threshold: Int) -> [String] {
        
        let pointer = (secret as NSString).utf8String
        let unsafeMutablePointer: UnsafeMutablePointer<Int8> = UnsafeMutablePointer(mutating: pointer!)
        
        let shares = CShamir.generate_share_strings(unsafeMutablePointer, Int32(shares), Int32(threshold))
        
        let cResult =  String(cString: shares!)
        return cResult.components(separatedBy: .newlines)
    }

    public static func join(shares: [String]) -> String {
        let sharesString = shares.joined(separator: "\n")
        let pointer = (sharesString as NSString).utf8String
        let unsafeMutablePointer: UnsafeMutablePointer<Int8> = UnsafeMutablePointer(mutating: pointer!)
        
        let secret = CShamir.extract_secret_from_share_strings(unsafeMutablePointer)
        
        return String(cString: secret!)
    }
}
