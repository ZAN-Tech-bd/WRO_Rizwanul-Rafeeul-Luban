#!/usr/bin/env bash
# =====================================================================
#  generate_tracks.sh
#  Deterministic generator for the 14 DFPlayer Mini placeholder audio
#  tracks used by the Smart Museum Guidance System.
#
#  Output: ./mp3/0001.mp3 .. ./mp3/0014.mp3
#    - Tracks 0001..0007 : English station cues (Flite TTS + bell)
#    - Tracks 0008..0014 : Bangla station cues (distinct chime motif,
#                          NO speech, because no local Bangla voice exists)
#
#  All audio is 100% originally synthesized in-process by FFmpeg's
#  lavfi sources (aevalsrc damped sinusoids + flite English TTS).
#  No external/copyrighted sound is read, downloaded, or wrapped.
#
#  Encoding target (per DFRobot DFR0299 + project findings):
#    MP3, MPEG-1 Layer III, CBR 128 kb/s, 44.1 kHz, mono, no artwork,
#    no metadata tags.
#
#  License of generated assets: the project dedicates these synthesized
#  cues to the public domain via CC0 1.0 (see ../PROVENANCE.md). The
#  FFmpeg/Flite engines are third-party software under their own
#  licenses and are NOT part of the audio asset license.
# =====================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$HERE/mp3"
TMP="$HERE/.gen_tmp"
mkdir -p "$OUT" "$TMP"

# --- Station data (must match webapp/js/data.js ARTIFACTS order) ------
STATION_IDS=(jamdani nakshi liberation terracotta ektara palki tiger)
EN_NAMES=(
  "Jamdani Saree"
  "Nakshi Kantha"
  "Liberation War Artifacts"
  "Terracotta Plaques"
  "Ektara"
  "Palki Palanquin"
  "Royal Bengal Tiger"
)
# Pentatonic root frequency (Hz) per station 1..7: C4 D4 E4 G4 A4 C5 D5
ROOT_FREQ=(261.63 293.66 329.63 392.00 440.00 523.25 587.33)
FLITE_VOICE="slt"   # clear English female voice, consistent across EN tracks

# Shared encoder settings
SR=44100
CH=mono
BITRATE=128k

# ---------------------------------------------------------------------
# gen_bell <freq_hz> <duration_s> <outfile.wav>
#   A damped (bell-like) sinusoid. Peak amplitude 0.6 (no clipping),
#   exponential decay gives a clean attack/sustain-free chime.
# ---------------------------------------------------------------------
gen_bell() {
  local freq="$1" dur="$2" out="$3"
  ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i "aevalsrc=0.6*exp(-t*3.2)*sin(2*PI*${freq}*t):d=${dur}:s=${SR}:c=${CH}" \
    -af "aformat=sample_fmts=fltp:sample_rates=${SR}:channel_layouts=${CH}" \
    -c:a pcm_s16le "$out"
}

# ---------------------------------------------------------------------
# gen_silence <duration_s> <outfile.wav>
# ---------------------------------------------------------------------
gen_silence() {
  local dur="$1" out="$2"
  ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i "aevalsrc=0:d=${dur}:s=${SR}:c=${CH}" \
    -af "aformat=sample_fmts=fltp:sample_rates=${SR}:channel_layouts=${CH}" \
    -c:a pcm_s16le "$out"
}

# ---------------------------------------------------------------------
# gen_speech <text> <outfile.wav>   (English, Flite slt voice)
# ---------------------------------------------------------------------
gen_speech() {
  local text="$1" out="$2"
  ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i "flite=text='${text}':v=${FLITE_VOICE}" \
    -af "aformat=sample_fmts=fltp:sample_rates=${SR}:channel_layouts=${CH}" \
    -c:a pcm_s16le "$out"
}

# ---------------------------------------------------------------------
# encode <concat_list_file> <out_mp3_path>
#   Concatenate segment wavs, apply modest gain + safety limiter, then
#   encode to the DFPlayer target profile. -map_metadata -1 strips all
#   tags; -id3v2_version 0 + -write_xing 0 keeps the file header-light.
# ---------------------------------------------------------------------
encode() {
  local list="$1" out="$2"
  ffmpeg -hide_banner -loglevel error -y \
    -f concat -safe 0 -i "$list" \
    -af "aformat=sample_fmts=fltp:sample_rates=${SR}:channel_layouts=${CH},\
volume=0.85,alimiter=limit=0.80:attack=5:release=50" \
    -c:a libmp3lame -b:a "${BITRATE}" -ar "${SR}" -ac 1 \
    -map_metadata -1 -id3v2_version 0 -write_xing 0 \
    "$out"
}

# Empty the segment list helper
new_list() { : > "$TMP/concat.txt"; }
add() { echo "file '$1'" >> "$TMP/concat.txt"; }

# =====================================================================
#  ENGLISH tracks 0001..0007  (bell + speech + bell)
# =====================================================================
for i in 0 1 2 3 4 5 6; do
  track=$((i+1))
  num=$(printf "%04d" "$track")
  root="${ROOT_FREQ[$i]}"
  name="${EN_NAMES[$i]}"
  stn=$((i+1))
  tmpdir="$TMP/en_${num}"; mkdir -p "$tmpdir"

  gen_bell "$root" 0.60 "$tmpdir/open.wav"
  gen_silence      0.15 "$tmpdir/gap.wav"
  gen_speech "Station ${stn}. ${name}." "$tmpdir/speech.wav"
  gen_bell "$root" 0.60 "$tmpdir/close.wav"

  new_list
  add "$tmpdir/open.wav"
  add "$tmpdir/gap.wav"
  add "$tmpdir/speech.wav"
  add "$tmpdir/close.wav"
  encode "$TMP/concat.txt" "$OUT/${num}.mp3"
  echo "EN  ${num}.mp3  station ${stn}  root=${root}Hz  '${name}'"
done

# =====================================================================
#  BANGLA tracks 0008..0014  (distinct chime motif, NO speech)
#  Motif: ascending root -> fifth -> octave, played twice with a short
#  gap. This is deliberately non-verbal so it is never a mispronounced
#  placeholder for Bangla; root pitch encodes the station identity.
# =====================================================================
for i in 0 1 2 3 4 5 6; do
  track=$((i+8))
  num=$(printf "%04d" "$track")
  root="${ROOT_FREQ[$i]}"
  stn=$((i+1))
  tmpdir="$TMP/bn_${num}"; mkdir -p "$tmpdir"

  # equal-temperament multipliers
  fifth=$(awk  -v r="$root" 'BEGIN{printf "%.3f", r*1.498307}')
  octav=$(awk  -v r="$root" 'BEGIN{printf "%.3f", r*2.0}')

  gen_bell "$root"  0.55 "$tmpdir/n1.wav"
  gen_bell "$fifth" 0.55 "$tmpdir/n2.wav"
  gen_bell "$octav" 0.70 "$tmpdir/n3.wav"
  gen_silence      0.30 "$tmpdir/gap.wav"

  new_list
  # pass 1
  add "$tmpdir/n1.wav"; add "$tmpdir/n2.wav"; add "$tmpdir/n3.wav"
  add "$tmpdir/gap.wav"
  # pass 2 (call-response repeat)
  add "$tmpdir/n1.wav"; add "$tmpdir/n2.wav"; add "$tmpdir/n3.wav"
  encode "$TMP/concat.txt" "$OUT/${num}.mp3"
  echo "BN  ${num}.mp3  station ${stn}  root=${root}Hz  motif ${root}->${fifth}->${octav} (x2)"
done

# Clean intermediate segment wavs (keep the script itself + outputs)
rm -rf "$TMP"

echo "----"
echo "Done. Generated $(ls "$OUT" | wc -l) track(s) in $OUT"
