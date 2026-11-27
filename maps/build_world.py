"""Build WORLD.FDM for the SD card. Requires Shapely 2 and the inputs in README.md."""
import argparse
from collections import defaultdict
import hashlib
import json
import math
from pathlib import Path
import sqlite3
import struct
import unicodedata
import zipfile
import zlib

from shapely import from_wkb
from shapely.geometry import box, LineString, shape


def parts(geometry, kind):
    if geometry.is_empty:
        return
    if geometry.geom_type == kind:
        yield geometry
    elif geometry.geom_type.startswith("Multi") or geometry.geom_type == "GeometryCollection":
        for piece in geometry.geoms:
            yield from parts(piece, kind)


def label(value):
    return unicodedata.normalize("NFKD", value or "").encode("ascii", "ignore").strip()[:95]


def tile_at(lon, lat, step):
    return (min(360//step-1, max(0, math.floor((lon+180)/step))),
            min(180//step-1, max(0, math.floor((lat+90)/step))))


class Tile:
    def __init__(self):
        self.places = []
        self.paths = []

    def encode(self, cx, cy):
        names, offsets = bytearray(b"\0"), {b"": 0}
        def name_ref(name):
            if name not in offsets:
                offsets[name] = len(names)
                names.extend(name + b"\0")
            return offsets[name]
        def xy(x, y):
            return round((x-cx)*5000), round((y-cy)*5000)

        def varuint(value):
            while value >= 128:
                stream.append(128 + value % 128)
                value //= 128
            stream.append(value)
        stream = bytearray()
        for _, lon, lat, name in sorted(self.places, reverse=True):
            stream.extend(struct.pack("<hh", *xy(lon, lat)))
            varuint(name_ref(name))
        point_count=0
        for kind, name, line in self.paths:
            stream.append(kind)
            varuint(len(line))
            if kind>=6:
                varuint(name_ref(name))
            old_x=old_y=0
            for lon, lat in line:
                x,y=xy(lon,lat)
                x,y=round(x/5),round(y/5)
                for delta in (x-old_x,y-old_y):
                    varuint(2*delta if delta>=0 else -2*delta-1)
                old_x,old_y=x,y
            point_count+=len(line)
        raw = struct.pack("<4s4I", b"FDM1",len(self.places),len(self.paths),point_count,len(names))
        raw += stream + names
        if max(len(raw),20+8*len(self.places)+20*len(self.paths)+4*point_count+len(names)) > 256*1024:
            raise ValueError("Tile exceeds device memory limit")
        return struct.pack("<I", len(raw)) + zlib.compress(raw, 9)


def build(source, output):
    grids = {2: defaultdict(Tile), 10: defaultdict(Tile)}
    def distribute(geom, step, kind, name=b""):
        x0, y0 = tile_at(*geom.bounds[:2], step)
        x1, y1 = tile_at(*geom.bounds[2:], step)
        for y in range(y0, y1+1):
            for x in range(x0, x1+1):
                left, bottom = x*step-180, y*step-90
                clipped = geom if x0 == x1 and y0 == y1 else geom.intersection(box(left, bottom, left+step, bottom+step))
                for piece in parts(clipped, "Polygon" if kind == 6 else "LineString"):
                    paths = grids[step][x,y].paths
                    if kind == 6:
                        paths.append((6, name, list(piece.exterior.coords)))
                        for hole in piece.interiors:
                            paths.append((7, name, list(hole.coords)))
                    else:
                        paths.append((kind, name, list(piece.coords)))

    with zipfile.ZipFile(source/"cities500.zip") as archive, archive.open("cities500.txt") as rows:
        total = 0
        for raw in rows:
            f = raw.decode().split("\t")
            if f[6] != "P" or f[7] in {"PPLH","PPLQ","PPLW","PPLX","PPLS"}:
                continue
            name, lon, lat, pop = label(f[2] or f[1]), float(f[5]), float(f[4]), int(f[14] or 0)
            if not name:
                continue
            total += 1
            for step, tiles in grids.items():
                if step == 2 or pop >= 25000 or f[7] == "PPLC":
                    tiles[tile_at(lon,lat,step)].places.append((pop,lon,lat,name))

    for stem, kind in [("coastline",3),("admin_0_boundary_lines_land",3),("rivers_lake_centerlines",1),("lakes",2)]:
        for feature in json.loads((source/(stem+".json")).read_bytes())["features"]:
            geometry = shape(feature["geometry"])
            if "Polygon" in geometry.geom_type:
                geometry = geometry.boundary
            for step in grids:
                for line in parts(geometry.simplify(.008 if step==2 else .08), "LineString"):
                    coords=list(line.coords)
                    for start in range(0,len(coords)-1,96):
                        segment=coords[start:start+97]
                        # Longitude jumps cross the seam, never the entire world.
                        begin=0
                        for end in range(1,len(segment)+1):
                            if end==len(segment) or abs(segment[end][0]-segment[end-1][0])>180:
                                if end-begin>=2:
                                    distribute(LineString(segment[begin:end]),step,kind)
                                begin=end

    with sqlite3.connect(f"file:{(source/'world-urban.gpkg').resolve().as_posix()}?mode=ro",uri=True) as db:
        sql = "SELECT UC_NM_MN,geom FROM GHS_STAT_UCDB2015MT_GLOBE_R2019A_V1_2 ORDER BY fid"
        urban=0
        for value, blob in db.execute(sql):
            name=label(value)
            if not name or name in {b"N/A",b"-"}:
                continue
            envelope = (blob[3]>>1)&7
            geometry = from_wkb(blob[8+(0,32,48,48,64)[envelope]:])
            for polygon in parts(geometry, "Polygon"):
                if polygon.bounds[2]-polygon.bounds[0]>180:
                    continue
                simple=polygon.simplify(.0008)
                if not simple.is_valid:
                    simple=simple.buffer(0)
                distribute(simple,2,6,name)
            urban+=1

    offsets, content = [], bytearray()
    count=sum(360//s*(180//s) for s in grids)+2
    base=16+4*(count+1)
    for step, tiles in grids.items():
        for y in range(180//step):
            for x in range(360//step):
                offsets.append(base+len(content))
                if (x,y) in tiles:
                    content.extend(tiles[x,y].encode((x+.5)*step-180,(y+.5)*step-90))
    for name in ("hungary.fdm","central-europe.fdm"):
        offsets.append(base+len(content))
        raw=Path(__file__).with_name(name).read_bytes()
        content.extend(struct.pack("<I",len(raw))+zlib.compress(raw,9))
    offsets.append(base+len(content))
    index=struct.pack(f"<{len(offsets)}I",*offsets)
    header=struct.pack("<4sIII",b"FDW2",offsets[-1],count,zlib.crc32(index))
    pack=header+index+content
    digest=hashlib.sha256(pack).hexdigest()
    manifest=output.with_name("world.json")
    previous=json.loads(manifest.read_bytes()) if manifest.exists() else {}
    revision=previous.get("revision",0)+int(digest!=previous.get("sha256"))
    output.write_bytes(pack)
    manifest.write_text(json.dumps(dict(format="FDW2",revision=revision,size=len(pack),sha256=digest),
                                   indent=2)+"\n",encoding="utf-8")
    print(f"{total} settlements, {urban} urban centres, {offsets[-1]} bytes")


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source",type=Path)
    parser.add_argument("output",type=Path)
    args=parser.parse_args()
    build(args.source,args.output)
