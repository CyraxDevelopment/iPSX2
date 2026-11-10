// CheatCollection.swift — ready-made cheat files from the community collection on GitHub
// SPDX-License-Identifier: GPL-3.0+
//
// Nothing from the collection ships in the app. When the user opens a game's Patches & Cheats
// screen, the app asks GitHub whether xs1l3n7x/pcsx2_cheats_collection has a .pnach for that
// game; if it does, the user is offered the codes and the file is saved into Documents/cheats
// only after they say yes.
//
// The collection names its files the way PCSX2 looks them up — "SERIAL_CRC.pnach", or just
// "CRC.pnach" when the serial is unknown — so the raw file URL can be tried directly. That
// avoids the GitHub API (and its 60 requests/hour limit) and needs no index.

import Foundation

@MainActor
enum CheatCollection {
    /// UserDefaults key for Settings › Patches › Find Cheats Online. On unless turned off.
    static let enabledKey = "onlineCheatLookup"

    static let sourceName = "pcsx2_cheats_collection"
    static let sourceURL = URL(string: "https://github.com/xs1l3n7x/pcsx2_cheats_collection")!
    private static let rawBase = "https://raw.githubusercontent.com/xs1l3n7x/pcsx2_cheats_collection/main/cheats/"

    struct Offer: Equatable {
        /// Name the file is saved under in Documents/cheats.
        let filename: String
        let data: Data
        /// Section names in the file, which are what the user can tick one by one.
        let codeNames: [String]
    }

    /// CRCs that had no file, so reopening the same game does not ask GitHub again right away.
    private static var misses: [UInt32: Date] = [:]
    private static let missTTL: TimeInterval = 6 * 60 * 60

    static var isEnabled: Bool {
        UserDefaults.standard.object(forKey: enabledKey) as? Bool ?? true
    }

    static func candidateFilenames(serial: String, crc: UInt32) -> [String] {
        let crcHex = String(format: "%08X", crc)
        let s = serial.trimmingCharacters(in: .whitespaces).uppercased()
        return s.isEmpty ? ["\(crcHex).pnach"] : ["\(s)_\(crcHex).pnach", "\(crcHex).pnach"]
    }

    /// Returns an offer when the collection has codes for this game that the user does not
    /// already have. Returns nil when the lookup is off, nothing is found, or the device is
    /// offline — none of those are worth interrupting the user for.
    static func lookUp(serial: String, crc: UInt32, cheatsDirectory: String) async -> Offer? {
        guard isEnabled, crc != 0 else { return nil }

        let names = candidateFilenames(serial: serial, crc: crc)
        let fm = FileManager.default
        if names.contains(where: { fm.fileExists(atPath: (cheatsDirectory as NSString).appendingPathComponent($0)) }) {
            return nil
        }
        if let missed = misses[crc], Date().timeIntervalSince(missed) < missTTL {
            return nil
        }

        for name in names {
            guard let url = URL(string: rawBase + name) else { continue }
            let request = URLRequest(url: url, cachePolicy: .useProtocolCachePolicy, timeoutInterval: 10)
            let data: Data
            let response: URLResponse
            do {
                (data, response) = try await URLSession.shared.data(for: request)
            } catch {
                return nil // offline or blocked: try again next time
            }
            guard let http = response as? HTTPURLResponse else { continue }
            if http.statusCode == 404 { continue }
            guard http.statusCode == 200, !data.isEmpty, data.count < 2_000_000 else { return nil }

            let codes = codeNames(in: String(decoding: data, as: UTF8.self))
            guard !codes.isEmpty else { continue }
            return Offer(filename: name, data: data, codeNames: codes)
        }

        misses[crc] = Date()
        return nil
    }

    /// Writes the offered file into the cheats folder. Never overwrites a file the user has.
    static func save(_ offer: Offer, cheatsDirectory: String) -> Bool {
        let fm = FileManager.default
        let path = (cheatsDirectory as NSString).appendingPathComponent(offer.filename)
        guard !fm.fileExists(atPath: path) else { return false }
        do {
            try fm.createDirectory(atPath: cheatsDirectory, withIntermediateDirectories: true)
            try offer.data.write(to: URL(fileURLWithPath: path), options: .atomic)
            return true
        } catch {
            return false
        }
    }

    /// "[Infinite Health]" style section headers, which name the individual codes. A name can
    /// appear several times (one code split over sections); PCSX2 treats those as one code.
    private static func codeNames(in pnach: String) -> [String] {
        var names: [String] = []
        var seen = Set<String>()
        for raw in pnach.split(whereSeparator: \.isNewline) {
            let line = raw.trimmingCharacters(in: .whitespaces)
            guard line.count > 2, line.hasPrefix("["), line.hasSuffix("]") else { continue }
            let name = line.dropFirst().dropLast().trimmingCharacters(in: .whitespaces)
            if !name.isEmpty, seen.insert(name).inserted { names.append(name) }
        }
        return names
    }
}
