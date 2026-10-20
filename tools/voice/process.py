#!/usr/bin/env python3
"""Voix de l'arbitre : prend les WAV produits par tools/voice/gen.sh (workflow GitHub
"Voix de l'arbitre") et prepare des echantillons pour le driver PCM4 de SGDK.

usage : process.py dossier_sources [voix]
  - coupe les silences, filtre les basses, compresse (voix criee), ajoute un leger
    echo de salle, normalise, reechantillonne a 16 kHz mono 16 bits
  - ecrit res/voice/<id>.wav
"""
import glob
import os
import sys
import wave

import numpy as np
from scipy.signal import butter, lfilter, resample_poly

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = sys.argv[1]
VOICE = sys.argv[2] if len(sys.argv) > 2 else "pt_BR-faber-medium"
OUT = os.path.join(ROOT, "res", "voice")
SR = 16000


def load(path):
    w = wave.open(path)
    sr = w.getframerate()
    a = np.frombuffer(w.readframes(w.getnframes()), np.int16).astype(float) / 32768
    if w.getnchannels() == 2:
        a = a.reshape(-1, 2).mean(1)
    return a, sr


def trim(a, sr, thr=0.03, pad=0.015):
    env = np.convolve(np.abs(a), np.ones(int(sr * 0.01)) / int(sr * 0.01), "same")
    idx = np.where(env > thr)[0]
    if not len(idx):
        return a
    s = max(0, idx[0] - int(pad * sr))
    e = min(len(a), idx[-1] + int(pad * sr))
    return a[s:e]


def process(a, sr):
    a = trim(a, sr)
    b, c = butter(2, 140 / (sr / 2), "high")
    a = lfilter(b, c, a)
    # voix "criee" : compression douce
    a = a / (np.abs(a).max() + 1e-9)
    a = np.tanh(2.4 * a) / np.tanh(2.4)
    # echo de salle (deux reflexions adoucies) + petite queue
    tail = int(0.22 * sr)
    y = np.concatenate([a, np.zeros(tail)])
    bl, al = butter(1, 2500 / (sr / 2), "low")
    for d, g in ((0.065, 0.22), (0.13, 0.12), (0.2, 0.06)):
        n = int(d * sr)
        e = np.zeros_like(y)
        e[n:] = y[:len(y) - n]
        y = y + g * lfilter(bl, al, e)
    # fondu de fin
    f = int(0.05 * sr)
    y[-f:] *= np.linspace(1, 0, f)
    y = resample_poly(y, SR, sr)
    y = y / (np.abs(y).max() + 1e-9) * 0.97
    return y


def main():
    os.makedirs(OUT, exist_ok=True)
    files = sorted(glob.glob(os.path.join(SRC, "*__%s.wav" % VOICE)))
    for f in files:
        vid = os.path.basename(f).split("__")[0]
        a, sr = load(f)
        y = process(a, sr)
        with wave.open(os.path.join(OUT, vid + ".wav"), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(SR)
            w.writeframes((y * 32767).astype(np.int16).tobytes())
        print("%-16s %.2f s" % (vid, len(y) / SR))


if __name__ == "__main__":
    main()
