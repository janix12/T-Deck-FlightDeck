# Maps and credits · Térképek és források

[English guide](../README.md) · [Magyar útmutató](../README.hu.md)

### Offline world map · Offline világtérkép

**EN** · FlightDeck clients with FDW2 support download the world map independently of firmware. Revision 1 is 5,835,336 bytes and includes 226,439 settlement points, coastlines, national borders, rivers, lakes, the Hungarian municipal atlas and the Central European weather overview. The flight card shows a containing Hungarian municipality where available. Abroad it distinguishes the nearest named settlement within 30 km from the containing built-up urban area. One urban area can span several towns. A point over open ocean can have no nearby settlement. To install with a card reader, place `WORLD.FDM` in the card root.

**HU** · Az FDW2 formátumot támogató FlightDeck a firmware-től külön tölti le a térképet. Az 1. térképkiadás 5 835 336 bájt, benne 226 439 településpont, partvonalak, országhatárok, folyók, tavak, a részletes magyar településhatárok és a közép-európai időjárási alaptérkép. A gép adatlapján Magyarországon a befoglaló település jelenik meg, ahol a határ ismert. Külföldön a 30 km-en belüli legközelebbi település és a befoglaló beépített várostérség külön adat. Egy várostérség több településre is kiterjedhet. Nyílt tengeren előfordul, hogy nincs közeli település. Kártyaolvasós telepítéshez a `WORLD.FDM` fájlt a kártya gyökerébe másold.

[Map file · Térképfájl](WORLD.FDM) · [Revision and SHA-256 · Verzió és SHA-256](world.json)

**Data credits · Adatforrások** — Settlement points are a subset of GeoNames cities500, licensed under CC BY 4.0. Coastlines and hydrography use Natural Earth public-domain vectors. Urban footprints were simplified, tiled and quantised from Florczyk et al. (2019), GHS Urban Centre Database 2015 R2019A v1.2, European Commission, Joint Research Centre. The footprints refer to 2015. A várostérségek a JRC 2015-ös állapotot leíró adatbázisából származnak, egyszerűsítve és térképszelvényekre osztva. [GeoNames](https://www.geonames.org/export/) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · [Natural Earth](https://www.naturalearthdata.com/about/terms-of-use/) · [JRC dataset and citation](https://data.jrc.ec.europa.eu/dataset/53473144-b88c-44bc-b4a3-4583ed1f547e)

**Build · Előállítás** — Install Python and Shapely 2. Add the Natural Earth 1:10m coastline as `coastline.json` to the input directory below. Extract the full polygon GeoPackage from the JRC v1.2 archive there as `world-urban.gpkg`. Futtasd a következő parancsot a projekt gyökeréből a bemeneti adatok és a két regionális térképfájl előkészítése után. [Coastline source](https://raw.githubusercontent.com/nvkelso/natural-earth-vector/ca96624a56bd078437bca8184e78163e5039ad19/geojson/ne_10m_coastline.geojson) · [JRC archive](https://jeodpp.jrc.ec.europa.eu/ftp/jrc-opendata/GHSL/GHS_STAT_UCDB2015MT_GLOBE_R2019A/V1-2/)

```powershell
py maps/build_world.py map-input maps/WORLD.FDM
```

**Publishing updates · Térképfrissítés közzététele** — The builder also writes `world.json`. Changed map bytes increment the map revision. Publish the manifest and `WORLD.FDM` in the same Git commit. A generátor változó tartalomnál növeli a térkép verzióját. A verziófájlt és a térképet azonos Git-commitban tedd közzé. A kliens induláskor, majd 24 óránként ellenőrzi a verziófájlt.

**EN** · `hungary.fdm` contains simplified Hungarian municipal boundaries, settlement names, water and country borders. `central-europe.fdm` is the weather overview, covering 5–33° E and 39–57° N. Both are built into the firmware. On a writable card the firmware can install and verify a copy of the detailed map. The weather frames are downloaded separately.

**HU** · A `hungary.fdm` egyszerűsített magyar településhatárokat, helységneveket, vízrajzot és országhatárokat tartalmaz. A `central-europe.fdm` az időjárási áttekintőtérkép, az 5–33° keleti hosszúság és 39–57° északi szélesség közötti területre. Mindkettő beépül a firmware-be. Írható kártyára a részletes térkép másolatát a készülék telepíti és ellenőrzi. Az időjárási képkockákat külön tölti le.

| Component / Összetevő | Attribution and changes / Forrás és módosítás |
|---|---|
| Hungarian municipalities / Magyar településhatárok | Simplified and quantised / Egyszerűsített és kerekített koordináták. [czinkos](https://gist.github.com/czinkos/bedb669ca606627780483e1949ad4bb4) via [wuerdo/geoHungary](https://github.com/wuerdo/geoHungary/blob/58cff8842690d5537254c641027ff0b9880ec539/settlements.geojson) |
| Settlement points / Településpontok | Regional subset with ASCII names / Területi kivágat egyszerűsített írásmódú nevekkel. [GeoNames](https://www.geonames.org/export/) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Water and national borders / Vízrajz és országhatárok | Clipped and simplified from 1:10m data / Az 1:10 milliós adatokból kivágva és egyszerűsítve. [Natural Earth](https://github.com/nvkelso/natural-earth-vector/tree/ca96624a56bd078437bca8184e78163e5039ad19/geojson) · [Public domain](https://www.naturalearthdata.com/about/terms-of-use/) |
| Aircraft silhouette / Repülősziluett | Adapted to a rotating pixel icon / Forgatható pixelikonná alakítva. [Pictogrammers Material Design Icons, airplane](https://pictogrammers.com/library/mdi/icon/airplane/) · [Apache 2.0](../licenses/Apache-2.0.txt) |
| Board pin definitions / Alaplapi lábkiosztás | Shenzhen Xin Yuan Electronic Technology Co., Ltd., 2023. [LilyGO utilities.h](https://github.com/Xinyuan-LilyGO/T-Deck/blob/9aac147a313975ad063594b3fe11e28a96d94936/examples/UnitTest/utilities.h) · [MIT](../licenses/LilyGO-board-MIT.txt) |

## Optional rebuild · Újragenerálás igény szerint

**EN** · For a normal firmware build use the included packs. To generate new packs, install Python 3 and place these inputs in a folder named `map-input` in the project root. Keep the ZIPs compressed. For the Natural Earth inputs, use the pinned revision linked above.

**HU** · A szokásos firmware-fordításhoz a mellékelt csomagok elegendők. Új csomagok készítéséhez telepítsd a Python 3-at, és az alábbi fájlokat helyezd a projekt gyökerében lévő `map-input` mappába. A ZIP-eket hagyd tömörítve. A Natural Earth fájljaihoz a fent hivatkozott revíziót használd.

| Download / Letöltés | Local name / Helyi fájlnév |
|---|---|
| [GeoNames Hungary](https://download.geonames.org/export/dump/HU.zip) | `HU.zip` |
| [GeoNames cities500](https://download.geonames.org/export/dump/cities500.zip) | `cities500.zip` |
| geoHungary `settlements.geojson` | `settlements.json` |
| Natural Earth `ne_10m_rivers_lake_centerlines.geojson` | `rivers_lake_centerlines.json` |
| Natural Earth `ne_10m_lakes.geojson` | `lakes.json` |
| Natural Earth `ne_10m_admin_0_boundary_lines_land.geojson` | `admin_0_boundary_lines_land.json` |

```powershell
py maps/build_map.py map-input
py maps/build_map.py map-input --overview
```

**EN** · Recompile with a fresh build cache after changing a pack. The assembly file embeds the binary map at compile time.

**HU** · Módosítás után üres fordítási gyorsítótárral fordíts újra. Az assembly fájl fordításkor ágyazza be a térképcsomagot.
