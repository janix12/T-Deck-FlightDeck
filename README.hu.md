# FlightDeck a LilyGO T-Deckhez

**[USB-telepítő megnyitása →](https://janix12.github.io/T-Deck-FlightDeck/?lang=hu)** · [Kész firmware és kézi telepítés](https://github.com/janix12/T-Deck-FlightDeck/releases/latest)

Nyisd meg a telepítőt számítógépen Chrome-ban vagy Edge-ben, és csatlakoztasd az eredeti T-Decket USB-adatkábellel. Új eszköznél vagy másik rendszerről váltva válaszd az **Első telepítés** lehetőséget. Ha már FlightDeck fut rajta, a **FlightDeck frissítése** megtartja a beállításaidat. Kattints a **Csatlakozás és telepítés** gombra, majd válaszd ki az eszköz USB-portját.

**[English guide](README.md)** · [Hardver és kapcsolási rajzok](docs/hardware.md) · [Térképadatok és források](maps/README.md)

Nézd meg, melyik település fölött jár egy repülő. A FlightDeck a LilyGO T-Deck 320 × 240 pixeles kijelzőjén repülőtérképet, gépadatlapot, mozgó esőradart és hőmérsékleti térképet jelenít meg. A repülők ADS-B helyzetjelentései Wi-Fi-n érkeznek. Opcionális GPS-szel a térkép utazás közben követi a helyzetedet.

| Légtér Budapest körül | Közeli gépek távolság szerint |
|:---:|:---:|
| ![Repülőikonok Budapest környékének térképén](https://janix12.github.io/T-Deck-FlightDeck/docs/images/airspace.png?v=1.0.1) | ![Görgethető géplista a térkép mellett](https://janix12.github.io/T-Deck-FlightDeck/docs/images/flights.png?v=1.0.1) |
| **Közép-európai esőradar** | **Felszínközeli léghőmérséklet** |
| ![RainViewer csapadékréteg Közép-Európa felett](https://janix12.github.io/T-Deck-FlightDeck/docs/images/rain.png?v=1.0.1) | ![Budapest környéki hőmérsékleti térkép](https://janix12.github.io/T-Deck-FlightDeck/docs/images/temperature.png?v=1.0.1) |
| **Főmenü** | **Időzóna beállítása** |
| ![A főmenü a légtértérkép fölött](https://janix12.github.io/T-Deck-FlightDeck/docs/images/menu.png?v=1.0.1) | ![Időzóna szerkesztése a Mentés és Mégse gombokkal](https://janix12.github.io/T-Deck-FlightDeck/docs/images/time-zone.png?v=1.0.1) |

A képek a FlightDeck v1.0.1-et futtató T-Deck kijelzőjéről származnak. A térképek középpontja Budapest. A felület nyelve a **Beállítások → Nyelv** sorban váltható.

## Mire van szükséged?

- Az eredeti **LilyGO T-Deck** modellre, ESP32-S3 processzorral, 16 MB flashmemóriával és 8 MB OPI PSRAM-mal, valamint egy USB-adatkábelre. A firmware a fő ESP32-S3 processzorra kerül. A billentyűzetnek külön vezérlője van.
- **2,4 GHz-es Wi-Fi-re** az élő forgalomhoz és az időjáráshoz. A beépített térképen Magyarország részletesen, az időjárási nézet nagyobb léptékein Közép-Európa áttekintő térképpel látható.
- **A GPS, az utólagos I²C-érzékelők és a microSD választhatók.** GPS-pozíció nélkül a kézzel beállított hely a térkép közepe, alapból Budapest belvárosa. A hiányzó mérések helyén `--` jelenik meg. Használható kártya nélkül a térkép a flashmemóriából töltődik be.

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
   py -m esptool --chip esp32s3 --port $port --before no_reset write_flash 0x10000 install/FlightDeck.ino.bin
   ```

4. Várd meg az írás ellenőrzését, majd nyomd meg az **RST** gombot. Saját teljes mentés visszaállításához lépj betöltő módba, és az első telepítés parancsában a `backup/original-16MB.bin` fájlt írd a `0x0` címre. A letöltött fájlok ellenőrzéséhez futtasd a `Get-FileHash install/*.bin -Algorithm SHA256` parancsot, és hasonlítsd össze a [közzétett ellenőrzőösszegekkel](install/SHA256SUMS.txt).

## Első indítás

1. Nyisd meg a **Beállítások** oldalt a **3** gombbal. Add meg a Wi-Fi nevét és jelszavát. A hálózatnév mezőből keresést is indíthatsz. Nyílt hálózatnál üres jelszót ments. A bal alsó **Mentés** elmenti, a jobb alsó **Mégse** elveti a beírt értéket.
2. GPS nélkül add meg a szélességet és a hosszúságot tizedes fokban, tizedesponttal. Időzónának a `Budapest`, az `UTC` vagy egy POSIX-időzóna adható meg. Wi-Fi-n az óra NTP-vel szinkronizál, induláskor érvényes GPS-idő is beállíthatja. Kézzel az UTC-időt `YYYY-MM-DD HH:MM` formában írd be.
3. Az **1** gombbal nyisd meg a **Légtér** nézetet. Az alapbeállítás 40 tengeri mérföldes körzet és kétperces frissítés. Egy gépre koppintva láthatod a hívójelét, magasságát, sebességét és helyét.

## Miért kétpercenként frissül a légtér?

Az adsb.lol API-kulcs nélkül **500 lekérést enged naponta, nyilvános IP-címenként**. Az azonos internetkapcsolatot használó készülékek ezen a kereten osztoznak. A szolgáltató éjfélkor (00:00 UTC) indít új napot. Ez Magyarországon télen 01:00, nyáron 02:00. [Az adsb.lol hivatalos korlátai](https://www.adsb.lol/docs/open-data/api)

A FlightDeck a v1.0.2-es verziótól alapból **120 másodpercenként** kér repülőadatot. Ez 16 órányi folyamatos légtérnézethez legfeljebb **480 kérés**. A **Beállítások → Frissítés (mp)** alatt 120–300 másodperc választható. A korábbi rövidebb beállítás frissítéskor 120 másodpercre vált. Másik oldalon és képernyőpihenő közben szünetel az új repülőlekérések indítása.

A **Műszerek** lap az eszköz saját napi számlálóját mutatja. A készülék újraindítás után is megőrzi a felhasznált keretet. Ha a szerver **HTTP 429** választ ad, a radaron az **API-korlát** felirat és a következő próbálkozás helyi időpontja látszik. A készülék betartja a szolgáltató várakozási idejét (`Retry-After`). Ha ilyen fejléc nem érkezik, egyre ritkábban próbálkozik. Az időjárási rétegek külön szolgáltatásokból érkeznek.

Wi-Fi-szakadáskor a készülék újracsatlakozik. Tartósan gyenge jelnél azonos hálózatnévvel és biztonsági móddal működő, legalább 10 dB-lel erősebb hozzáférési pontot keres. A barangolás is megtartja a lekérdezések ütemezését.

## Kezelés

| Művelet | Mit nyomj meg? |
|---|---|
| Menü és oldalak | A **☰** vagy az **M** nyitja a menüt. Az **1–5** gombok sorrendben a Légteret, a Műszereket, a Beállításokat, az Esőradart és a Hőtérképet nyitják meg. |
| Nagyítás és gépadatok | Koppints a **+ / −** gombokra, vagy nyomj **I / O**-t. A bal alsó listaikon megnyitja a géplistát. Újabb koppintás, a **×** vagy az **F** bezárja. Az oldalsó kártyát függőlegesen húzva görgetheted. |
| Repülőikonok | A kitöltött ikon legfeljebb 90 másodperces pozíciót jelöl. Régebbi pozíciónál a gép körvonalként látszik. A **?** ismeretlen irányt, az állapot melletti **\*** a mozgásból becsült állapotot jelzi. |
| Időjárás | Az esőradar legfeljebb három RainViewer-képkockát játszik le. Az alsó időpontra koppintva megállíthatod vagy folytathatod. A hőtérkép kilenc Open-Meteo mintát vagy közeli repülőterek METAR-észleléseit használja. Mindkét nézet körülbelül tízpercenként frissül. |
| Hardver ellenőrzése | A **2** nyitja a **Műszerek** lapot. A **B** hangszórópróbát indít, az **R** újrapróbálja az SD csatolását. Az akkuikon akkuról becsült százalékot, felismert külső tápon **USB** feliratot mutat. A tényleges töltést a kék LED jelzi. |
| Megjelenés | A Beállítások utolsó lapján választhatsz nyelvet, színtémát és képernyőpihenőt. Az alapérték 30 perc. A **0** kikapcsolja a pihenőt. Érintés vagy billentyű ébreszti fel a képernyőt. |

## Fordítás forrásból

Telepítsd a Git for Windows programot, majd nyisd meg a PowerShellt a projekt mappájában. Az első parancs letölti az Arduino CLI-t, az ESP32 Arduino 2.0.17-es csomagját, a rögzített LilyGO-meghajtókat és az érzékelőkönyvtárakat. Később elég a második parancs.

```powershell
.\build.ps1 -Setup
.\build.ps1
```

A saját fordítású képek a `firmware/` mappába kerülnek. Feltöltésükhöz a telepítési parancsokban ennek a mappának az útvonalát használd. A fordítás beépíti a `maps/` mappa kész térképeit. Újragenerálásuk választható. [Térképforrások és újragenerálás](maps/README.md)

## Hibaelhárítás és adatkezelés

- **Nincsenek gépek vagy időjárás:** ellenőrizd a Wi-Fi-t és az órát a Műszerek lapon. A HTTPS-kérésekhez érvényes dátum kell. GPS-pozíció híján ellenőrizd a kézi térképközéppontot.
- **Nincs GPS-adat:** a 3,3 V-os UART-modul TX kimenetét kösd a T-Deck beállított RX bemenetére, közös földdel. Alapból GPIO44, választható a GPIO43 is. A **0** baud érték automatikus keresést indít.
- **SD-hiba:** helyezd vissza a kártyát, majd nyomj **R**-t a Műszerek lapon. A készülék betöltéskor ellenőrzi az alaptérképet. A percenkénti környezeti naplózáshoz írható kártya és **SD napló = 1** szükséges.
- **USB-diagnosztika:** a soros monitor sebessége 115200 baud. A `?` állapotjelentést kér, az `X` hardvertesztet indít, az `Y` kiírja a számlálókat, az `E` kilép a tesztből.
- A Wi-Fi-beállítások a készüléken maradnak. Az online repülő- és időjárási lekérdezések a megadott térképközéppontot vagy a környező területet elküldik az adatszolgáltatóknak. A flashmentést, a konzolnaplókat és az SD-naplókat kezeld magánadatként. [adsb.lol](https://api.adsb.lol/docs) · [RainViewer](https://www.rainviewer.com/api/weather-maps-api.html) · [Open-Meteo](https://open-meteo.com/en/docs) · [Aviation Weather Center](https://aviationweather.gov/data/api/)

[Hardverdokumentáció](docs/hardware.md) · [Adatforrások, ikon és licencek](maps/README.md)
