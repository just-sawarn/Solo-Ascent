#!/usr/bin/env python3 -I
"""Prepare Quaternius 'Universal Base Characters [Standard]' (CC0) for the web build.
 - drops normal / metal-rough texture references (toon shader samples base colour only)
 - downsizes base-colour textures with macOS `sips` (the web budget is ~0.5 MB/texture; KTX2 comes with the asset pipeline)
Usage: prep_characters.py <extracted UBC dir> <out dir>"""
import json, os, shutil, subprocess, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
tex_dir = os.path.join(src, "Base Characters", "Textures")
HAIR_DIR = os.path.join(src, "Hairstyles", "Origin at 0", "glTF (Godot)")
BODY_DIR = os.path.join(src, "Base Characters", "Godot - UE")

def resize(srcfile, dstname, px):
    dst = os.path.join(out, dstname)
    subprocess.run(["sips", "-Z", str(px), srcfile, "--out", dst], check=True, stdout=subprocess.DEVNULL)

def slim(gltf_in, gltf_out, uri_map):
    j = json.load(open(gltf_in))
    for m in j.get("materials", []):
        m.pop("normalTexture", None)
        pbr = m.get("pbrMetallicRoughness", {})
        pbr.pop("metallicRoughnessTexture", None)
    for img in j.get("images", []):
        img["uri"] = uri_map.get(img["uri"], img["uri"])
    json.dump(j, open(gltf_out, "w"), separators=(",", ":"))

# textures (512 px). Both skin tones for each body; hair maps are greyscale-tinted at runtime.
resize(os.path.join(tex_dir, "T_Superhero_Male_Dark.png"), "skin_male_dark.png", 512)
resize(os.path.join(tex_dir, "T_Superhero_Male_Ligh.png"), "skin_male_light.png", 512)
resize(os.path.join(tex_dir, "T_Superhero_Female_Dark_BaseColor.png"), "skin_female_dark.png", 512)
resize(os.path.join(tex_dir, "T_Superhero_Female_Light_BaseColor.png"), "skin_female_light.png", 512)
resize(os.path.join(tex_dir, "T_Hair_1_BaseColor.png"), "hair1.png", 512)
resize(os.path.join(tex_dir, "T_Hair_2_BaseColor.png"), "hair2.png", 512)
shutil.copy(os.path.join(tex_dir, "T_Eye_Brown.png"), os.path.join(out, "eye_brown.png"))

for sex, skin in (("Male", "skin_male_dark.png"), ("Female", "skin_female_dark.png")):
    base = "Superhero_%s_FullBody" % sex
    slim(os.path.join(BODY_DIR, base + ".gltf"), os.path.join(out, "body_%s.gltf" % sex.lower()),
         {"T_Superhero_%s_Dark.png" % sex: skin, "T_Superhero_Female_Dark_BaseColor.png": skin,
          "T_Hair_1_BaseColor.png": "hair1.png", "T_Hair_2_BaseColor.png": "hair2.png", "T_Eye_Brown.png": "eye_brown.png"})
    shutil.copy(os.path.join(BODY_DIR, base + ".bin"), os.path.join(out, "body_%s.bin" % sex.lower()))
    # bin uri must follow the rename
    p = os.path.join(out, "body_%s.gltf" % sex.lower())
    j = json.load(open(p)); j["buffers"][0]["uri"] = "body_%s.bin" % sex.lower(); json.dump(j, open(p, "w"), separators=(",", ":"))

for name in ("Hair_SimpleParted", "Hair_Buzzed", "Hair_Long", "Hair_Beard", "Eyebrows_Regular", "Eyebrows_Female"):
    p = os.path.join(out, name.lower() + ".gltf")
    slim(os.path.join(HAIR_DIR, name + ".gltf"), p, {"T_Hair_1_BaseColor.png": "hair1.png", "T_Hair_2_BaseColor.png": "hair2.png"})
    shutil.copy(os.path.join(HAIR_DIR, name + ".bin"), os.path.join(out, name.lower() + ".bin"))
    j = json.load(open(p)); j["buffers"][0]["uri"] = name.lower() + ".bin"; json.dump(j, open(p, "w"), separators=(",", ":"))
print("ok")
