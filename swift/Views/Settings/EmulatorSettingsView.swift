// EmulatorSettingsView.swift — EE/IOP/VU/boot/speedhack settings
// SPDX-License-Identifier: GPL-3.0+
//
// Global defaults. Anything that only one game needs belongs in the Tweaks tab, which layers
// over these.

import SwiftUI

struct EmulatorSettingsView: View {
    @State private var settings = SettingsStore.shared

    var body: some View {
        Form {
            Section {
                cpuModePicker("EE Core", selection: $settings.eeMode)
                cpuModePicker("IOP", selection: $settings.iopMode)
                cpuModePicker("VU0", selection: $settings.vu0Mode)
                cpuModePicker("VU1", selection: $settings.vu1Mode)
            } header: {
                Text("CPU Engine")
            } footer: {
                Text("JIT (recompiler) is much faster and needs JIT enabled for the app (e.g. StikDebug with the legacy script). Interpreter is slower but the most faithful — switch a component to it if a game misbehaves. Takes effect on the next boot.")
            }

            // [SPEED] The multipliers only; the mode switch itself lives in the in-game menu,
            // because that is where you use it.
            Section {
                speedStepper("Normal", value: $settings.normalSpeedPercent)
                speedStepper("Fast Forward", value: $settings.turboSpeedPercent)
                speedStepper("Slow Motion", value: $settings.slowMotionSpeedPercent)
            } header: {
                Text("Emulation Speed")
            } footer: {
                Text("Fast Forward and Slow Motion are switched on from the in-game menu. Normal is the target the emulator paces itself to — lower it if a game runs too fast to play.")
            }

            Section {
                Toggle("Fast Boot", isOn: $settings.fastBoot)
            } header: {
                Text("Boot")
            } footer: {
                Text("Skips the BIOS intro. A few games need it to run in order to initialise — missing 3D graphics is the usual sign.")
            }

            Section {
                Toggle("Fastmem", isOn: $settings.fastmem)
            } header: {
                Text("Memory")
            } footer: {
                Text("Direct memory mapping for the EE recompiler. Leave it on; disable only if 3D graphics are broken in a game. Takes effect on the next boot.")
            }

            Section {
                Stepper("EE Cycle Rate: \(settings.eeCycleRate)", value: $settings.eeCycleRate, in: -3...3)
                Stepper("EE Cycle Skip: \(settings.eeCycleSkip)", value: $settings.eeCycleSkip, in: 0...3)
                Toggle("Fast CDVD", isOn: $settings.fastCDVD)
                Toggle("VU1 Instant", isOn: $settings.vu1Instant)
                    .disabled(settings.mtvu)
                Toggle("Wait Loop Detection", isOn: $settings.waitLoop)
                Toggle("INTC Stat Hack", isOn: $settings.intcStat)
            } header: {
                Text("Speedhacks")
            } footer: {
                Text("Cycle Rate 0 is standard. Negative underclocks the EE: faster here, but the game draws slightly less each frame (−1 is roughly +4% speed for about 1% fewer draws; −2 and below drop around 7%). Positive overclocks it — slower here, but it fixes slowdown in a few games. The four switches are PCSX2's recommended defaults; turn one off only for a game that needs it. All of these take effect on the next boot.")
            }

            Section {
                Toggle(isOn: $settings.mtvu) {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("MTVU (Multi-threaded VU1)")
                        Text("Experimental")
                            .font(.caption2)
                            .foregroundStyle(.orange)
                    }
                }
                .disabled(settings.vu1Mode != .jit)
            } header: {
                Text("Experimental")
            } footer: {
                Text("Runs VU1 on its own CPU thread — often the biggest speedup for 3D-heavy games, but some games hang or glitch with it (PCSX2's game database turns it off automatically for known bad games). Needs VU1 on JIT; VU1 Instant is ignored while it is on. EE Cycle Skip (Speedhacks) 0 = off; higher values trade timing accuracy for speed. Takes effect on the next boot.")
            }

            Section {
                Button(role: .destructive) {
                    settings.resetEmulatorDefaults()
                } label: {
                    Text("Reset Emulator to Defaults")
                }
            } footer: {
                Text("Only the settings on this screen. Per-game overrides in the Tweaks tab are untouched.")
            }
        }
        .navigationTitle("Emulation")
        .navigationBarTitleDisplayMode(.inline)
    }

    @ViewBuilder
    private func speedStepper(_ title: String, value: Binding<Int>) -> some View {
        Stepper(value: value, in: 10...400, step: 10) {
            HStack {
                Text(title)
                Spacer()
                Text("\(value.wrappedValue)%").foregroundStyle(.secondary)
            }
        }
    }

    private func cpuModePicker(_ title: String, selection: Binding<CpuMode>) -> some View {
        Picker(title, selection: selection) {
            ForEach(CpuMode.allCases) { mode in
                Text(mode.label).tag(mode)
            }
        }
    }
}
