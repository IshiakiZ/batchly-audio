"""Verify the built VST3 in an independent host and create original listening files.

Requires numpy, pedalboard and soundfile. No commercial plugin or audio assets are used.
"""
import argparse
import json
import time
from pathlib import Path

import numpy as np
import soundfile as sf
from pedalboard import load_plugin

parser = argparse.ArgumentParser()
parser.add_argument("--plugin", required=True, type=Path)
parser.add_argument("--output", required=True, type=Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
plugin = load_plugin(str(args.plugin))
assert plugin.is_effect and not plugin.is_instrument
report = {"plugin": plugin.name, "checks": [], "parameters": sorted(plugin.parameters)}

for sample_rate in (44100, 48000, 96000):
    t = np.arange(sample_rate * 2, dtype=np.float64) / sample_rate
    tone = (0.1 * np.sin(2 * np.pi * 220 * t)).astype(np.float32)
    source = np.stack((tone, tone))
    plugin.mix = 0
    # Pedalboard infers text parameter steps; normalized values avoid its rounding to +0.012 dB.
    plugin.parameters["output_db"].raw_value = 2 / 3
    dry = plugin(source, sample_rate, buffer_size=257)
    assert np.max(np.abs(dry - source)) < 1e-6, "Dry path changed the audio"
    plugin.mix = 1
    plugin.depth = 0.8
    plugin.rate_hz = 1.4
    plugin.stereo_width = 1
    wet = plugin(source, sample_rate, buffer_size=512)
    assert np.isfinite(wet).all()
    assert np.sqrt(np.mean((wet - source) ** 2)) > .02
    assert np.sqrt(np.mean((wet[0] - wet[1]) ** 2)) > .005
    plugin.bypass = True
    bypass = plugin(source, sample_rate, buffer_size=127)
    assert np.max(np.abs(bypass - source)) < 1e-6
    plugin.bypass = False
    mono = plugin(source[:1], sample_rate, buffer_size=256)
    assert mono.shape == source[:1].shape and np.isfinite(mono).all()
    report["checks"].append(f"{sample_rate} Hz: dry, wet, width, bypass, mono")

plugin.depth = .61
plugin.mix = .42
saved = plugin.raw_state
plugin.depth = .05
plugin.mix = .9
plugin.raw_state = saved
assert abs(plugin.depth - .61) < .002 and abs(plugin.mix - .42) < .002
report["checks"].append("Plugin state restores edited parameters")

plugin.mix = 1
plugin.noise = 0
silence = plugin(np.zeros((2, 48000), dtype=np.float32), 48000)
assert np.max(np.abs(silence)) == 0
report["checks"].append("Silence remains silent with noise off")

sample_rate = 48000
t = np.arange(sample_rate * 12, dtype=np.float64) / sample_rate
source = np.zeros_like(t)
for chord_index, chord in enumerate(((220, 261.625565, 329.627557, 391.995436),
                                     (174.614116, 220, 261.625565, 329.627557),
                                     (196, 246.941651, 293.664768, 369.994423),
                                     (164.813778, 196, 246.941651, 293.664768))):
    local = t - chord_index * 3
    active = (local >= 0) & (local < 3)
    envelope = np.where(active, (1 - np.exp(-np.maximum(local, 0) * 70)) * np.exp(-np.maximum(local, 0) * 1.5), 0)
    for frequency in chord:
        source += .045 * envelope * (np.sin(2 * np.pi * frequency * t) + .2 * np.sin(4 * np.pi * frequency * t))
source = np.stack((source, source)).astype(np.float32)
sf.write(args.output / "01-original.wav", source.T, sample_rate, subtype="PCM_24")
# A fresh instance restores the default even when a host skips reselecting the current program.
plugin = load_plugin(str(args.plugin))
start = time.perf_counter()
processed = plugin(source, sample_rate, buffer_size=512)
elapsed = time.perf_counter() - start
assert np.isfinite(processed).all() and np.max(np.abs(processed)) < 1
sf.write(args.output / "02-drift-chorus.wav", processed.T, sample_rate, subtype="PCM_24")
plugin.program = "Pure vibrato"
vibrato = plugin(source, sample_rate, buffer_size=512)
sf.write(args.output / "03-drift-vibrato.wav", vibrato.T, sample_rate, subtype="PCM_24")
report["render_seconds_for_12_seconds"] = elapsed
report["chorus_peak"] = float(np.max(np.abs(processed)))
report["checks"].append("Original listening examples rendered without clipping")

for sample_rate in (44100, 48000, 96000):
    p = load_plugin(str(args.plugin))
    p.program = "Fresh spool"
    assert p.patina_enabled and not p.drift_enabled
    t = np.arange(sample_rate * 2, dtype=np.float64) / sample_rate
    probe = np.stack([.1 * np.sin(2 * np.pi * 443 * t)] * 2).astype(np.float32)
    p.patina_mix = 0
    assert np.max(np.abs(p(probe, sample_rate) - probe)) < 1e-6
    p.patina_mix = 1
    p.patina_chorus = 1
    tape = p(probe, sample_rate, buffer_size=137)
    assert np.isfinite(tape).all() and np.max(np.abs(tape)) < 1
    assert np.sqrt(np.mean((tape[0] - tape[1]) ** 2)) > .005
    p.bypass = True
    assert np.max(np.abs(p(probe, sample_rate) - probe)) < 1e-6
    p.bypass = False
    assert p(probe[:1], sample_rate).shape == probe[:1].shape
    p.patina_hiss = 0
    assert np.max(np.abs(p(np.zeros_like(probe), sample_rate))) == 0
    report["checks"].append(f"{sample_rate} Hz: Patina dry, wet, stereo, global bypass, mono, noise-off silence")

p.patina_wear = .71
p.patina_sample_rate_hz = 8000
p.drift_enabled = True
saved = p.raw_state
p.patina_wear = .01
p.patina_enabled = False
p.raw_state = saved
assert abs(p.patina_wear - .71) < .002 and p.patina_enabled and p.drift_enabled
report["checks"].append("Two-module rack and Patina controls survive state recall")

# Captured from the released 0.1.0 plugin, with Depth 61% and Mix 42%.
p.atrium_enabled = True
p.raw_state = (Path(__file__).resolve().parents[1] / "tests/fixtures/drift-0.1.0.bapreset").read_bytes()
assert abs(p.depth - .61) < .002 and abs(p.mix - .42) < .002
assert p.drift_enabled and not p.patina_enabled and not p.atrium_enabled
assert abs(p.patina_wear - .2) < .002
report["checks"].append("Actual 0.1.0 state disables Patina and restores legacy Drift controls")

p = load_plugin(str(args.plugin))
p.program = "Fresh spool"
start = time.perf_counter()
tape = p(source, 48000, buffer_size=512)
report["patina_render_seconds_for_12_seconds"] = time.perf_counter() - start
assert np.isfinite(tape).all() and np.max(np.abs(tape)) < 1
sf.write(args.output / "04-patina-fresh-spool.wav", tape.T, 48000, subtype="PCM_24")
p.program = "Submerged"
submerged = p(source, 48000)
sf.write(args.output / "05-patina-submerged.wav", submerged.T, 48000, subtype="PCM_24")
p.program = "Pocket cassette"
p.drift_enabled = True
stacked = p(source, 48000)
assert np.isfinite(stacked).all() and np.max(np.abs(stacked)) < 1
sf.write(args.output / "06-drift-into-patina.wav", stacked.T, 48000, subtype="PCM_24")
report["checks"].append("Patina and combined-rack listening examples rendered without clipping")

for sample_rate in (44100, 48000, 96000):
    p = load_plugin(str(args.plugin))
    p.program = "Open atrium"
    assert p.atrium_enabled and not p.drift_enabled and not p.patina_enabled
    impulse = np.zeros((2, sample_rate * 3), dtype=np.float32)
    impulse[:, 0] = .25
    p.atrium_mix = 0
    assert np.max(np.abs(p(impulse, sample_rate) - impulse)) < 1e-6
    p.atrium_mix = 1
    room = p(impulse, sample_rate, buffer_size=137)
    assert np.isfinite(room).all() and np.max(np.abs(room)) < 1
    assert np.sum(room[:, sample_rate // 4:] ** 2) > 1e-6
    assert np.sum((room[0] - room[1]) ** 2) > 1e-6
    p.atrium_width = 0
    centered = p(impulse, sample_rate)
    assert np.array_equal(centered[0], centered[1])
    mono = p(impulse[:1], sample_rate)
    assert mono.shape == impulse[:1].shape and np.isfinite(mono).all()
    p.bypass = True
    assert np.max(np.abs(p(impulse, sample_rate) - impulse)) < 1e-6
    p.bypass = False
    assert np.max(np.abs(p(np.zeros_like(impulse), sample_rate))) == 0
    report["checks"].append(f"{sample_rate} Hz: Atrium dry, stereo tail, width, mono, bypass, silence")

p.atrium_decay_s = 5.3
p.atrium_pre_delay_ms = 71
p.atrium_motion = .67
p.drift_enabled = True
p.patina_enabled = True
saved = p.raw_state
p.atrium_decay_s = .3
p.atrium_enabled = False
p.raw_state = saved
assert abs(p.atrium_decay_s - 5.3) < .02 and p.atrium_enabled and p.patina_enabled and p.drift_enabled
assert abs(p.atrium_pre_delay_ms - 71) < .1 and abs(p.atrium_motion - .67) < .002
report["checks"].append("Three-module rack and Atrium controls survive state recall")
p.raw_state = (Path(__file__).resolve().parents[1] / "tests/fixtures/patina-0.2.0.bapreset").read_bytes()
assert p.drift_enabled and p.patina_enabled and not p.atrium_enabled
assert abs(p.patina_wear - .476) < .002 and abs(p.atrium_decay_s - 2.4) < .02
report["checks"].append("Actual 0.2.0 state restores Drift/Patina and disables Atrium after an active room")

p = load_plugin(str(args.plugin))
p.program = "Open atrium"
start = time.perf_counter()
room_source = np.pad(source, ((0, 0), (0, 48000 * 6)))
room = p(room_source, 48000, buffer_size=512)
report["atrium_render_seconds_for_18_seconds"] = time.perf_counter() - start
assert np.isfinite(room).all() and np.max(np.abs(room)) < 1
sf.write(args.output / "07-atrium-open.wav", room.T, 48000, subtype="PCM_24")
p.program = "After hours"
room = p(np.pad(source, ((0, 0), (0, 48000 * 20))), 48000, buffer_size=512)
assert np.isfinite(room).all() and np.max(np.abs(room)) < 1
sf.write(args.output / "08-atrium-after-hours.wav", room.T, 48000, subtype="PCM_24")
p.program = "Open atrium"
p.patina_enabled = True
p.drift_enabled = True
rack = p(room_source, 48000, buffer_size=512)
assert np.isfinite(rack).all() and np.max(np.abs(rack)) < 1
sf.write(args.output / "09-drift-patina-atrium.wav", rack.T, 48000, subtype="PCM_24")
report["checks"].append("Atrium and full-rack listening examples with tails render without clipping")
(args.output / "verification.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps(report, indent=2))
