#!/usr/bin/env python3
"""Convertit un morceau en musique d'ecran titre Mega Drive (driver PCM SGDK, 13,4 kHz).

    python3 tools/convert_music.py morceau.mp3 DEBUT_BOUCLE FIN_BOUCLE

  - de 0 a DEBUT_BOUCLE (secondes) : intro jouee une seule fois
  - de DEBUT_BOUCLE a FIN_BOUCLE   : partie jouee en boucle (coupe sur un temps fort)

Sorties : res/music/title_intro.wav et res/music/title_loop.wav
Prerequis : ffmpeg, python3 avec numpy et scipy.
Exemple utilise pour la version actuelle : 11.31 19.681
"""
import os
import subprocess
import sys

import numpy as np
from scipy import signal
from scipy.io import wavfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SR_IN, SR = 44100, 13400


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(1)
    src, a, b = sys.argv[1], float(sys.argv[2]), float(sys.argv[3])

    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", src, "-vn", "-ac", "1", "-ar", str(SR_IN),
                          "-t", str(b + 1), "-f", "f32le", "-"], check=True, capture_output=True).stdout
    x = np.frombuffer(raw, dtype=np.float32).astype(np.float64)

    # filtrage : rien sous 45 Hz, rien au-dessus de ce que 13,4 kHz peut rendre
    x = signal.lfilter(*signal.butter(2, 45 / (SR_IN / 2), "high"), x)
    x = signal.filtfilt(*signal.butter(6, 5800 / (SR_IN / 2), "low"), x)
    y = signal.resample_poly(x, 134, 441)

    # compression douce pour garder du volume sur le DAC 8 bits
    env = signal.filtfilt(*signal.butter(1, 8 / (SR / 2)), np.abs(y))
    y = y * np.where(env > 0.12, (0.12 / np.maximum(env, 1e-6)) ** 0.5, 1.0)

    ia = int(round(a * SR))
    nloop = int(round((b - a) * SR)) // 256 * 256          # le driver lit par blocs de 256
    intro, loop = y[:ia].copy(), y[ia:ia + nloop].copy()
    peak = max(np.abs(intro).max() if len(intro) else 0, np.abs(loop).max())
    intro *= 0.95 / peak
    loop *= 0.95 / peak

    f = int(0.004 * SR)                                     # micro-fondus anti-clic
    loop[:f] *= np.linspace(0, 1, f)
    loop[-f:] *= np.linspace(1, 0, f)
    if len(intro) > f:
        intro[-f:] *= np.linspace(1, 0, f)
    intro = np.concatenate([intro, np.zeros((-len(intro)) % 256)])
    if len(intro) == 0:
        intro = np.zeros(256)

    out = os.path.join(ROOT, "res", "music")
    os.makedirs(out, exist_ok=True)
    for name, d in (("title_intro", intro), ("title_loop", loop)):
        wavfile.write(os.path.join(out, name + ".wav"), SR, (d * 32767).astype(np.int16))
        print(f"{name}: {len(d) / SR:.2f} s, {len(d) // 1024} Ko en ROM")


if __name__ == "__main__":
    main()
