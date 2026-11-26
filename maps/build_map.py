"""Build the Hungarian radar basemap using only Python's standard library."""
import json
import math
import struct
import sys
import unicodedata
import zipfile
from pathlib import Path

source = Path(sys.argv[1])
overview = "--overview" in sys.argv[2:]
scale = 2000 if overview else 5000
places, paths, points = [], [], []
names, offsets = bytearray(), {}


def name_offset(name):
    # The display's built-in fonts use ASCII.
    name = unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode()
    if name not in offsets:
        offsets[name] = len(names)
        names.extend(name.encode() + b"\0")
    return offsets[name]


def quantize(coord):
    return (round((coord[0] - 19) * scale), round((coord[1] - 47) * scale))


def inside(p):
    if overview:
        return 5 <= p[0] <= 33 and 39 <= p[1] <= 57
    return 15.5 <= p[0] <= 23.25 and 45.25 <= p[1] <= 49.0


def simplify(line, tolerance):
    if len(line) < 3:
        return line
    x, y = line[0]
    dx, dy = line[-1][0] - x, line[-1][1] - y
    norm = dx*dx + dy*dy
    worst, pivot = 0, 0
    for i, (px, py) in enumerate(line[1:-1], 1):
        t = max(0, min(1, ((px-x)*dx + (py-y)*dy)/norm)) if norm else 0
        err = (px-x-t*dx)**2 + (py-y-t*dy)**2
        if err > worst:
            worst, pivot = err, i
    if worst <= tolerance**2:
        return [line[0], line[-1]]
    return simplify(line[:pivot+1], tolerance)[:-1] + simplify(line[pivot:], tolerance)


def add_path(line, kind, name=0):
    coords = simplify([quantize(p) for p in line], 20 if overview else 1.6)
    if len(coords) < (4 if kind >= 4 else 2):
        return False
    xs, ys = zip(*coords)
    paths.append((min(xs), min(ys), max(xs), max(ys), len(points), len(coords), kind, name))
    points.extend(coords)
    return True


for stem, kind in [("rivers_lake_centerlines", 1), ("lakes", 2), ("admin_0_boundary_lines_land", 3)]:
    for feature in json.loads((source / (stem+".json")).read_bytes())["features"]:
        geom = feature["geometry"]
        gtype, lines = geom["type"], geom["coordinates"]
        if gtype == "LineString":
            lines = [lines]
        elif gtype == "MultiPolygon":
            lines = [ring for polygon in lines for ring in polygon]
        for line in lines:
            local = []
            for p in line:
                if inside(p):
                    local.append(p)
                elif local:
                    add_path(local, kind)
                    local = []
            if local:
                add_path(local, kind)

for feature in ([] if overview else json.loads((source / "settlements.json").read_bytes())["features"]):
    name = name_offset(feature["properties"]["NAME"])
    geom = feature["geometry"]
    polygons = [geom["coordinates"]] if geom["type"] == "Polygon" else geom["coordinates"]
    for rings in polygons:
        if add_path(rings[0], 4, name):
            for hole in rings[1:]:
                add_path(hole, 5, name)

seen = set()
for stem in (["cities500"] if overview else ["HU", "cities500"]):
    with zipfile.ZipFile(source / (stem+".zip")) as archive, archive.open(stem+".txt") as rows:
        for row in rows:
            f = row.decode().split("\t")
            if f[0] in seen or f[7] not in {"PPL", "PPLA", "PPLA2", "PPLA3", "PPLA4", "PPLC"}:
                continue
            coord = (float(f[5]), float(f[4]))
            if not inside(coord) or int(f[14] or 0) < (25000 if overview else 1):
                continue
            seen.add(f[0])
            places.append((int(f[14]), *quantize(coord), name_offset(f[2])))

places.sort(reverse=True)
pack = bytearray(struct.pack("<4s4I", b"FDM1", len(places), len(paths), len(points), len(names)))
for _, x, y, name in places:
    pack.extend(struct.pack("<hhI", x, y, name))
for path in paths:
    pack.extend(struct.pack("<4hIHHI", *path))
for p in points:
    pack.extend(struct.pack("<hh", *p))
pack.extend(names)
target = Path(__file__).with_name("central-europe.fdm" if overview else "hungary.fdm")
target.write_bytes(pack)
print(f"{len(places)} hely, {len(paths)} vonal/gyuru, {len(points)} pont, {len(pack)} bajt")
