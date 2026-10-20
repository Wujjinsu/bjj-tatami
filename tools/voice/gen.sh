#!/bin/bash
# Genere les voix de l'arbitre (portugais du Bresil) avec plusieurs synthetiseurs.
# Lance par le workflow .github/workflows/voices.yml (les runners GitHub ont acces a Internet).
set -e
OUT=${1:-voices_out}
mkdir -p "$OUT"
VOICES="pt_BR-faber-medium pt_BR-cadu-medium pt_BR-jeff-medium pt_BR-edresson-low"
for v in $VOICES; do python3 -m piper.download_voices "$v" || true; done
ls -la
while IFS='|' read -r id txt; do
  [ -z "$id" ] && continue
  for v in $VOICES; do
    [ -f "$v.onnx" ] || continue
    python3 -m piper -m "$v.onnx" --length-scale 0.85 -f "$OUT/${id}__${v}.wav" -- "$txt" || echo "echec $v $id"
  done
  espeak-ng -v pt-br -s 150 -p 35 -a 180 -w "$OUT/${id}__espeak.wav" "$txt" || true
done < tools/voice/phrases.txt
ls "$OUT" | wc -l
