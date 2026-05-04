import { TikTokLiveConnection, WebcastEvent } from 'tiktok-live-connector';
import { WebSocketServer } from 'ws';
import fs from 'fs';
import path from 'path';
import sharp from 'sharp';

const TIKTOK_USERNAME = "kyro1eu";
const PFP_DIR = "./pfp_cache";

if (!fs.existsSync(PFP_DIR)) {
  fs.mkdirSync(PFP_DIR);
}

const wss = new WebSocketServer({ port: 8080 });
console.log("WS server running on ws://localhost:8080");

wss.on('connection', () => {
  console.log("C++ client connected");
});

const tiktok = new TikTokLiveConnection(TIKTOK_USERNAME, {
  // Enables gift name, cost and images, and allows fetching the full gift catalog.
  enableExtendedGiftInfo: false,
  enableWebsocketUpgrade: true,
  disableEulerFallbacks: true
});

const GIFTS_CACHE_PATH = path.resolve("./gift_ids.json");

async function connectTikTok() {
  try {
    await tiktok.connect();
    console.log("Connected to TikTok");

    // Fetch full gift catalog (id -> name). This is the reliable way to discover gift IDs.
    try {
      const gifts = await tiktok.fetchAvailableGifts();
      console.log(`Fetched ${gifts?.length || 0} available gifts`);

      // Persist as a simple { [id]: name } map for convenience.
      const idMap = {};
      for (const g of (gifts || [])) {
        if (!g) continue;
        const id = Number(g.id);
        const name = String(g.name || "").trim();
        if (Number.isFinite(id) && id > 0 && name) idMap[id] = name;
      }
      fs.writeFileSync(GIFTS_CACHE_PATH, JSON.stringify(idMap, null, 2));
      console.log("Saved gift id map:", GIFTS_CACHE_PATH);
    } catch (e) {
      console.log("fetchAvailableGifts failed:", e?.message || e);
    }
  } catch (err) {
    console.log("TikTok connect failed, retrying...");
    setTimeout(connectTikTok, 3000);
  }
}

connectTikTok();

// Authoritative gift ID -> display name map. IDs verified against the
// public catalog dump (alberand gist) and the TikTok webcast catalog.
// Names sent here are what the C++ AbilityFromGift() switch matches on.
const giftMap = {
  // Basic / spammable (1-30 diamonds)
  5269: "TikTok",          // 1
  5655: "Rose",            // 1
  5760: "Weights",         // 1
  5827: "Ice Cream Cone",  // 1
  5874: "Lion 222",        // 1
  5487: "Finger Heart",    // 5
  5650: "Mic",             // 5
  5657: "Lollipop",        // 10
  5919: "Love you",        // 20
  5658: "Perfume",         // 20
  5879: "Doughnut",        // 30

  // Mid (49-499 diamonds)
  5707: "Love you",        // 49
  5659: "Paper Crane",     // 99
  5660: "Hand Hearts",     // 100
  5915: "Music Note",      // 169
  5586: "Hearts",          // 199
  5734: "Goggles",         // 199
  5880: "Lock and Key",    // 199
  5882: "Rock 'n' Roll",   // 299
  5661: "Air Dancer",      // 300
  5899: "Swing",           // 399
  5662: "Necklace",        // 400
  5731: "Coral",           // 499

  // High (500-1500 diamonds)
  5764: "Ice Machine",     // 538
  5663: "Heels",           // 700
  5488: "LOVE Balloon",    // 699
  5897: "Swan",            // 699
  5651: "Garland",         // 1500

  // Premium (1500-5000 diamonds)
  5730: "Treehouse",       // 1799
  5763: "Speedboat",       // 1888
  5489: "Carousel",        // 2020
  5765: "Motorcycle",      // 2988
  5652: "Ferris Wheel",    // 3000
  5767: "Private Jet",     // 4888
  5938: "Pool Party",      // 4999

  // Ultimate (5000+ diamonds)
  5732: "Submarine",       // 5199
  6149: "Interstellar",    // 10000
  5954: "Planet",          // 15000
  5930: "Rocket",          // 20000
  6223: "Lion",            // 29999

  // Legacy aliases kept for backward compatibility / regional names.
  5566: "Finger Heart",
  9875: "Shamrock",
  8913: "Rosa",
  6064: "GG",
};

// Approximate diamond cost per id, used as a fallback signal in C++ when
// a gift name can't be matched directly. Only filled where TikTok's own
// `data.gift.diamondCount` may be missing.
const diamondMap = {
  5269: 1,    5655: 1,    5760: 1,    5827: 1,    5874: 1,
  5487: 5,    5650: 5,
  5657: 10,
  5919: 20,   5658: 20,
  5879: 30,
  5707: 49,
  5659: 99,
  5660: 100,
  5915: 169,
  5586: 199,  5734: 199,  5880: 199,
  5882: 299,
  5661: 300,
  5899: 399,
  5662: 400,
  5731: 499,
  5764: 538,
  5663: 700,  5488: 699,  5897: 699,
  5651: 1500,
  5730: 1799,
  5763: 1888,
  5489: 2020,
  5765: 2988,
  5652: 3000,
  5767: 4888,
  5938: 4999,
  5732: 5199,
  6149: 10000,
  5954: 15000,
  5930: 20000,
  6223: 29999,
};

function loadGiftCache() {
  try {
    if (!fs.existsSync(GIFTS_CACHE_PATH)) return {};
    const raw = fs.readFileSync(GIFTS_CACHE_PATH, "utf8");
    const parsed = JSON.parse(raw);
    return parsed && typeof parsed === "object" ? parsed : {};
  } catch {
    return {};
  }
}

let giftCache = loadGiftCache();

function saveGiftCache() {
  try {
    fs.writeFileSync(GIFTS_CACHE_PATH, JSON.stringify(giftCache, null, 2));
  } catch {}
}

function getUserName(data) {
  return data?.user?.nickname || data?.user?.uniqueId || "unknown";
}

function getUserId(data) {
  return data?.user?.userId || "unknown";
}

function getGiftName(data) {
  const id = data?.giftId;

  if (id && giftMap[id]) return giftMap[id];
  if (data?.gift?.name) return data.gift.name;
  if (id && giftCache[id]) return giftCache[id];

  if (id && Array.isArray(tiktok.availableGifts)) {
    const g = tiktok.availableGifts.find(x => Number(x?.id) === Number(id));
    if (g?.name) {
      giftCache[id] = String(g.name);
      saveGiftCache();
      return giftCache[id];
    }
  }

  if (id) return `gift_${id}`;

  return "unknown_gift";
}

function getDiamondCount(data) {
  const live = Number(
    data?.gift?.diamondCount ??
    data?.diamondCount ??
    data?.diamond_count ??
    NaN
  );
  if (Number.isFinite(live) && live > 0) return live;

  const id = Number(data?.giftId);
  if (Number.isFinite(id) && diamondMap[id]) return diamondMap[id];

  if (id && Array.isArray(tiktok.availableGifts)) {
    const g = tiktok.availableGifts.find(x => Number(x?.id) === id);
    const dc = Number(g?.diamond_count);
    if (Number.isFinite(dc) && dc > 0) return dc;
  }

  return 0;
}

function getPfpUrl(data) {
  const urls = data?.user?.profilePicture?.url;
  if (!urls) return "";
  const jpeg = urls.find(u => u.includes(".jpeg"));
  return jpeg || urls[0];
}

async function downloadPfp(url, userId) {
  if (!url || typeof url !== "string") return "";

  const filePath = path.join(PFP_DIR, `${userId}.png`);
  const tempPath = filePath + ".tmp";
  if (fs.existsSync(filePath)) {
    return path.resolve(filePath);
  }

  try {
    if (typeof fetch !== "function") {
      return "";
    }

    const res = await fetch(url);
    if (!res.ok) return "";

    const buffer = await res.arrayBuffer();

    await sharp(Buffer.from(buffer))
      .resize(128, 128)
      .png()
      .toFile(tempPath);

    fs.renameSync(tempPath, filePath);

    console.log("Saved PFP:", filePath);

    return path.resolve(filePath);

  } catch (err) {
    console.log("Download error:", err.message);
    return "";
  }
}

function getLocalPfp(userId) {
  const filePath = path.resolve(`${PFP_DIR}/${userId}.png`);
  return fs.existsSync(filePath) ? filePath : "";
}

function broadcast(data) {
  const msg = JSON.stringify(data);

  wss.clients.forEach(client => {
    if (client.readyState === 1) {
      client.send(msg);
    }
  });
}

tiktok.on(WebcastEvent.CHAT, async data => {
  const user = getUserName(data);
  const userId = getUserId(data);
  const message = String(data.comment || "");
  const pfpUrl = getPfpUrl(data);

  console.log("CHAT:", user, "->", message);

  const pfp = await downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "chat",
    user,
    userId,
    message,
    pfp
  });
});

tiktok.on(WebcastEvent.GIFT, async data => {
  if (!data) return;
  if (data.repeatEnd === false) return;

  const user = getUserName(data);
  const userId = getUserId(data);
  const giftId = Number(data?.giftId) || 0;
  const giftName = getGiftName(data);
  const diamondCount = getDiamondCount(data);
  const amount = data.repeatCount || 1;
  const pfpUrl = getPfpUrl(data);

  console.log("GIFT:", user, giftName, `(id=${giftId}, dmnd=${diamondCount}) x`, amount);

  const pfp = await downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "gift",
    user,
    userId,
    gift: giftName,
    giftId,
    diamondCount,
    amount,
    pfp
  });
});

tiktok.on(WebcastEvent.MEMBER, async data => {
  const user = getUserName(data);
  const userId = getUserId(data);
  const pfpUrl = getPfpUrl(data);

  console.log("JOIN:", user);

  const pfp = await downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "join",
    user,
    userId,
    pfp
  });
});

tiktok.on(WebcastEvent.LIKE, async data => {
  const user = getUserName(data);
  const userId = getUserId(data);
  const pfpUrl = getPfpUrl(data);

  console.log("LIKE:", user, "count:", data.likeCount);

  const pfp = await downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "like",
    user,
    userId,
    count: data.likeCount,
    pfp
  });
});

tiktok.on('error', err => {
  console.error("TikTok ERROR:", err);
});

tiktok.on('disconnected', () => {
  console.log("TikTok disconnected");
});