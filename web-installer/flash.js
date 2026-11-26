// These digests identify the two published v1.0.1 release assets.
const images = {
  update: {
    filename: "FlightDeck.ino.bin", address: 0x10000,
    sha256: "dab1c7a0f04a26f50a3f3c19f57eb34737d01058d251ede010a0aed7e7adcedb",
    md5: "df16c7ed5126f5b24a113b741ebe86cb"
  },
  full: {
    filename: "FlightDeck-full.bin", address: 0,
    sha256: "0608f5a9f8eb6e778db732dccb08ae3642411617d31d177f1d0676c727dc2d0b",
    md5: "22cd1fc0abbe363ef8188386df5b3d78"
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
