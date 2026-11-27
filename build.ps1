param([switch]$Setup)
$ErrorActionPreference = 'Stop'
if ($PSScriptRoot.Length -gt 15) {
    # The Windows Xtensa toolchain expands nested relative paths past MAX_PATH.
    $letter = 90..69 | ForEach-Object { [char]$_ } | Where-Object { !(Test-Path "${_}:\") } | Select-Object -First 1
    if (!$letter) { throw 'Nincs szabad ideiglenes meghajtobetujel.' }
    $drive = "${letter}:"
    subst $drive $PSScriptRoot
    if ($LASTEXITCODE) { throw 'Nem sikerult rovid utvonalat letrehozni.' }
    try { & "$drive\build.ps1" -Setup:$Setup } finally { Set-Location $PSScriptRoot; subst $drive /D }
    return
}
Set-Location $PSScriptRoot
$env:ARDUINO_DIRECTORIES_DATA = "$PSScriptRoot\.tools\arduino-data"
$env:ARDUINO_DIRECTORIES_DOWNLOADS = "$PSScriptRoot\.tools\downloads"
$env:ARDUINO_DIRECTORIES_USER = "$PSScriptRoot\.tools\sketchbook"
$cli = "$PSScriptRoot\.tools\arduino-cli\arduino-cli.exe"
$vendor = "$PSScriptRoot\.tools\lilygo"
$index = 'https://espressif.github.io/arduino-esp32/package_esp32_index.json'
$revision = '9aac147a313975ad063594b3fe11e28a96d94936'
function Check-Result { if ($LASTEXITCODE -ne 0) { throw "Hibakod: $LASTEXITCODE" } }
if ($Setup) {
    New-Item -ItemType Directory -Force .tools | Out-Null
    if (!(Test-Path $cli)) {
        Invoke-WebRequest 'https://downloads.arduino.cc/arduino-cli/arduino-cli_1.6.0_Windows_64bit.zip' -OutFile .tools/arduino.zip
        Expand-Archive .tools/arduino.zip .tools/arduino-cli
    }
    if (!(Test-Path $vendor)) {
        git clone https://github.com/Xinyuan-LilyGO/T-Deck.git $vendor
        Check-Result
        git -C $vendor checkout --detach $revision
        Check-Result
    }
    & $cli core update-index --additional-urls $index
    Check-Result
    & $cli core install esp32:esp32@2.0.17 --additional-urls $index
    Check-Result
    & $cli lib update-index
    Check-Result
    & $cli lib install 'ArduinoJson@7.4.2' 'Adafruit BMP280 Library@3.0.0' 'Adafruit BME280 Library@2.3.0' 'Adafruit BMP3XX Library@2.1.6' 'Adafruit DPS310@1.1.6' 'Adafruit SHT31 Library@2.2.2' 'SdFat@2.3.0' 'StreamUtils@1.9.2' 'PNGdec@1.1.6'
    Check-Result
}
if ((git -C $vendor rev-parse HEAD) -ne $revision) { throw 'A LilyGO driververzio elter a rogzitett valtozattol.' }
$fqbn = 'esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB'
& $cli compile --jobs 6 --fqbn $fqbn --build-property "compiler.S.extra_flags=-Wa,-I$PSScriptRoot/maps" --libraries "$vendor\lib" --build-path "$PSScriptRoot\.tools\build" --output-dir "$PSScriptRoot\firmware" FlightDeck
Check-Result
$esptool = "$env:ARDUINO_DIRECTORIES_DATA\packages\esp32\tools\esptool_py\4.5.1\esptool.exe"
$ota = "$env:ARDUINO_DIRECTORIES_DATA\packages\esp32\hardware\esp32\2.0.17\tools\partitions\boot_app0.bin"
& $esptool --chip esp32s3 merge_bin -o firmware/FlightDeck-full.bin --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 firmware/FlightDeck.ino.bootloader.bin 0x8000 firmware/FlightDeck.ino.partitions.bin 0xe000 $ota 0x10000 firmware/FlightDeck.ino.bin
Check-Result
$versionSource = Get-Content -LiteralPath FlightDeck/firmware_version.h -Raw
if ($versionSource -notmatch '#define FW_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"') { throw 'Invalid firmware version' }
$releaseVersion = $Matches[1]
$appImage = Get-Item -LiteralPath firmware/FlightDeck.ino.bin
$manifest = [ordered]@{
    device = 'lilygo-t-deck-esp32s3'
    version = $releaseVersion
    size = $appImage.Length
    sha256 = (Get-FileHash -LiteralPath $appImage.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
}
[IO.File]::WriteAllText((Join-Path $appImage.DirectoryName 'manifest.json'), ($manifest | ConvertTo-Json) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
