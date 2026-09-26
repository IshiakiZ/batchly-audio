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
# Exercise the new modal bank through the public VST3 parameter interface.
for sample_rate in (44100, 48000, 96000):
    p = load_plugin(str(args.plugin))
    p.program = "Glass strings"
    assert p.chime_enabled and not p.drift_enabled and not p.patina_enabled and not p.atrium_enabled
    impulse = np.zeros((2, sample_rate * 3), dtype=np.float32)
    impulse[:, 0] = .5
    p.chime_mix = 0
    assert np.max(np.abs(p(impulse, sample_rate) - impulse)) < 1e-6
    p.chime_mix = 1
    ringing = p(impulse, sample_rate, buffer_size=137)
    assert np.isfinite(ringing).all() and np.max(np.abs(ringing)) < 1
    assert np.sum(ringing[:, sample_rate // 10:] ** 2) > 1e-7
    assert np.sum((ringing[0] - ringing[1]) ** 2) > 1e-7
    p.chime_width = 0
    centered = p(impulse, sample_rate)
    assert np.array_equal(centered[0], centered[1])
    assert p(impulse[:1], sample_rate).shape == impulse[:1].shape
    p.bypass = True
    assert np.max(np.abs(p(impulse, sample_rate) - impulse)) < 1e-6
    p.bypass = False
    assert np.max(np.abs(p(np.zeros_like(impulse), sample_rate))) == 0
    report["checks"].append(f"{sample_rate} Hz: Chime dry, stereo ring, width, mono, bypass, silence")

p.chime_root = "D"
p.chime_scale = "Dorian"
p.chime_octave = 4
p.chime_ring_s = 3.1
p.drift_enabled = p.patina_enabled = p.atrium_enabled = True
saved = p.raw_state
p.program = "Minor bells"
p.raw_state = saved
assert p.chime_root == "D" and p.chime_scale == "Dorian" and p.chime_octave == 4
assert abs(p.chime_ring_s - 3.1) < .02
assert p.chime_enabled and p.drift_enabled and p.patina_enabled and p.atrium_enabled
report["checks"].append("Four-module rack, key, scale, octave and ring survive state recall")
for version in ("drift-0.1.0", "patina-0.2.0", "atrium-0.3.0"):
    p.chime_enabled = True
    p.raw_state = (Path(__file__).resolve().parents[1] / f"tests/fixtures/{version}.bapreset").read_bytes()
    assert not p.chime_enabled and abs(p.chime_ring_s - 1.6) < .02
    if version.startswith("atrium"):
        assert p.drift_enabled and p.patina_enabled and p.atrium_enabled
        assert abs(p.atrium_decay_s - 4.33) < .02
    report["checks"].append(f"Actual {version} state disables Chime and retains its saved rack")

for index, name in enumerate(("Glass strings", "Minor bells", "Copper choir", "Small music box", "Suspended air")):
    p = load_plugin(str(args.plugin))
    p.program = name
    start = time.perf_counter()
    ringing = p(np.pad(source, ((0, 0), (0, 48000 * 13))), 48000, buffer_size=512)
    assert np.isfinite(ringing).all() and np.max(np.abs(ringing)) < 1
    sf.write(args.output / f"{10 + index:02}-chime-{name.lower().replace(' ', '-')}.wav", ringing.T, 48000, subtype="PCM_24")
    if index == 0: report["chime_render_seconds_for_25_seconds"] = time.perf_counter() - start
report["checks"].append("All five original Chime listening examples render with full tails and no clipping")
for sample_rate in (44100, 48000, 96000):
    p = load_plugin(str(args.plugin))
    p.program = "Slow orbit"
    assert p.helix_enabled and not p.drift_enabled and not p.patina_enabled and not p.atrium_enabled and not p.chime_enabled
    t = np.arange(sample_rate * 2) / sample_rate
    probe = np.stack([.15 * np.sin(2 * np.pi * 443 * t)] * 2).astype(np.float32)
    p.helix_mix = 0
    assert np.max(np.abs(p(probe, sample_rate) - probe)) < 1e-6
    p.helix_mix = .5
    phase = p(probe, sample_rate, buffer_size=137)
    assert np.isfinite(phase).all() and np.max(np.abs(phase)) < 1
    assert np.sqrt(np.mean((phase - probe) ** 2)) > .01
    assert np.sqrt(np.mean((phase[0] - phase[1]) ** 2)) > .005
    p.helix_width = 0
    centered = p(probe, sample_rate)
    assert np.max(np.abs(centered[0] - centered[1])) < 1e-7
    assert p(probe[:1], sample_rate).shape == probe[:1].shape
    p.bypass = True
    assert np.max(np.abs(p(probe, sample_rate) - probe)) < 1e-6
    p.bypass = False
    assert np.max(np.abs(p(np.zeros_like(probe), sample_rate))) == 0
    report["checks"].append(f"{sample_rate} Hz: Helix dry, phase movement, stereo, width, mono, bypass, silence")
p.helix_feedback = -.61
p.helix_rate_hz = 1.7
p.drift_enabled = p.patina_enabled = p.atrium_enabled = p.chime_enabled = True
saved = p.raw_state
p.program = "Hollow metal"
p.raw_state = saved
assert abs(p.helix_feedback + .61) < .002 and abs(p.helix_rate_hz - 1.7) < .02
assert p.helix_enabled and p.chime_enabled and p.atrium_enabled and p.patina_enabled and p.drift_enabled
report["checks"].append("Five-module rack and signed Helix feedback survive state recall")
for version in ("drift-0.1.0", "patina-0.2.0", "atrium-0.3.0", "chime-0.4.0"):
    p.helix_enabled = True
    p.raw_state = (Path(__file__).resolve().parents[1] / f"tests/fixtures/{version}.bapreset").read_bytes()
    assert not p.helix_enabled and abs(p.helix_feedback - .35) < .002
    if version.startswith("chime"):
        assert p.chime_enabled and p.chime_octave == 4 and p.chime_root == "D" and p.chime_scale == "Dorian"
    report["checks"].append(f"Actual {version} state disables Helix and retains its saved rack")
for index, name in enumerate(("Slow orbit", "Silver sweep", "Deep current", "Retro spin", "Hollow metal")):
    p = load_plugin(str(args.plugin)); p.program = name
    start = time.perf_counter()
    phase = p(np.pad(source, ((0, 0), (0, 48000 * 2))), 48000, buffer_size=512)
    assert np.isfinite(phase).all() and np.max(np.abs(phase)) < 1
    sf.write(args.output / f"{15 + index:02}-helix-{name.lower().replace(' ', '-')}.wav", phase.T, 48000, subtype="PCM_24")
    if index == 0: report["helix_render_seconds_for_14_seconds"] = time.perf_counter() - start
report["checks"].append("All five original Helix presets render with tails and no clipping")
for sample_rate in (44100, 48000, 96000):
    p = load_plugin(str(args.plugin)); p.program = "Clear vocal"
    assert p.gleam_enabled and not any(getattr(p, name) for name in ("drift_enabled", "patina_enabled", "atrium_enabled", "chime_enabled", "helix_enabled"))
    t = np.arange(sample_rate) / sample_rate
    probe = np.stack([.1 * np.sin(2 * np.pi * 11000 * t)] * 2).astype(np.float32)
    p.gleam_mix = 0
    assert np.max(np.abs(p(probe, sample_rate) - probe)) < 1e-6
    p.gleam_mix = 1
    processed = p(probe, sample_rate, buffer_size=137)
    assert np.isfinite(processed).all() and np.max(np.abs(processed)) < 1
    assert np.sqrt(np.mean((processed - probe) ** 2)) > .005
    assert np.max(np.abs(processed[0] - processed[1])) < 1e-7
    assert p(probe[:1], sample_rate).shape == probe[:1].shape
    p.bypass = True
    assert np.max(np.abs(p(probe, sample_rate) - probe)) < 1e-6
    p.bypass = False
    assert np.max(np.abs(p(np.zeros_like(probe), sample_rate))) == 0
    report["checks"].append(f"{sample_rate} Hz: Gleam dry, brightness, stereo, mono, bypass, silence")
p.gleam_air_db = 7.3
p.gleam_tame = .82
for name in ("drift_enabled", "patina_enabled", "atrium_enabled", "chime_enabled", "helix_enabled"): setattr(p,name,True)
saved = p.raw_state
p.program = "Soft lift"
p.raw_state = saved
assert abs(p.gleam_air_db - 7.3) < .02 and abs(p.gleam_tame - .82) < .002
assert all(getattr(p,name) for name in ("drift_enabled", "patina_enabled", "atrium_enabled", "chime_enabled", "helix_enabled", "gleam_enabled"))
report["checks"].append("Six-effect rack and edited Gleam controls survive state recall")
for version in ("drift-0.1.0", "patina-0.2.0", "atrium-0.3.0", "chime-0.4.0", "helix-0.5.0"):
    p.gleam_enabled = True
    p.raw_state = (Path(__file__).resolve().parents[1] / f"tests/fixtures/{version}.bapreset").read_bytes()
    assert not p.gleam_enabled and abs(p.gleam_air_db - 4) < .02
    if version.startswith("helix"):
        assert p.helix_enabled and abs(p.helix_feedback + .61) < .002
    report["checks"].append(f"Actual {version} state disables Gleam and retains its saved rack")
for index, name in enumerate(("Clear vocal", "Silver top", "Drum shine", "Soft lift", "Open mix")):
    p = load_plugin(str(args.plugin)); p.program = name
    start = time.perf_counter()
    bright = p(np.pad(source, ((0, 0), (0, 48000))), 48000, buffer_size=512)
    assert np.isfinite(bright).all() and np.max(np.abs(bright)) < 1
    sf.write(args.output / f"{20 + index:02}-gleam-{name.lower().replace(' ', '-')}.wav", bright.T, 48000, subtype="PCM_24")
    if index == 0: report["gleam_render_seconds_for_13_seconds"] = time.perf_counter() - start
report["checks"].append("All five original Gleam presets render with no clipping")
for sample_rate in (44100, 48000, 96000):
    p = load_plugin(str(args.plugin)); p.program = "Cross town"
    assert p.relay_enabled and not any(getattr(p,n) for n in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled"))
    impulse = np.zeros((2, sample_rate * 2), dtype=np.float32); impulse[:, 0] = .1
    p.relay_mix = 0
    assert np.max(np.abs(p(impulse,sample_rate)-impulse)) < 1e-6
    p.relay_mix = 1; p.relay_motion = 0; p.relay_time_ms = 100
    echoes = p(impulse,sample_rate,buffer_size=137)
    assert np.isfinite(echoes).all() and np.max(np.abs(echoes)) < 1
    assert np.max(np.abs(echoes[:,:int(sample_rate*.099)])) == 0
    assert np.max(np.abs(echoes[0,int(sample_rate*.099):int(sample_rate*.12)])) > .005
    assert np.max(np.abs(echoes[1,int(sample_rate*.199):int(sample_rate*.22)])) > .001
    assert np.max(np.abs(echoes[0]-echoes[1])) > .001
    assert p(impulse[:1],sample_rate).shape == impulse[:1].shape
    p.bypass = True
    assert np.max(np.abs(p(impulse,sample_rate)-impulse)) < 1e-6
    p.bypass = False
    assert np.max(np.abs(p(np.zeros_like(impulse),sample_rate))) == 0
    report["checks"].append(f"{sample_rate} Hz: Relay dry, echo timing, alternating stereo repeats, mono, bypass, silence")
p.relay_time_ms = 423; p.relay_feedback = .71
# This host rounds text-inferred parameter steps when assigning physical values.
# Compare the actual accepted normalized values to test lossless state recall.
saved_time = p.parameters["relay_time_ms"].raw_value
saved_feedback = p.parameters["relay_feedback"].raw_value
for name in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled"): setattr(p,name,True)
saved = p.raw_state; p.program = "Bent signal"; p.raw_state = saved
assert abs(p.parameters["relay_time_ms"].raw_value-saved_time) < 1e-7
assert abs(p.parameters["relay_feedback"].raw_value-saved_feedback) < 1e-7
assert all(getattr(p,name) for name in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled","relay_enabled"))
report["checks"].append("Seven-effect rack and edited delay time/feedback survive state recall")
for version in ("drift-0.1.0","patina-0.2.0","atrium-0.3.0","chime-0.4.0","helix-0.5.0","gleam-0.6.0"):
    p.relay_enabled = True
    p.raw_state = (Path(__file__).resolve().parents[1] / f"tests/fixtures/{version}.bapreset").read_bytes()
    assert not p.relay_enabled and abs(p.relay_time_ms-350) < .2
    if version.startswith("gleam"): assert p.gleam_enabled and abs(p.gleam_air_db-6.3) < .02
    report["checks"].append(f"Actual {version} state disables Relay and retains its saved rack")
for index,name in enumerate(("Soft answer","Cross town","Short circuit","Long return","Bent signal")):
    p = load_plugin(str(args.plugin)); p.program = name
    start = time.perf_counter()
    echoes = p(np.pad(source,((0,0),(0,48000*25))),48000,buffer_size=512)
    assert np.isfinite(echoes).all() and np.max(np.abs(echoes)) < 1
    assert np.max(np.abs(echoes[:,-48000:])) < .00001
    sf.write(args.output/f"{25+index:02}-relay-{name.lower().replace(' ','-')}.wav",echoes.T,48000,subtype="PCM_24")
    if index == 0: report["relay_render_seconds_for_37_seconds"] = time.perf_counter()-start
report["checks"].append("Five original Relay presets render with decayed tails and no clipping")
# Original drum probes exercise Forge through the real VST3 wrapper.
for sample_rate in (44100,48000,96000):
    p=load_plugin(str(args.plugin));p.program="First strike"
    assert p.forge_enabled and not any(getattr(p,n) for n in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled","relay_enabled"))
    t=np.arange(sample_rate)/sample_rate
    hit=(.18*np.exp(-np.fmod(t,.25)*20)*np.sin(2*np.pi*600*t)).astype(np.float32)
    probe=np.stack((hit,hit*.7))
    p.forge_mix=0
    assert np.max(np.abs(p(probe,sample_rate)-probe))<1e-6
    p.forge_mix=1
    shaped=p(probe,sample_rate,buffer_size=137)
    assert np.isfinite(shaped).all() and np.sqrt(np.mean((shaped-probe)**2))>.005
    p.forge_width=0
    centered=p(probe,sample_rate)
    assert np.max(np.abs(centered[0]-centered[1]))<1e-7
    p.forge_ceiling_db=-6
    loud=p(probe*20,sample_rate)
    assert np.max(np.abs(loud))<=10**(float(p.forge_ceiling_db)/20)+1e-6
    assert p(probe[:1],sample_rate).shape==probe[:1].shape
    p.bypass=True
    assert np.max(np.abs(p(probe,sample_rate)-probe))<1e-6
    p.bypass=False
    assert np.max(np.abs(p(np.zeros_like(probe),sample_rate)))==0
    report["checks"].append(f"{sample_rate} Hz: Forge dry, drum shaping, width, ceiling, mono, bypass, silence")
p.forge_punch=-.63;p.forge_ceiling_db=-4.7
accepted={n:p.parameters[n].raw_value for n in ("forge_punch","forge_ceiling_db")}
for name in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled","relay_enabled"):setattr(p,name,True)
saved=p.raw_state;p.program="Snare press";p.raw_state=saved
assert all(abs(p.parameters[n].raw_value-v)<1e-7 for n,v in accepted.items())
assert all(getattr(p,n) for n in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled","relay_enabled","forge_enabled"))
report["checks"].append("Eight-effect rack, signed Punch and Ceiling survive state recall")
for version in ("drift-0.1.0","patina-0.2.0","atrium-0.3.0","chime-0.4.0","helix-0.5.0","gleam-0.6.0","relay-0.7.0"):
    p.forge_enabled=True
    p.raw_state=(Path(__file__).resolve().parents[1]/f"tests/fixtures/{version}.bapreset").read_bytes()
    assert not p.forge_enabled and abs(p.forge_punch-.3)<.002
    if version.startswith("relay"):assert p.relay_enabled and p.gleam_enabled
    report["checks"].append(f"Actual {version} state disables Forge and retains its saved rack")
# All audio is synthesized here: pitched kick, noise snare and short hats.
sr=48000;t=np.arange(sr*8)/sr;rng=np.random.default_rng(808)
kick_time=np.fmod(t,.5);snare_time=np.fmod(t+.25,.5);hat_time=np.fmod(t,.125)
kick=.24*np.exp(-kick_time*18)*np.sin(2*np.pi*(52*kick_time+7*(1-np.exp(-kick_time*45))))
noise=rng.normal(0,1,len(t))
snare=.1*np.exp(-snare_time*35)*(noise*.7+np.sin(2*np.pi*180*t)*.3)
hat=.025*np.exp(-hat_time*130)*(noise-np.roll(noise,1))
drums=np.stack((kick+snare+hat,kick+snare+hat*.7)).astype(np.float32)
sf.write(args.output/"30-original-drums.wav",drums.T,sr,subtype="PCM_24")
for index,name in enumerate(("First strike","Heavy floor","Snare press","Soft mallet","Parallel iron")):
    p=load_plugin(str(args.plugin));p.program=name
    start=time.perf_counter();shaped=p(np.pad(drums,((0,0),(0,sr))),sr,buffer_size=257)
    assert np.isfinite(shaped).all() and np.max(np.abs(shaped))<1
    assert np.max(np.abs(shaped[:,-sr//2:]))<1e-7
    sf.write(args.output/f"{31+index:02}-forge-{name.lower().replace(' ','-')}.wav",shaped.T,sr,subtype="PCM_24")
    if index==0:report["forge_render_seconds_for_9_seconds"]=time.perf_counter()-start
report["checks"].append("Five Forge presets render original drums without clipping or residual tails")
for sample_rate in (44100,48000,96000):
    p=load_plugin(str(args.plugin));p.program="Fine grain"
    assert p.cinder_enabled and not any(getattr(p,n) for n in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled","relay_enabled","forge_enabled"))
    t=np.arange(sample_rate)/sample_rate
    probe=np.stack([.12*np.sin(2*np.pi*700*t)]*2).astype(np.float32)
    p.cinder_mix=0
    assert np.max(np.abs(p(probe,sample_rate)-probe))<1e-6
    p.cinder_mix=1;p.cinder_grit=.6;p.cinder_noise=.7
    texture=p(probe,sample_rate,buffer_size=137)
    assert np.isfinite(texture).all() and np.sqrt(np.mean((texture-probe)**2))>.005
    assert np.max(np.abs(texture[0]-texture[1]))>.001
    p.cinder_width=0
    centered=p(probe,sample_rate)
    assert np.max(np.abs(centered[0]-centered[1]))<1e-7
    assert p(probe[:1],sample_rate).shape==probe[:1].shape
    p.bypass=True
    assert np.max(np.abs(p(probe,sample_rate)-probe))<1e-6
    p.bypass=False
    assert np.max(np.abs(p(np.zeros_like(probe),sample_rate)))==0
    report["checks"].append(f"{sample_rate} Hz: Cinder dry, texture, stereo, width, mono, bypass, idle silence")
p.cinder_tone_focus_hz=731;p.cinder_decay_s=.43
accepted={n:p.parameters[n].raw_value for n in ("cinder_tone_focus_hz","cinder_decay_s")}
for name in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled","relay_enabled","forge_enabled"):setattr(p,name,True)
saved=p.raw_state;p.program="Ash cloud";p.raw_state=saved
assert all(abs(p.parameters[n].raw_value-v)<1e-7 for n,v in accepted.items())
assert all(getattr(p,n) for n in ("drift_enabled","patina_enabled","atrium_enabled","chime_enabled","helix_enabled","gleam_enabled","relay_enabled","forge_enabled","cinder_enabled"))
report["checks"].append("Nine-effect rack and Cinder focus/decay survive state recall")
for version in ("drift-0.1.0","patina-0.2.0","atrium-0.3.0","chime-0.4.0","helix-0.5.0","gleam-0.6.0","relay-0.7.0","forge-0.8.0"):
    p.cinder_enabled=True
    p.raw_state=(Path(__file__).resolve().parents[1]/f"tests/fixtures/{version}.bapreset").read_bytes()
    assert not p.cinder_enabled and abs(p.cinder_decay_s-.15)<.002
    if version.startswith("forge"):assert p.forge_enabled and p.relay_enabled
    report["checks"].append(f"Actual {version} state disables Cinder and retains its saved rack")
for index,name in enumerate(("Fine grain","Copper dust","Paper speaker","Ash cloud","Rough edge")):
    p=load_plugin(str(args.plugin));p.program=name
    start=time.perf_counter();textured=p(np.pad(drums,((0,0),(0,48000*9))),48000,buffer_size=257)
    assert np.isfinite(textured).all() and np.max(np.abs(textured))<1
    assert np.max(np.abs(textured[:,-48000:]))<1e-5
    sf.write(args.output/f"{36+index:02}-cinder-{name.lower().replace(' ','-')}.wav",textured.T,48000,subtype="PCM_24")
    if index==0:report["cinder_render_seconds_for_17_seconds"]=time.perf_counter()-start
report["checks"].append("Five original Cinder presets render drums with decayed noise tails and no clipping")
(args.output / "verification.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps(report, indent=2))
