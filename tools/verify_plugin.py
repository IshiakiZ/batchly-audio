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
(args.output / "verification.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps(report, indent=2))
