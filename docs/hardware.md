# T-Deck hardware reference · T-Deck műszaki dokumentáció

[English guide](../README.md) · [Magyar útmutató](../README.hu.md)

**EN** · Use the original T-Deck pinout below. Confirm the exact model on your board before connecting peripherals. LilyGO publishes separate documentation for the Plus and Pro models. The PDFs in the table open at the manufacturer's repository.

**HU** · Az alábbi kiosztás az eredeti T-Deckhez tartozik. Bekötés előtt ellenőrizd az alaplap pontos típusát. A Plus és a Pro modellekhez külön gyári dokumentáció készült. A táblázat PDF-jei a gyártó tárhelyén nyílnak meg.

| English | Magyar | Source / Forrás |
|---|---|---|
| Specifications, pinout and dimensions | Jellemzők, lábkiosztás és méretek | [LilyGO T-Deck Wiki](https://wiki.lilygo.cc/products/t-deck-series/t-deck/) |
| Power, USB boot and first steps | Tápellátás, USB-betöltés, első lépések | [LilyGO Quick Start](https://wiki.lilygo.cc/products/t-deck-series/t-deck/quick-start) |
| Board schematic, battery divider and charging LED | Alaplapi kapcsolási rajz, akkufeszültség-osztó és töltésjelző LED | [T-Deck PDF](https://github.com/Xinyuan-LilyGO/T-Deck/blob/9aac147a313975ad063594b3fe11e28a96d94936/schematic/schematic.pdf) |
| GPS expansion schematics, including the L76K board | GPS-bővítők kapcsolási rajzai, köztük az L76K panel | [LilyGO GPS / L76K](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/schematic) |
| LoRa antenna data for 433 and 868–915 MHz | LoRa-antennák adatlapjai 433 és 868–915 MHz-re | [LilyGO antenna PDFs](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/datasheet) |
| Enclosure drawings and model files | Házrajzok és modellek | [LilyGO shell](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/shell) |
| ESP32-S3 chip data sheet | ESP32-S3 processzoradatlap | [Espressif PDF](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf) |
| Vendor pin definitions and hardware examples | Gyári lábkiosztás és hardverpéldák | [GPIO](https://github.com/Xinyuan-LilyGO/T-Deck/blob/9aac147a313975ad063594b3fe11e28a96d94936/examples/UnitTest/utilities.h) · [Examples](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/examples) · [Drivers](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/lib) |
| GT911 touch, keyboard and microphone examples | GT911 érintés, billentyűzet és mikrofon példái | [Touch](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/examples/Touchpad) · [Keyboard](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/examples/Keyboard_T_Deck_Master) · [Mic](https://github.com/Xinyuan-LilyGO/T-Deck/tree/9aac147a313975ad063594b3fe11e28a96d94936/examples/Microphone) |
| Product identification and Plus model differences | Termékazonosítás és a Plus modell eltérései | [Product / Termék](https://lilygo.cc/en-us/products/t-deck-meshtastic) · [T-Deck Plus](https://wiki.lilygo.cc/products/t-deck-series/t-deck-plus/) |
| ESP32-S3 flash backup and write commands | ESP32-S3 mentési és feltöltési parancsok | [Espressif esptool 4.x](https://docs.espressif.com/projects/esptool/en/release-v4/esp32s3/esptool/basic-commands.html) |

## Connections used by FlightDeck · A firmware bekötései

| Function / Funkció | GPIO | Notes / Megjegyzés |
|---|---|---|
| Peripheral power / Perifériatáp | 10 | Enabled at startup / Induláskor bekapcsol |
| I²C | SDA 18, SCL 8 | Shared by keyboard, touch and external sensors / A billentyűzet, az érintés és az érzékelők közös busza |
| SPI | SCK 40, MOSI 41, MISO 38 | LCD CS 12, microSD CS 39, LoRa CS 9 |
| Display / Kijelző | DC 11, BL 42 | ST7789, 320 × 240 |
| Trackball | 1, 2, 3, 15 · press / nyomás 0 | The centre also acts as BOOT / A középső gomb a BOOT is |
| GPS UART | RX 44 or / vagy 43 | GPS TX → T-Deck RX, common GND / közös GND |
| Battery ADC / Akkumérés | 4 | Voltage estimate / Feszültségalapú becslés |
| LoRa SX1262 | IRQ 45, BUSY 13, RST 17 | FlightDeck initialises receive mode at 868 MHz / Vétel 868 MHz-en |
| Speaker / Hangszóró | WS 5, BCLK 7, DATA 6 | I²S |
| Microphone / Mikrofon | MCLK 48, LRCK 21, SCK 47, DIN 14 | ES7210 |

**EN** · GPS and external sensors are independent additions. Wire a compatible 3.3 V I²C sensor to SDA, SCL and ground, and provide power according to its module data sheet. FlightDeck probes BMP280/BME280, BMP3xx, DPS310, SHT31 and AHT2x-compatible devices. A BMP280 and an AHT2x-compatible sensor have been used together. With no external sensor the other pages still work. With no GPS the firmware uses the manual location and reads time from NTP over Wi-Fi. Consult the schematic before wiring a Plus board because its GPS uses the Grove connection.

**HU** · A GPS és a külső érzékelők egymástól független bővítések. A 3,3 V-os I²C-vel kompatibilis érzékelő SDA, SCL és GND pontját kösd a megfelelő lábakhoz, a tápját pedig a modul adatlapja szerint csatlakoztasd. A firmware BMP280/BME280, BMP3xx, DPS310, SHT31 és AHT2x-kompatibilis érzékelőket keres. BMP280 és AHT2x-kompatibilis érzékelő együtt is működött. Külső érzékelők nélkül a többi oldal használható. GPS nélkül a kézi helybeállítást veszi alapul, az időt Wi-Fi-n, NTP-ről kapja. A Plus változatnál bekötés előtt nézd meg a rajzot, mert a GPS a Grove csatlakozást használja.

[FlightDeck pin definitions / lábkiosztás](../FlightDeck/board.h) · [Hardware detection / hardverfelismerés](../FlightDeck/hardware.h)
