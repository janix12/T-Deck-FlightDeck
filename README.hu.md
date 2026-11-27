# FlightDeck a LilyGO T-Deckhez

**[USB-telepítő megnyitása →](https://janix12.github.io/T-Deck-FlightDeck/?lang=hu)** · [Kész firmware és kézi telepítés](https://github.com/janix12/T-Deck-FlightDeck/releases/latest)

Nyisd meg a telepítőt számítógépen Chrome-ban vagy Edge-ben, és csatlakoztasd az eredeti T-Decket USB-adatkábellel. Új eszköznél vagy másik rendszerről váltva válaszd az **Első telepítés** lehetőséget. Ha már FlightDeck fut rajta, a **FlightDeck frissítése** megtartja a beállításaidat. Kattints a **Csatlakozás és telepítés** gombra, majd válaszd ki az eszköz USB-portját.

A **Firmware-verzió** listában alapból a legfrissebb kiadás van kiválasztva. USB-s visszaállításhoz válassz egy korábbi verziót, hagyd bejelölve a **FlightDeck frissítése** módot, majd kattints a **Csatlakozás és telepítés** gombra. Az **Első telepítés** is a kiválasztott verziót írja fel, és felülírja a korábbi beállításokat.

**[English guide](README.md)** · [Hardver és kapcsolási rajzok](docs/hardware.md) · [Térképadatok és források](maps/README.md)

Nézd meg, melyik település fölött jár egy repülő. A FlightDeck a LilyGO T-Deck 320 × 240 pixeles kijelzőjén repülőtérképet, gépadatlapot, mozgó esőradart és hőmérsékleti térképet jelenít meg. A repülők ADS-B helyzetjelentései Wi-Fi-n érkeznek. Opcionális GPS-szel a térkép utazás közben követi a helyzetedet.

| Élő forgalom · 29 gép Budapest körül | Repülőlista · DLH6CH kiválasztva |
|:---:|:---:|
| ![A Légtér cím mellett az aktív Wi-Fi jele és a 29-es gépszám, a térképen repülőikonok látszanak.](docs/images/wifi131-traffic-hu.png) | ![A budapesti forgalomból kiválasztott DLH6CH járat a géplistában.](docs/images/wifi131-list-hu.png) |
| **Követés és Csak ez a gép** | **DLH6CH követése a teljes térképen** |
| ![A gépadatlapon bekapcsolt Követés és Csak ez a gép kapcsolók.](docs/images/wifi131-controls-hu.png) | ![A térkép közepén a követett DLH6CH járat ikonja és hívójele.](docs/images/wifi131-map-hu.png) |
| **Közép-európai esőradar** | **Felszínközeli léghőmérséklet** |
| ![A RainViewer csapadékrétege és az aktív Wi-Fi jele.](docs/images/wifi131-rain-hu.png) | ![Hőmérsékleti térkép Budapest körül.](docs/images/wifi131-heat-hu.png) |
| **Műszerek · v1.5.0** | **Beállítások · v1.4.0** |
| ![A szenzorkártyák alatt látszik a RAM és a PSRAM foglaltsága, valamint a két CPU-mag terhelése és a microSD szabad és teljes tárhelye.](docs/images/v150-instruments-hu.png) | ![A Kijelző és eszköz lap műveletikonjai.](docs/images/v140-settings-hu.png) |

A repülőforgalmi és időjárási képek a v1.3.1, a Műszerek képe a v1.5.0, a Beállítások képe a v1.4.0 kijelzőjéről származnak. A lapcím melletti három ív az aktív Wi-Fi-kapcsolatot mutatja. A radaron az ADS-B-lekéréskor észlelt 29 gép és a követésre kiválasztott DLH6CH járat látszik. A felület nyelve a **Beállítások → Nyelv** sorban váltható. [Korábbi példa az API-korlát jelzésére](docs/images/airspace.png)

| Térkép és géplista a trackballal | Térkép mozgatása érintéssel |
|:---:|:---:|
| ![A térkép elmozdul, majd a kijelölés végiggördül a géplistán. A középső kattintás megnyitja az ELY361 adatlapját.](docs/images/trackball-navigation-hu.gif) | ![Az ujj jobbra húzza a térképet, a + gombra koppintva nagyít, majd a Légtér lapcímre koppintva visszahozza Budapestet középre.](docs/images/touch-navigation-hu.gif) |
| Görgesd a golyót a térkép mozgatásához. Nyomd le a listához, görgess a járatra, majd nyomd le újra az adatlapjához. | Húzd el a térképet, majd koppints a **+** gombra. A **Légtér** lapcím visszaközpontosít, és megtartja a nagyítást. |

A T-Decken rögzített bemutatók egy korábbi budapesti forgalmi mintát használnak.

**Egyetlen gép követése**

![A DEMO101 kiválasztása és a Csak ez a gép bekapcsolása után bezárul az adatlap. A környező települések elmozdulnak, miközben a repülő középen marad.](docs/images/follow-aircraft-hu.gif)

A járat adatlapjának alján kapcsold be a **Csak ez a gép** kapcsolót. Ezzel a **Követés** is bekapcsol. Koppints a **×** gombra a teljes térképhez. A GIF gyorsított, szimulált útvonalon mutatja a DEMO101 mozgását.

**Esőradar Budapest körül**

![A csapadéksáv mozgása három radarképen, nagyítás Budapestre a + gombbal, majd az animáció megállítása az időbélyegre koppintva.](docs/images/rain-radar-hu.gif)

A RainViewer archivált képei 2026. november 26-án, budapesti idő szerint 12:40 és 13:00 között készültek. A **+** gombbal közelíthetsz. A bal alsó időbélyegre koppintva megállíthatod a lejátszást, újabb koppintással pedig folytathatod.

### Hőtérkép Budapest körül

![A térképen Celsius-fokban látszanak a hőmérsékletek. Három koppintással 160 NM-ről 20 NM-re közelítünk Budapestre, ahol 2,9 °C látható.](docs/images/heat-map-hu.gif)

A címkék a felszín felett két méterrel számított léghőmérsékletet mutatják. A **+** gombbal közelíthetsz. Az alsó színskála mindig az adott nézethez igazodik. A felvétel a 2026. november 27-i, budapesti idő szerint 12:30-as adatokat mutatja. [Adatforrás: Open-Meteo](https://open-meteo.com/)

### Műszerek

![A menüből megnyílik a Műszerek lap. A GIF kiemeli a RAM és a CPU kijelzőit, majd a microSD szabad és teljes kapacitását.](docs/images/instruments-hu.gif)

A **RAM** és a **PSRAM** mezőben a foglalt és a teljes heapmemória mérete, valamint a foglaltság százaléka látható. A **CPU 0** és a **CPU 1** külön mutatja a két mag becsült terhelését az utolsó egy másodperc mintái alapján.

A **☰ → Műszerek** lapon külön kártyán látható a hőmérséklet (°C), a relatív páratartalom (%) és a légnyomás (hPa). A **Hardveradatok** alatt találod az eszközök és a hálózat állapotát. [A v1.4.0 főmenüje](docs/images/v140-menu-hu.png)

A **Beállítások** három lapja a **Kapcsolat és rendszer**, a **Térkép és GPS**, valamint a **Kijelző és eszköz**. A sor végén a nyíl almenüt, a ceruza szerkesztést, a kettős nyíl értékváltást, a kapcsoló pedig ki- és bekapcsolást jelöl.

### Képernyőpihenő

![A Légtér képernyőről megnyílik a hamburger menü. A Képernyőpihenő kiválasztása után sötét háttéren látszik az óra, majd egy koppintás visszahozza a térképet.](docs/images/screen-saver-hu.gif)

Koppints a bal felső **☰** gombra, majd válaszd a **Képernyőpihenő** menüpontot. Amikor az óra látszik, koppints egyszer bárhová a kijelzőn, és visszatérsz az előző képernyőre.

![A T-Deck Wi-Fi-jelzője bontáskor és újracsatlakozáskor](docs/images/wifi-reconnect-hu.gif)

A felső sávban három ív jelzi az aktív kapcsolatot, áthúzott ikon a szakadást, környíl az újracsatlakozást. A képkockákat egy T-Decken rögzítettük.

## Mire van szükséged?

- Az eredeti **LilyGO T-Deck** modellre, ESP32-S3 processzorral, 16 MB flashmemóriával és 8 MB OPI PSRAM-mal, valamint egy USB-adatkábelre. A firmware a fő ESP32-S3 processzorra kerül. A billentyűzetnek külön vezérlője van.
- **2,4 GHz-es Wi-Fi-re** az élő forgalomhoz és az időjáráshoz. Az 1.5.0-s firmware első induláskor felajánlja a külön letölthető offline világtérképet. A **Később** gombbal előbb beállíthatod a Wi-Fi-t. A **Műszerek → SD újra [R]** újranyitja a letöltést, ha hiányzik a térkép.
- **A GPS, az utólagos I²C-érzékelők és a microSD választhatók.** GPS-pozíció nélkül a kézzel beállított hely a térkép közepe, alapból Budapest belvárosa. A hiányzó mérések helyén `--` jelenik meg. Az offline térképhez írható microSD szükséges, kivéve, ha a készüléken külön előkészített belső térképtár van.

## Telepítés és frissítés

A [telepítőmappában](install/) két kész firmware található. Az alábbi parancsok Windows PowerShellhez készültek. Telepítsd a Python 3-at a Windowsos Python Launcherrel, majd nyisd meg a terminált a klónozott vagy a [ZIP-ből kicsomagolt projekt](https://github.com/janix12/T-Deck-FlightDeck/archive/refs/heads/main.zip) mappájában.

1. Zárd be a soros monitort. Csatlakoztasd a T-Decket, és kapcsold be. Az Eszközkezelő **Portok (COM és LPT)** részén figyeld meg, melyik port jelenik meg csatlakoztatáskor. A példában lévő `COM5` helyére ezt írd. Telepítsd a feltöltőprogramot, és állítsd be a portot.

   ```powershell
   py -m pip install "esptool==4.8.1"
   $port = 'COM5'
   ```

2. **Másik firmware cseréje előtt** készíts mentést, ha később vissza szeretnéd állítani. Ebben Wi-Fi-jelszó és személyes beállítás is lehet, ezért tartsd magadnál. Betöltő módhoz tartsd lenyomva a trackball közepét, nyomd meg és engedd el az **RST** gombot, majd engedd el a trackballt. Ha a Windows új portszámot ad, módosítsd a változót.

   ```powershell
   New-Item -ItemType Directory -Force backup | Out-Null
   py -m esptool --chip esp32s3 --port $port --before no_reset read_flash 0x0 0x1000000 backup/original-16MB.bin
   ```

3. Lépj ismét betöltő módba. **Első telepítéskor** a teljes képet írd a **0x0** címre. Ez felülírja a betöltőt, a partíciós táblát, az alkalmazást és az érintett területen lévő beállításokat.

   ```powershell
   py -m esptool --chip esp32s3 --port $port --before no_reset write_flash 0x0 install/FlightDeck-full.bin
   ```

   Ha **már FlightDeck fut azonos partíciókiosztással**, frissítéshez az alábbi parancsot használd. Így megmaradnak a mentett beállításaid.

   ```powershell
   py -m esptool --chip esp32s3 --port $port --before no_reset --after no_reset write_flash 0x10000 install/FlightDeck.ino.bin
   if ($LASTEXITCODE -eq 0) { py -m esptool --chip esp32s3 --port $port --before no_reset erase_region 0xe000 0x2000 }
   ```

4. Várd meg az írás ellenőrzését, majd nyomd meg az **RST** gombot. Saját teljes mentés visszaállításához lépj betöltő módba, és az első telepítés parancsában a `backup/original-16MB.bin` fájlt írd a `0x0` címre. A letöltött fájlok ellenőrzéséhez futtasd a `Get-FileHash install/*.bin -Algorithm SHA256` parancsot, és hasonlítsd össze a [közzétett ellenőrzőösszegekkel](install/SHA256SUMS.txt).

## Első indítás

1. Nyisd meg a **Beállítások → Wi-Fi név → Hálózatok keresése** lehetőséget. Védett hálózat kiválasztása után írd be a jelszót, majd nyomj **Mentés**-t. Nyílt hálózatnál elég a nevet menteni. Mesh hálózathoz a 2,4 GHz-en is sugárzott közös nevet válaszd. A **Műszerek → Hardveradatok** lapon látszik, ha a kapcsolat hitelesítésnél vagy az IP-cím kérésénél tart.
2. GPS nélkül add meg a szélességet és a hosszúságot tizedes fokban, tizedesponttal. Időzónának a `Budapest`, az `UTC` vagy egy POSIX-időzóna adható meg. Wi-Fi-n az óra NTP-vel szinkronizál, induláskor érvényes GPS-idő is beállíthatja. Kézzel az UTC-időt `YYYY-MM-DD HH:MM` formában írd be.
3. Az **1** gombbal nyisd meg a **Légtér** nézetet. Az alapbeállítás 40 tengeri mérföldes körzet és kétperces frissítés. Egy gépre koppintva láthatod a hívójelét, magasságát, sebességét és helyét.

## Az offline térkép frissítése

Az első indításkor válaszd a **Térkép letöltése** ablakban a **Letöltés** gombot. A T-Deck Wi-Fi-n tölti le az 5,84 MB-os első kiadást az írható microSD-kártyára. A fájl neve `/WORLD.FDM`. A letöltött térképet offline is használhatod.

Induláskor, majd 24 óránként a készülék lekéri a GitHubról a kis verziófájlt. Naprakész térképnél ezzel végez. Új kiadásnál automatikusan letölti a csomagot, és aktiválás előtt SHA-256-tal ellenőrzi a tárolt fájlt. Sérülés vagy megszakadt letöltés után újrapróbálja, amikor van Wi-Fi és szinkronizált az óra.

## Mentett Wi-Fi és firmware-frissítés

A **Beállítások → Mentett hálózatok** alatt legfeljebb nyolc hálózatot tárolhatsz. Az **Új hálózat** megkeresi a környékbeli hálózatokat. Válassz ki egy mentett nevet a csatlakozáshoz vagy a törléshez. A törlést meg kell erősíteni. Frissítéskor a készülék átveszi a korábban mentett hálózatot. Ha megszakad a kapcsolat, megkeresi az elérhető mentett hálózatokat és a mentett jelszóval próbálkozik.

A v1.1.0 az első Wi-Fi-n frissíthető kiadás. Ezt egyszer USB-n kell telepíteni. A **Beállítások → Firmware-frissítés** megmutatja a telepített verziót. A készülék Wi-Fi-kapcsolat és óraszinkron után induláskor, majd naponta ellenőriz. Új kiadásnál az óra mellett felváltva megjelenő **FW+** jelzésre koppintva megnyithatod a frissítést. A **Telepítés** gomb indítja a letöltést. A készülék HTTPS-en tölti le az alkalmazást a projekt GitHub Pages oldaláról, a másik alkalmazáspartícióra ír, és a SHA-256 ellenőrzés után újraindul. A hálózatok és a beállítások megmaradnak.

A v1.2.0-tól a **Beállítások → Firmware-frissítés → Korábbi verziók** menüben választhatsz a telepített kiadást megelőző öt verzióból. A v1.1.0, az első Wi-Fi-s kiadás mindig szerepel. Válassz egy verziót, ellenőrizd a két verziószámot, majd koppints a **Visszaállítás** gombra. A készülék Wi-Fi-n tölti le és ellenőrzi az eredeti kiadást. A v1.1.x-re visszatérve az **Ellenőrzés → Telepítés** gombokkal újra felteheted a legfrissebb verziót. [A verziólista a T-Decken](docs/images/firmware-versions.png)

## Miért kétpercenként frissül a légtér?

Az adsb.lol API-kulcs nélkül **500 lekérést enged naponta, nyilvános IP-címenként**. Az azonos internetkapcsolatot használó készülékek ezen a kereten osztoznak. A szolgáltató éjfélkor (00:00 UTC) indít új napot. Ez Magyarországon télen 01:00, nyáron 02:00. [Az adsb.lol hivatalos korlátai](https://www.adsb.lol/docs/open-data/api)

A FlightDeck a v1.0.2-es verziótól alapból **120 másodpercenként** kér repülőadatot. Ez 16 órányi folyamatos légtérnézethez legfeljebb **480 kérés**. A **Beállítások → Frissítés (mp)** alatt 120–300 másodperc választható. A korábbi rövidebb beállítás frissítéskor 120 másodpercre vált. Másik oldalon és képernyőpihenő közben szünetel az új repülőlekérések indítása.

A **Műszerek → Hardveradatok** lap az eszköz saját napi számlálóját mutatja. A készülék újraindítás után is megőrzi a felhasznált keretet. Ha a szerver **HTTP 429** választ ad, a radaron az **API-korlát** felirat és a következő próbálkozás helyi időpontja látszik. A készülék betartja a szolgáltató várakozási idejét (`Retry-After`). Ha ilyen fejléc nem érkezik, egyre ritkábban próbálkozik. Az időjárási rétegek külön szolgáltatásokból érkeznek.

Wi-Fi-szakadáskor a készülék újracsatlakozik. Tartósan gyenge jelnél azonos hálózatnévvel és biztonsági móddal működő, legalább 10 dB-lel erősebb hozzáférési pontot keres. A barangolás is megtartja a lekérdezések ütemezését.

## Kezelés

| Művelet | Mit nyomj meg? |
|---|---|
| Menü és oldalak | A **☰** vagy az **M** nyitja a menüt. Az **1–5** gombok sorrendben a Légteret, a Műszereket, a Beállításokat, az Esőradart és a Hőtérképet nyitják meg. |
| Nyitva hagyott menü | A menü az utolsó érintés, billentyű vagy trackball-mozdulat után 60 másodperccel bezárul. Amíg nyitva van, az új repülő-, időjárás- és automatikus verziólekérdezések szünetelnek. |
| Nagyítás és gépadatok | Koppints a **+ / −** gombokra, vagy nyomj **I / O**-t. A bal alsó listaikon megnyitja a géplistát. Újabb koppintás, a **×** vagy az **F** bezárja. Az oldalsó kártyát függőlegesen húzva görgetheted. |
| Térkép mozgatása | A Légtér, az Esőradar és a Hőtérkép térképét ujjal húzhatod. A trackballal négy irányban nézhetsz körül. A felső lapcímre koppintva visszatérsz a saját helyedhez, aktív követésnél a géphez. |
| Listák és középső gomb | A teljes Légtér térképen nyomd le a trackballt a géplistához. Görgess fel vagy le a kijelöléshez, majd nyomd le a golyót a gép adatlapjához. A menüben és a beállítások listáiban is így választhatsz. |
| Követés | Válassz gépet a listából, görgess az adatlap aljára, és kapcsold be a **Követés** kapcsolót. A térkép ránagyít a gépre, és minden új pozíciónál középre helyezi. Ha a jel eltűnik, az utolsó ismert hely marad látható. A készülék a szokásos frissítési ütemben keresi újra. |
| Csak ez a gép | Bekapcsoláskor elindul a kiválasztott gép követése, és a többi gép ikonja eltűnik a térképről. Ha kikapcsolod a szűrőt, a többi gép újra látszik, a követés pedig folytatódik. A követést külön is kikapcsolhatod. Másik gép kiválasztásakor mindkét kapcsoló kikapcsol. |
| Wi-Fi-jel a felső sávban | A három folyamatos ív aktív Wi-Fi-kapcsolatot és kiosztott IP-címet jelez. Kapcsolódáskor az ívek egymás után jelennek meg. Újracsatlakozáskor környíl, megszakadt vagy hiányzó kapcsolatnál áthúzott Wi-Fi-jel látszik. |
| Repülőikonok | A kitöltött ikon legfeljebb 90 másodperces pozíciót jelöl. Régebbi pozíciónál a gép körvonalként látszik. A **?** ismeretlen irányt, az állapot melletti **\*** a mozgásból becsült állapotot jelzi. |
| Időjárás | Az esőradar legfeljebb három RainViewer-képkockát játszik le. Az alsó időpontra koppintva megállíthatod vagy folytathatod. A hőtérkép kilenc Open-Meteo mintát vagy közeli repülőterek METAR-észleléseit használja. Mindkét nézet körülbelül tízpercenként frissül. |
| Hardver ellenőrzése | A **2** nyitja a **Műszerek** lapot. A **microSD** sor a szabad és teljes kötetterületet mutatja. A **B** hangszórópróbát indít, az **R** újrapróbálja az SD csatolását. Az akkuikon akkuról becsült százalékot, felismert külső tápon **USB** feliratot mutat. A tényleges töltést a kék LED jelzi. |
| Megjelenés | A Beállítások utolsó lapján választhatsz nyelvet, színtémát és képernyőpihenőt. Az alapérték 30 perc. A **0** kikapcsolja a pihenőt. Érintés vagy billentyű ébreszti fel a képernyőt. |
| Számok és jelek | Szerkesztéskor a képernyő **SYM** gombjával válthatsz. Billentyűzetről az újabb, mátrixmódú vezérlőn nyomd meg a **SYM** gombot. Régebbi vezérlőn tartsd lenyomva a **SYM**-et és nyomj **Q**-t. Ismételd meg a kikapcsoláshoz. **SYM BE** mellett a **W E R** beírja az **1 2 3** számokat, a **szóköz** a **0**-t, a **Q** a **#** jelet. A kijelző külön **0** gombja mindkét módban működik. |
| Billentyűvilágítás | Régebbi billentyűzeten tartsd lenyomva az **ALT** gombot, és nyomd meg a **B**-t a fény be- vagy kikapcsolásához. Ezeken a modelleken a fény pihenőben is bekapcsolva maradhat. Az újabb vezérlők fényereje a **Beállítások → Billentyűfény (%)** mezőben állítható. |

## Fordítás forrásból

Telepítsd a Git for Windows programot, majd nyisd meg a PowerShellt a projekt mappájában. Az első parancs letölti az Arduino CLI-t, az ESP32 Arduino 2.0.17-es csomagját, a rögzített LilyGO-meghajtókat és az érzékelőkönyvtárakat. Később elég a második parancs.

```powershell
.\build.ps1 -Setup
.\build.ps1
```

A saját fordítású képek a `firmware/` mappába kerülnek. Feltöltésükhöz a telepítési parancsokban ennek a mappának az útvonalát használd. A térképet külön adatcsomagként kezeli. A készülék indításkor és utána 24 óránként lekéri a GitHubos térkép verzióját. Új kiadás vagy hibás helyi fájl esetén letölti a csomagot, és SHA-256-tal ellenőrzi. Megszakadás után automatikusan újrapróbálja. [Térképforrások és újragenerálás](maps/README.md)

Új kiadás készítésekor töltsd le a címkéket (`git fetch --tags`), állítsd be az új verziót a `FlightDeck/firmware_version.h` fájlban, majd fordítás előtt futtasd a `py tools/prepare_rollback.py` parancsot. A generált katalógust és az `install/archive/` fájlokat a firmware-rel együtt tedd közzé. A régebbi készülékek ugyanezekről a címekről töltenek le. Az új binárisok, a `manifest.json` és a `SHA256SUMS.txt` telepítőmappába helyezése után futtasd a `py tools/prepare_rollback.py --usb` parancsot. Az így kapott `install/usb-releases.json` fájlt és az archív teljes képeket is tedd közzé.

## Hibaelhárítás és adatkezelés

- **Nincsenek gépek vagy időjárás:** ellenőrizd a Wi-Fi-t és az órát a Műszerek → Hardveradatok lapon. A HTTPS-kérésekhez érvényes dátum kell. GPS-pozíció híján ellenőrizd a kézi térképközéppontot.
- **Nincs GPS-adat:** a 3,3 V-os UART-modul TX kimenetét kösd a T-Deck beállított RX bemenetére, közös földdel. Alapból GPIO44, választható a GPIO43 is. A **0** baud érték automatikus keresést indít.
- **SD-hiba:** helyezd vissza a kártyát, majd nyomj **R**-t a Műszerek lapon. A készülék betöltéskor ellenőrzi az alaptérképet. A percenkénti környezeti naplózáshoz írható kártya és bekapcsolt **SD-naplózás** szükséges.
- **USB-diagnosztika:** a soros monitor sebessége 115200 baud. A `?` állapotjelentést kér, az `X` hardvertesztet indít, az `Y` kiírja a számlálókat, az `E` kilép a tesztből.
- A Wi-Fi-beállítások a készüléken maradnak. Az online repülő- és időjárási lekérdezések a megadott térképközéppontot vagy a környező területet elküldik az adatszolgáltatóknak. A flashmentést, a konzolnaplókat és az SD-naplókat kezeld magánadatként. [adsb.lol](https://api.adsb.lol/docs) · [RainViewer](https://www.rainviewer.com/api/weather-maps-api.html) · [Open-Meteo](https://open-meteo.com/en/docs) · [Aviation Weather Center](https://aviationweather.gov/data/api/)

[Hardverdokumentáció](docs/hardware.md) · [Adatforrások, ikon és licencek](maps/README.md)
