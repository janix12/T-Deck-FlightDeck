// Digests for the firmware files offered by this installer.
const images = {
  update: {
    filename: "FlightDeck.ino.bin", address: 0x10000,
    sha256: "11575905b166b1e8f0c5f26f35dbdc082afc7f378557b344a56a96ddde48e56a",
    md5: "91a134c40316628dfff0df5ed41e71c9"
  },
  full: {
    filename: "FlightDeck-full.bin", address: 0,
    sha256: "9202d9b8ba135cc262d63bb3df4ff6526b18394886d30b2da99e94cb6d4854bf",
    md5: "81fc15846d37c2e0d16225406ea32c94"
  }
};

export async function installFirmware(loader, mode, status, progress) {
  const image = images[mode];
  status("download");
  const url = new URL("../install/" + image.filename, import.meta.url);
  url.searchParams.set("sha256", image.sha256);
  const response = await fetch(url);
  if (!response.ok) throw new Error("downloadError");
  const data = new Uint8Array(await response.arrayBuffer());
  const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", data));
  const hex = Array.from(digest, byte => byte.toString(16).padStart(2, "0")).join("");
  if (hex !== image.sha256) throw new Error("digestError");

  status("connect");
  await loader.main();
  if (loader.chip.CHIP_NAME !== "ESP32-S3") throw new Error("chipError");
  if (loader.secureDownloadMode || !loader.IS_STUB) throw new Error("securityError");
  const security = await loader.getSecurityInfo();
  if (security.parsedFlags.SECURE_BOOT_EN || security.flashCryptCnt !== 0) throw new Error("securityError");
  if (await loader.detectFlashSize() !== "16MB") throw new Error("sizeError");
  if (mode === "update" && await loader.flashMd5sum(0x8000, 0xc00) !== "ba686c6c6fdf2b963448683d5bbd8380") {
    throw new Error("layoutError");
  }

  status("write");
  await loader.writeFlash({
    fileArray: [{ address: image.address, data }],
    flashSize: "keep", flashMode: "keep", flashFreq: "keep",
    compress: true, eraseAll: false,
    calculateMD5Hash: () => image.md5,
    reportProgress: (_index, sent, total) => {
      progress(Math.min(100, Math.floor(100 * sent / total)));
      if (sent >= total) status("verify");
    }
  });
  try {
    await loader.after("hard_reset");
    return "done";
  } catch {
    return "manualReset";
  }
}
