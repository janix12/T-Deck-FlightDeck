// These digests identify the two published v1.0 release assets.
const images = {
  update: {
    filename: "FlightDeck.ino.bin", address: 0x10000,
    sha256: "ff459edb676a80935a4e8c67e2d5f02a5179dc501efb16beab3bfc1c67b65d79",
    md5: "6ac1207653d2d1bd72f0fda584b3e875"
  },
  full: {
    filename: "FlightDeck-full.bin", address: 0,
    sha256: "b97e282e4a67b4ed1c2e004010f48f259b6e315fe7950eca82d4c15df8f88f3e",
    md5: "7e28d8a38bebd174d3cae2e67a51acf4"
  }
};

export async function installFirmware(loader, mode, status, progress) {
  const image = images[mode];
  status("download");
  const response = await fetch(new URL("../install/" + image.filename, import.meta.url));
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
