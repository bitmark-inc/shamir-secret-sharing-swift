Pod::Spec.new do |spec|
  spec.name = "SwiftSSSS"
  spec.version = "0.0.3"
  spec.summary = "Shamir secret sharing wrapper on Swift."
  spec.homepage = "https://github.com/bitmark-inc/shamir-secret-sharing-swift"
  spec.license = { type: 'GPLv2', file: 'LICENSE' }
  spec.authors = { "Bitmark Inc" => 'support@bitmark.com' }
  spec.social_media_url = "https://twitter.com/bitmarkinc"

  spec.platform = :ios, "9.1"
  spec.requires_arc = true
  spec.source = { git: "https://github.com/bitmark-inc/shamir-secret-sharing-swift.git", tag: "v#{spec.version}", submodules: true }
  spec.source_files = 'ShamirSecretSharing/**/*.{h,swift,a,c}'
  spec.xcconfig = { "SWIFT_INCLUDE_PATHS" => "$(PODS_ROOT)/SwiftSSSS/ShamirSecretSharing/CSSSS",
                    "LIBRARY_SEARCH_PATHS[sdk=iphoneos*]" => "$(PODS_ROOT)/SwiftSSSS/ShamirSecretSharing/CSSSS/GMPLib/iphoneos",
                    "LIBRARY_SEARCH_PATHS[sdk=iphonesimulator*]" => "$(PODS_ROOT)/SwiftSSSS/ShamirSecretSharing/CSSSS/GMPLib/iphonesimulator",
                    "LIBRARY_SEARCH_PATHS[sdk=macosx*]" => "$(PODS_ROOT)/SwiftSSSS/ShamirSecretSharing/CSSSS/GMPLib/macosx"}

  spec.preserve_paths = "ShamirSecretSharing/CSSSS/module.map"
  spec.vendored_libraries = "ShamirSecretSharing/CSSSS/GMPLib/iphoneos/libgmp.a",
                            "ShamirSecretSharing/CSSSS/GMPLib/iphonesimulator/libgmp.a",
                            "ShamirSecretSharing/CSSSS/GMPLib/macosx/libgmp.a"
end
