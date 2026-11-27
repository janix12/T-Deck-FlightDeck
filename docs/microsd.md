# microSD · FlightDeck

[Magyar](#magyar) · [English](#english)

## Magyar

**Egyszerű választás egy 8, 16 vagy 32 GB-os microSDHC, egyetlen elsődleges MBR-partícióval, FAT32-re formázva a kártya teljes méretére.** A formázó alapértelmezett klasztermérete megfelelő. A formázás törli az adatokat.

| Kártyacsalád | Névleges kártyaméret | Szokásos formátum és FlightDeck-támogatás |
|---|---|---|
| microSD (SDSC) | legfeljebb 2 GB | FAT16. A nagyon kis, FAT12-es köteteket ez a firmware nem kezeli. |
| microSDHC | 2 GB felett, 32 GB-ig | FAT32. Egy kötet használhatja a teljes kártyát. |
| microSDXC | 32 GB felett, 2 TB-ig | exFAT. Ezt a fájlrendszert kezeli a meghajtó. A nagy kapacitásokat fizikai kártyával még nem igazoltuk. |
| microSDUC | 2 TB felett | Ebben a firmware-ben nem támogatott. |

**Partíció.** A készülék az MBR első bejegyzését próbálja megnyitni. Utána a 0. szektorban kezdődő, partíciós tábla nélküli fájlrendszerrel próbálkozik. Az elsődleges partíció kisebb is lehet a kártyánál. A többi partíció helyét nem adja hozzá. GPT, NTFS, ext4 és titkosított kötet nem használható. A meghajtó 512 bájtos logikai szektorokkal dolgozik. FAT16/FAT32 esetén a formázó hagyományos, két FAT-táblás beállítását használd.

**Térkép és szabad hely.** A jelenlegi `/WORLD.FDM` mérete 5 835 336 bájt. Frissítéskor az előző fájl mellett el kell férnie az új példánynak is. A jelenlegi csomaghoz legalább 6 MB szabad helyet hagyj, a naplózáshoz ezen felül számolj hellyel. A **Műszerek → microSD** a csatolt kötet szabad és teljes kapacitását mutatja. A firmware nem particionálja vagy formázza a kártyát.

**Mit próbáltunk ki?** A T-Deck egy 1906 MiB-os (1,86 GiB) FAT16-kötetet felismert. Ezen a kártyán az írás utáni visszaolvasás hibázott. A kötet felismerése önmagában nem igazol sikeres írást. Az 1,86 GiB a mért kötet mérete. A 4 GB körüli FAT32-korlát egyetlen fájl méretére vonatkozik. A táblázatban a kártyacsaládok szabványos tartományai szerepelnek.

## English

**For a straightforward setup, use an 8, 16 or 32 GB microSDHC card with one primary MBR partition formatted as FAT32 across the whole card.** Use the formatter's default cluster size. Formatting erases existing files.

| Card family | Nominal card capacity | Usual format and FlightDeck support |
|---|---|---|
| microSD (SDSC) | Up to 2 GB | FAT16. Very small FAT12 volumes are unsupported in this build. |
| microSDHC | Over 2 GB through 32 GB | FAT32. One volume can span the card. |
| microSDXC | Over 32 GB through 2 TB | exFAT. The driver supports this filesystem. Large capacities have not been verified with physical cards. |
| microSDUC | Over 2 TB | Unsupported by this firmware. |

**Partition layout.** FlightDeck tries the first MBR partition entry, then a filesystem starting directly at sector zero. A smaller first partition also works. Additional partitions do not contribute space. GPT, NTFS, ext4 and encrypted volumes are unsupported. The driver uses 512-byte logical sectors. For FAT16/FAT32 use the usual two FAT tables.

**Map space.** The current `/WORLD.FDM` is 5,835,336 bytes. During updates, the new file must fit alongside the existing one. Keep at least 6 MB free for the current package, plus space for any logs. **Instruments → microSD** reports free and total space on the mounted volume. FlightDeck does not partition or format the card.

**Hardware test.** The T-Deck mounted a 1906 MiB (1.86 GiB) FAT16 volume. Write verification failed on that particular card. Mounting a volume alone does not establish that it retains writes. The measured volume was 1.86 GiB. FAT32's roughly 4 GB limit applies to a single file. The table lists the standard capacity classes.

[SD Association · Capacity standards](https://www.sdcard.org/developers/sd-standard-overview/capacity-sd-sdhc-sdxc-sduc/) · [SdFat 2.3.0 · Mount order](https://github.com/greiman/SdFat/blob/2.3.0/src/SdFat.h#L349-L351) · [SdFat 2.3.0 · Volume selection](https://github.com/greiman/SdFat/blob/2.3.0/src/FsLib/FsVolume.h) · [SdFat 2.3.0 · Configuration](https://github.com/greiman/SdFat/blob/2.3.0/src/SdFatConfig.h)
