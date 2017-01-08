//
//  SecretSharing.swift
//  SecretSharing
//
//  Created by Anh Nguyen on 1/6/17.
//  Copyright © 2017 Bitmark. All rights reserved.
//

import Foundation
import BigInt
import CryptoSwift

public enum SecretSharingVersion {
    case compact96
    case compact104
    case compact128
}

public class SecretSharing {
    
    typealias ShareData = (m: Int, x: Int, y: BigUInt)
    
    let version: SecretSharingVersion
    let order: BigUInt
    let bitLength: Int
    
    public init(version: SecretSharingVersion) {
        self.version = version
        
        switch version {
        case .compact96:
            self.bitLength = 96
            self.order = BigUInt("ffffffffffffffffffffffef", radix: 16)!
            break
        case .compact104:
            self.bitLength = 104
            self.order = BigUInt("ffffffffffffffffffffffffef", radix: 16)!
            break
        case .compact128:
            self.bitLength = 128
            self.order = BigUInt("ffffffffffffffffffffffffffffff61", radix: 16)!
            break
        }
    }
    
    public func split(secret: Data, threshold m: Int, shares n: Int) -> [Data] {
        precondition(secret.count * 8 == self.bitLength, "Secret length does not match bitlength of BTCSecretSharing.")
        
        let prime = self.order
        let secretNumber = BigUInt(secret)
        
        precondition(secretNumber <= prime, "Secret as bigint must be less than prime order of BTCSecretSharing.")
        
        precondition(n >= 1 || n <= 16, "Number of shares (N) must be between 1 and 16.")
        
        var shares = [Data]()
        var coefficients = [secretNumber]
        
        for i in 0 ..< (m - 1) {
            // Generate unpredictable yet deterministic coefficients for each secret and M.
            
            var seed = secret
            seed.append(UInt8(m))
            seed.append(UInt8(i))
            let coefdata = prng(seed: seed)
            let coef = BigUInt(coefdata)
            coefficients.append(coef)
        }
        
        for i in 0 ..< n {
            let x = BigUInt(i + 1)
            var exp = 1
            var y = coefficients[0]
            
            while exp < m {
                var coef = coefficients[exp]
                var xexp = x
                xexp = xexp.power(BigUInt(exp), modulus: prime)
                coef = (coef * xexp) % prime
                y = (y + coef) % prime
//                y = (y + (coefficients[exp] * (x.power(BigUInt(exp), modulus: prime)) % prime)) % prime
                exp += 1
            }
            
            let share = encodeShare(m: m, x: i + 1, y: y)
            shares.append(share)
        }
        
        return shares
    }
    
    public func join(shares: [Data]) throws -> Data {
        let prime = BigInt(self.order)
        let primeU = self.order
        
        var points = [ShareData]()
        for share in shares {
            if let point = decodeShare(share) {
                points.append(point)
            }
        }
        
        precondition(points.count > 0, "No shares provided.")
        
        var ms = Set<Int>()
        for point in points {
            ms.insert(point.m)
        }
        precondition(ms.count == 1, "All shares must have the same threshold (M) value.")
        
        var xs = Set<Int>()
        for point in points {
            xs.insert(point.x)
        }
        precondition(xs.count == 1, "All shares must have unique indexes (X) values.")
        
        let m = ms.first!
        precondition(points.count >= m, "Not enough shares to restore the secret")
        
        var y = BigInt(0)
        for formula in 0 ..< m {
            var numerator = BigInt(1)
            var denominator = BigUInt(1)
            
            for count in 0 ..< m {
                if formula != count { // skip element with i == j
                    var startposition = BigUInt(points[formula].x)
                    let negnextposition = BigInt(0 - points[count].x)
                    numerator = (numerator * negnextposition) % prime
                    startposition += negnextposition
                    denominator = (denominator * startposition) % primeU
                }
            }
            
            var value = BigInt(points[formula].y)
            value *= numerator
            value = value * BigInt((denominator.inverse(primeU)! % primeU))
            
            y += prime
            y += (value % prime)
        }
        
        return y.abs.serialize().subdata(in: (self.bitLength / 8) ..< 32)
    }
}

extension SecretSharing {
    internal func prng(seed: Data) -> Data {
        var x = self.order
        var s: Data?
        var pad = Data()
        
        while x >= self.order {
            var input = seed
            input.append(pad)
            s = input.sha256().sha256().subdata(in: 0 ..< (self.bitLength / 8))
            x = BigUInt(s!)
            
            pad.append(UInt8(0))
        }
        
        return s!
        
    }
    
    // Returns mmmmxxxx yyyyyyyy yyyyyyyy ... (N bytes of y)
    internal func encodeShare(m: Int, x: Int, y: BigUInt) -> Data {
        let m = toNibble(m)
        let x = toNibble(x)
        let prefix = UInt8((m << 4) + x)
        var data = Data(bytes: [prefix])
        let yData = fillZeroBytes(data: y.serialize(), desiredLength: 32).subdata(in: (32 - self.bitLength / 8) ..< 32)
        
        data.append(yData)
        return data
    }
    
    // Returns [m, x, y] where m, x - Int, y - BigUInt
    internal func decodeShare(_ share: Data) -> ShareData? {
        if share.count != (self.bitLength / 8 + 1) {
            return nil
        }
        
        let byte = Int(share.bytes.first!)
        let m = fromNibble(byte >> 4)
        let x = fromNibble(byte & 0x0f)
        
        let yData = share.subdata(in: 1 ..< (self.bitLength / 8 + 1))
        let y = BigUInt(yData)
        
        return (m, x, y)
    }
    
    // Encodes values in range 1..16 to one nibble where all values are encoded as-is,
    // except for 16 which becomes 0. This is to make strings look friendly for common cases when M,N < 16
    private func toNibble(_ x: Int) -> Int {
        return x == 16 ? 0 : x
    }
    
    private func fromNibble(_ x: Int) -> Int {
        return x == 0 ? 16 : x
    }
    
    private func fillZeroBytes(data: Data, desiredLength: Int) -> Data {
        precondition(data.count <= desiredLength, "Data's length should be smaller than desired length")
        
        var buffers = [UInt8](data)
        
        for _ in (buffers.count + 1) ... desiredLength {
            buffers.insert(0, at: 0)
        }
        
        return Data(bytes: buffers)
    }
}
