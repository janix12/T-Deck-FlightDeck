import { installFirmware } from "./flash.js?v=versions-2";

const en = {
  wifiDemoCaption: "Wi-Fi disconnecting and reconnecting on the T-Deck", wifiDemoAlt: "Three Wi-Fi waves change to a crossed-out icon when disconnected, a circular arrow while reconnecting, then three waves again.",
  title: "USB installer",
  version: "Firmware version", latest: "latest", catalogError: "Could not load the firmware versions. Reload the page to try again.",
  intro: "Connect your original T-Deck to your computer with a USB data cable, switch it on and close any serial monitor.",
  mode: "Installation mode", update: "Update FlightDeck", updateHelp: "Keep the settings on a T-Deck already running FlightDeck.",
  full: "First installation", fullHelp: "The full firmware replaces the previous system and its saved settings.",
  button: "Connect and install", support: "Use desktop Chrome or Edge. After clicking, select the T-Deck’s USB port.",
  trafficCaption: "Live traffic · 29 aircraft around Budapest", trafficAlt: "The Airspace title shows active Wi-Fi and a count of 29 aircraft, with aircraft icons on the map.",
  caption: "Aircraft list · DLH6CH selected", alt: "Flight DLH6CH selected in the list of aircraft around Budapest.",
  followControlsCaption: "Follow and Solo aircraft", followControlsAlt: "The Follow and Solo aircraft switches are both on in the aircraft details.",
  followMapCaption: "Tracking DLH6CH on the full map", followMapAlt: "The icon and callsign of the tracked flight DLH6CH in the centre of the map.",
  limitExample: "Earlier example of the API limit notice ↗",
  help: "T-Deck won’t connect?", boot: "Hold down the centre of the trackball, press and release RST, then release the trackball. Try connecting again. If the port is missing, check the data cable and close other apps using the port.",
  manual: "Download firmware and manual instructions ↗",
  unsupported: "This browser cannot access the USB serial port. Open this page in desktop Chrome or Edge, or use the manual instructions.",
  picker: "Select the T-Deck’s USB port in the browser.", load: "Loading the USB installer…", download: "Downloading and checking the firmware…",
  connect: "Connecting and checking the device…", write: "Writing firmware. Keep the T-Deck connected.", verify: "Verifying the firmware on the device…",
  done: "Installation verified. If FlightDeck does not start, press RST.", manualReset: "Installation verified. Press RST to start FlightDeck.",
  cancel: "No port selected. You can try again.", downloadError: "The firmware could not be downloaded. Check your connection and try again.",
  digestError: "The download does not match the firmware for this release. Reload the page before trying again.",
  chipError: "This firmware requires the ESP32-S3 in the original LilyGO T-Deck.", sizeError: "Expected a T-Deck with 16 MB of flash. Installation stopped.",
  securityError: "This device’s flash protection or loader is incompatible with this installer.",
  layoutError: "The partition table differs from FlightDeck v1.0. To replace the existing system and its settings, choose First installation.",
  connectError: "Could not connect. Close any serial monitor and follow the bootloader steps under “T-Deck won’t connect?”.",
  writeError: "Installation could not be verified. Keep the USB cable connected, enter bootloader mode and retry the same installation.",
  loadError: "Could not load the USB installer. Reload the page and try again."
};
const hu = {
  title: "USB-telepítő",
  latest: "legfrissebb", catalogError: "A verziólista betöltése sikertelen. Próbáld újra az oldal frissítésével.",
  unsupported: "Ebben a böngészőben a soros USB-port nem érhető el. Nyisd meg az oldalt asztali Chrome-ban vagy Edge-ben, vagy válaszd a kézi telepítést.",
  picker: "Válaszd ki a T-Deck USB-portját a böngészőben.", load: "Az USB-telepítő betöltése…", download: "Firmware letöltése és ellenőrzése…",
  connect: "Csatlakozás és az eszköz ellenőrzése…", write: "Firmware írása. Hagyd csatlakoztatva az USB-kábelt.", verify: "A felírt firmware ellenőrzése…",
  done: "A telepítés ellenőrizve. Ha a FlightDeck nem indul el, nyomd meg az RST gombot.", manualReset: "A telepítés ellenőrizve. A FlightDeck indításához nyomd meg az RST gombot.",
  cancel: "Nem választottál portot. Újra próbálkozhatsz.", downloadError: "A firmware letöltése sikertelen. Ellenőrizd az internetkapcsolatot, majd próbáld újra.",
  digestError: "A letöltött fájl eltér a kiadáshoz tartozó firmware-től. Töltsd újra az oldalt, majd próbáld újra.",
  chipError: "Ez a firmware az eredeti LilyGO T-Deck ESP32-S3 chipjéhez készült.", sizeError: "A telepítéshez 16 MB flashmemóriás T-Deck szükséges. Az írás leállt.",
  securityError: "Az eszköz flashvédelme vagy betöltője nem kompatibilis ezzel a telepítővel.",
  layoutError: "A partíciók eltérnek a FlightDeck v1.0 kiosztásától. Ha felül szeretnéd írni a meglévő rendszert és a beállításait, válaszd az Első telepítést.",
  connectError: "Sikertelen csatlakozás. Zárd be a soros monitort, majd kövesd a „Nem csatlakozik a T-Deck?” alatt a betöltő mód lépéseit.",
  writeError: "A telepítést nem sikerült igazolni. Hagyd az USB-kábelt csatlakoztatva, lépj betöltő módba, majd ismételd meg ugyanazt a telepítést.",
  loadError: "Az USB-telepítőt nem sikerült betölteni. Töltsd újra az oldalt, majd próbáld újra."
};
const requested = new URLSearchParams(location.search).get("lang");
const lang = ["hu", "en"].includes(requested) ? requested : navigator.language.startsWith("hu") ? "hu" : "en";
const t = lang === "hu" ? hu : en;
document.documentElement.lang = lang;
document.title = "FlightDeck · " + t.title;
document.querySelector('a[hreflang="' + lang + '"]').setAttribute("aria-current", "page");
if (lang === "en") {
  for (const element of document.querySelectorAll("[data-t]")) element.textContent = en[element.dataset.t];
  for (const image of document.querySelectorAll("[data-alt]")) image.alt = en[image.dataset.alt];
  for (const image of document.querySelectorAll("[data-en-src]")) image.src = image.dataset.enSrc;
}
const button = document.querySelector("#install");
const modes = document.querySelector("#modes");
const versionSelect = document.querySelector("#firmware-version");
const manual = document.querySelector("#manual");
const feedback = document.querySelector("#feedback");
const bar = document.querySelector("#progress");
const percent = document.querySelector("#percent");
let phase;
let busy = false;
let releases = [];

function status(key, state = "") {
  phase = key;
  document.querySelector("#status").textContent = t[key];
  feedback.dataset.state = state;
}
function progress(value) {
  bar.hidden = false;
  bar.value = value;
  percent.textContent = value + "%";
}
function preventLeave(event) {
  event.preventDefault();
  event.returnValue = "";
}
function updateManualLink() {
  const selected = releases[versionSelect.selectedIndex];
  const readme = lang === "hu" ? "README.hu.md" : "README.md";
  manual.href = "https://github.com/janix12/T-Deck-FlightDeck/blob/v" + selected.version + "/" + readme;
}
async function loadVersions() {
  try {
    const response = await fetch(new URL("../install/usb-releases.json", import.meta.url), { cache: "no-cache" });
    if (!response.ok) throw new Error();
    const catalog = await response.json();
    if (!Array.isArray(catalog) || !catalog.length) throw new Error();
    releases = catalog.map((entry, i) => {
      if (!/^\d+\.\d+\.\d+$/.test(entry.version) || ![entry.full, entry.update].every(
        image => image && /^[a-f0-9]{64}$/.test(image.sha256) && /^[a-f0-9]{32}$/.test(image.md5)
      )) throw new Error();
      return { ...entry, directory: i === 0 ? "" : "archive/" + entry.version + "/" };
    });
    for (const [i, release] of releases.entries()) {
      const caption = "v" + release.version + (i === 0 ? " · " + t.latest : "");
      versionSelect.add(new Option(caption, release.version, i === 0, i === 0));
    }
    versionSelect.disabled = false;
    updateManualLink();
    if (window.isSecureContext && navigator.serial?.requestPort) modes.disabled = button.disabled = false;
    else status("unsupported");
  } catch {
    status("catalogError", "error");
  }
}
versionSelect.addEventListener("change", () => {
  updateManualLink();
  if (!button.disabled) {
    feedback.dataset.state = "";
    document.querySelector("#status").textContent = percent.textContent = "";
    bar.hidden = true;
  }
});
loadVersions();
button.addEventListener("click", async () => {
  if (busy) return;
  const release = releases[versionSelect.selectedIndex];
  if (!release) return;
  busy = modes.disabled = button.disabled = versionSelect.disabled = true;
  const mode = document.querySelector("input[name=mode]:checked").value;
  bar.hidden = true;
  percent.textContent = "";
  status("picker");
  let transport;
  window.addEventListener("beforeunload", preventLeave);
  try {
    const port = await navigator.serial.requestPort({ filters: [{ usbVendorId: 0x303a, usbProductId: 0x1001 }] });
    status("load");
    const { Transport, ESPLoader } = await import("./vendor/esptool-js-0.7.1.js");
    transport = new Transport(port, false);
    const terminal = { clean() {}, write() {}, writeLine() {} };
    const loader = new ESPLoader({ transport, terminal, baudrate: 115200, romBaudrate: 115200 });
    status(await installFirmware(loader, release, mode, status, progress), "success");
  } catch (error) {
    const fallback = { picker: "connectError", download: "downloadError", write: "writeError", verify: "writeError", load: "loadError" };
    const key = phase === "picker" && error.name === "NotFoundError" ? "cancel"
      : Object.hasOwn(hu, error.message) ? error.message : fallback[phase] || "connectError";
    status(key, key === "cancel" ? "" : "error");
  } finally {
    if (transport) {
      try { await transport.disconnect(); } catch { /* The port may already be closed. */ }
    }
    window.removeEventListener("beforeunload", preventLeave);
    busy = modes.disabled = button.disabled = versionSelect.disabled = false;
  }
});
