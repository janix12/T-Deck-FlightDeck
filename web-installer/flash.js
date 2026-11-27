// Digests for the firmware files offered by this installer.
const images = {
  update: {
    filename: "FlightDeck.ino.bin", address: 0x10000,
    sha256: "78c48bd2a3d5e2e2026a078e4e87637837a29f967ee8fb56041688117450f69e",
    md5: "550d9d975eaae2b5235e51f9efd9fbda"
  },
  full: {
    filename: "FlightDeck-full.bin", address: 0,
    sha256: "8c6990bc55c065cd3eec73b05120b90f62960aced379834dfeac18570e7a28c8",
    md5: "38f072401e9ad25f998850793494ad34"
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
  const parts = [{ address: image.address, data }];
  // An erased OTA selector boots app0. Write it only after app0 verifies.
  if (mode === "update") parts.push({ address: 0xe000, data: new Uint8Array(0x2000).fill(255) });
  await loader.writeFlash({
    fileArray: parts,
    flashSize: "keep", flashMode: "keep", flashFreq: "keep",
    compress: true, eraseAll: false,
    calculateMD5Hash: bytes => bytes.length === 0x2000 ? "84d04c9d6cc8ef35bf825d51a5277699" : image.md5,
    reportProgress: (index, sent, total) => {
      if (index > 0) return;
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
