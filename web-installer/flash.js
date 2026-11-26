// Digests for the firmware files offered by this installer.
const images = {
  update: {
    filename: "FlightDeck.ino.bin", address: 0x10000,
    sha256: "fda9c59c6bce468d0d1621baa40703f2aabdef9f5c41616b027e60694da7711f",
    md5: "2f13c37f56700aebf2726b474aad9917"
  },
  full: {
    filename: "FlightDeck-full.bin", address: 0,
    sha256: "ba1ff3c6e3e10fedf9dce0fa8686d6d819ac29c0b5f03d02651aa73a681d04be",
    md5: "7523641aff2d6c204f05ecf51bd5f922"
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
