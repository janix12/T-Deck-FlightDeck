# FlightDeck for LilyGO T-Deck

**[Open the USB installer →](https://janix12.github.io/T-Deck-FlightDeck/?lang=en)** · [Firmware downloads and manual installation](https://github.com/janix12/T-Deck-FlightDeck/releases/latest)

Open the installer in desktop Chrome or Edge and connect your original T-Deck with a USB data cable. Choose **First installation** for a new device or to replace another system. If it already runs FlightDeck, choose **Update FlightDeck** to keep your settings. Click **Connect and install** and select the device’s USB port.

**[Magyar útmutató](README.hu.md)** · [Hardware and schematics](docs/hardware.md) · [Map data and credits](maps/README.md)

See which town an aircraft is flying over. FlightDeck turns the 320 × 240 display of a LilyGO T-Deck into a flight map with aircraft details, animated rain radar and a temperature map. Aircraft positions come from ADS-B reports over Wi-Fi. An optional GPS sets the map centre as you travel.

| 32 aircraft around Budapest | Aircraft list and 10 NM scale |
|:---:|:---:|
| ![32 aircraft on the Budapest radar with the 10 NM scale](https://janix12.github.io/T-Deck-FlightDeck/docs/images/airspace-traffic.png?v=1.1.2) | ![The selected flight and scrollable aircraft list](https://janix12.github.io/T-Deck-FlightDeck/docs/images/flights.png?v=1.1.2) |
| **Rain radar over Central Europe** | **Surface air temperature** |
| ![RainViewer precipitation overlay across Central Europe](https://janix12.github.io/T-Deck-FlightDeck/docs/images/rain.png?v=1.0.4) | ![Temperature map around Budapest](https://janix12.github.io/T-Deck-FlightDeck/docs/images/temperature.png?v=1.0.4) |
| **Main menu** | **Time zone settings** |
| ![Main menu open over the flight map](https://janix12.github.io/T-Deck-FlightDeck/docs/images/menu.png?v=1.0.4) | ![Time zone editor with Save and Cancel buttons](https://janix12.github.io/T-Deck-FlightDeck/docs/images/time-zone.png?v=1.0.4) |

Both radar screenshots were captured on a T-Deck running v1.1.2 with live traffic. Choose **Beállítások → Nyelv → English** for the English interface. [Earlier example of the API limit notice](docs/images/airspace.png)

## What you need

- The original **LilyGO T-Deck**, ESP32-S3, 16 MB flash, 8 MB OPI PSRAM, and a USB data cable. The firmware targets the main ESP32-S3. The keyboard has its own controller.
- A **2.4 GHz Wi-Fi network** for live flights and weather. The built-in maps cover Hungary in detail and Central Europe at wider weather zoom levels.
- **GPS, external I²C environmental sensors and microSD are optional.** Without a GPS fix the map uses the coordinates in Settings. The default is central Budapest. Missing sensor readings appear as `--`. Without a usable card the map loads from flash.

## Install or update

The [installation folder](install/) contains two ready-to-flash images. The commands below are for Windows PowerShell. Install Python 3 with the Windows launcher, then open a terminal in a clone or an extracted [project ZIP](https://github.com/janix12/T-Deck-FlightDeck/archive/refs/heads/main.zip).

1. Close any serial monitor. Connect the T-Deck and switch it on. In Windows Device Manager, look under **Ports (COM & LPT)** for the port that appears when you connect it. Replace the example `COM5` below with yours. Install the flashing tool and set the port variable.

   ```powershell
   py -m pip install "esptool==4.8.1"
   $port = 'COM5'
   ```

2. **Before replacing another firmware**, save your current flash if you want to restore it later. This backup can contain Wi-Fi credentials and device settings. Keep it private. To enter download mode, hold the centre of the trackball, press and release **RST**, then release the trackball. Check the port again if Windows assigns a different number.

   ```powershell
   New-Item -ItemType Directory -Force backup | Out-Null
   py -m esptool --chip esp32s3 --port $port --before no_reset read_flash 0x0 0x1000000 backup/original-16MB.bin
   ```

3. Enter download mode again. For a **first installation**, write the full image at **0x0**. This replaces the bootloader, partition table, application and settings in the covered area.

   ```powershell
   py -m esptool --chip esp32s3 --port $port --before no_reset write_flash 0x0 install/FlightDeck-full.bin
   ```

   For an **update to an existing FlightDeck with the same partition layout**, use this command instead. It preserves saved settings.

   ```powershell
   py -m esptool --chip esp32s3 --port $port --before no_reset --after no_reset write_flash 0x10000 install/FlightDeck.ino.bin
   if ($LASTEXITCODE -eq 0) { py -m esptool --chip esp32s3 --port $port --before no_reset erase_region 0xe000 0x2000 }
   ```

4. Wait for the hash verification, then press **RST** to start FlightDeck. To restore your own full backup later, enter download mode and use the first-install command with `backup/original-16MB.bin` at `0x0`. You can check the downloads with `Get-FileHash install/*.bin -Algorithm SHA256` against [the published checksums](install/SHA256SUMS.txt).

## First start

1. Open **Settings → Wi-Fi name → Scan networks**. For a protected network, enter the password when prompted and tap **Save**. For an open network, save the name. On a mesh network, select the shared name that also broadcasts on 2.4 GHz. **Instruments** shows authentication failures and whether the device is waiting for an IP address.
2. Set latitude and longitude in decimal degrees if you use the device without GPS. Choose `Budapest`, `UTC` or a POSIX time zone. Wi-Fi synchronises the clock using NTP. A valid GPS date and time can initialise it too. Manual UTC entry uses `YYYY-MM-DD HH:MM`.
3. Open **Airspace** (key **1**). Defaults are a 40 nautical mile radius and a two-minute refresh. Tap an aircraft to see its callsign, altitude, speed and location.

## Saved Wi-Fi networks and firmware updates

Open **Settings → Saved networks** to browse up to eight profiles. **Add network** scans for nearby networks. Select a saved name to connect or forget it. Forgetting a network requires confirmation. Upgrading imports the previously saved network. After a lost connection FlightDeck looks for available saved networks and uses their stored credentials.

Version 1.1.0 is the first release with Wi-Fi updates. Install it once over USB. **Settings → Firmware update** shows the installed version. Once Wi-Fi and the clock are ready, FlightDeck checks at startup and once a day. When an update is available, **FW+** alternates with the clock. Tap it to open the update screen and choose **Install**. The device downloads the application over HTTPS from the project’s GitHub Pages site, writes it to the other application slot, verifies its SHA-256 digest and restarts. Your networks and settings are preserved.

From v1.2.0, open **Settings → Firmware update → Previous versions** to see up to five releases preceding the installed version. The first Wi-Fi release, v1.1.0, is always included. Select a version, review the installed and selected version numbers, then tap **Restore**. FlightDeck downloads and verifies the original release over Wi-Fi. After restoring v1.1.x, use **Check → Install** to return to the latest release. [Version list on the T-Deck](docs/images/firmware-versions.png)

## Why does the flight map refresh every two minutes?

Without an API key, adsb.lol allows **500 requests per day per public IP address**. Devices sharing an internet connection share this allowance. The provider resets its daily counter at **00:00 UTC**. [Official adsb.lol API limits](https://www.adsb.lol/docs/open-data/api)

Starting with v1.0.2, FlightDeck requests flight data every **120 seconds** by default. This uses at most **480 requests in 16 hours** of continuous Airspace use. Select an interval from 120 to 300 seconds in **Settings → Refresh (sec)**. Upgrading raises older, shorter intervals to 120 seconds. The device pauses new flight requests on other pages and while the screen saver is active.

**Instruments** shows this device’s daily request count, which survives a restart. If the server returns **HTTP 429**, the radar shows **API limit** and the next attempt in local time. FlightDeck honours the provider’s `Retry-After` header and backs off progressively if the header is missing. Weather layers use separate providers.

The device reconnects after losing Wi-Fi. When the signal stays weak, it looks for an access point on the same network name and security mode with a signal at least 10 dB stronger. Roaming preserves the flight request schedule.

## Controls

| Action | How |
|---|---|
| Open the menu | Tap **☰** or press **M**. Keys **1–5** open Airspace, Instruments, Settings, Rain radar and Heat map. |
| Zoom or inspect a flight | Tap **+ / −** or press **I / O**. Open the list with the lower-left list icon. Tap it again, tap **×**, or press **F** to return to the full map. Swipe the side card to scroll. |
| Read aircraft symbols | A filled plane has a position at most 90 seconds old. An outline marks an older position. A **?** means the heading is unknown. States marked **\*** are estimates from movement. |
| Read the weather | RainViewer supplies up to three recent frames. Tap the bottom time label to pause or resume. The heat map uses nine Open-Meteo samples or nearby METAR airport observations. Both weather views refresh about every ten minutes. |
| Check hardware | Open **Instruments** (key **2**). Press **B** to test the speaker or **R** to retry mounting the card. The battery icon shows estimated charge on battery power or **USB** on detected external power. The blue LED indicates charging. |
| Numbers and symbols | Tap **SYM** on screen while editing. On a newer matrix-mode keyboard, press the physical **SYM** key to toggle. On older keyboards, hold **SYM** and press **Q**. Repeat to switch back. With **SYM ON**, **W E R** type **1 2 3**, **Space** types **0**, and **Q** types **#**. The separate on-screen **0** button works in either mode. |
| Keyboard lighting | On older keyboards, hold **ALT** and press **B** to turn the light on or off. It can stay lit while the screen saver is active. On newer controllers, adjust the brightness under **Settings → Keyboard light (%)**. |
| Change appearance | On the last Settings page, select a language, theme and sleep delay. The default is 30 minutes. Enter **0** to disable sleep. A touch or key wakes the screen. |

## Build from source

Install Git for Windows and open PowerShell in the project folder. The first command downloads Arduino CLI, ESP32 Arduino core 2.0.17, the pinned LilyGO drivers and the sensor libraries. The second command rebuilds with those tools already installed.

```powershell
.\build.ps1 -Setup
.\build.ps1
```

Your new images are in `firmware/`. Use those paths in the flashing commands to install your own build. The map packs in `maps/` are included at build time. Regeneration is optional. [Map sources and regeneration](maps/README.md)

To prepare a release, fetch tags (`git fetch --tags`), set the new version in `FlightDeck/firmware_version.h`, then run `py tools/prepare_rollback.py` before building. Publish the generated catalog and `install/archive/` files with the firmware. Older devices use the same permanent archive URLs.

## Troubleshooting and privacy

- **No flights or weather:** check Wi-Fi and the clock in Instruments. The device requires a valid date for HTTPS. Confirm your manual map centre if GPS has no fix.
- **No GPS data:** connect a 3.3 V UART module's TX to the configured T-Deck RX and share ground. GPIO44 is the default, GPIO43 is selectable. Baud rate **0** tries supported rates automatically.
- **SD error:** reinsert the card and press **R** in Instruments. The firmware verifies the base map when loading it. Optional minute-by-minute environmental logging requires a writable card and **SD logging = 1**.
- **Serial diagnostics:** use 115200 baud. Send `?` for status, `X` for the input and hardware test, `Y` for its counters, and `E` to exit the test.
- Wi-Fi settings stay on the device. Online flight and weather queries send the selected map centre or surrounding area to their providers. Keep flash backups, serial logs and SD logs private. [adsb.lol](https://api.adsb.lol/docs) · [RainViewer](https://www.rainviewer.com/api/weather-maps-api.html) · [Open-Meteo](https://open-meteo.com/en/docs) · [Aviation Weather Center](https://aviationweather.gov/data/api/)

[Hardware documentation](docs/hardware.md) · [Data, artwork and third-party notices](maps/README.md)
