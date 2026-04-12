import { TikTokLiveConnection, WebcastEvent } from 'tiktok-live-connector';
import { WebSocketServer } from 'ws';
import fs from 'fs';
import path from 'path';
import sharp from 'sharp';

const TIKTOK_USERNAME = "irfanrifki123";
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
  enableExtendedGiftInfo: false,
  enableWebsocketUpgrade: true,
  disableEulerFallbacks: true
});

async function connectTikTok() {
  try {
    await tiktok.connect();
    console.log("Connected to TikTok");
  } catch (err) {
    console.log("TikTok connect failed, retrying...");
    setTimeout(connectTikTok, 3000);
  }
}

connectTikTok();

const giftMap = {
  5655: "Rose",
  5487: "TikTok",
  5566: "Finger Heart",
  5658: "Perfume",
  5760: "Galaxy",
  9875: "Shamrock",
  8913: "Rosa",
  5879: "Donut",
  6064: "GG"
};

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
  if (id) return `gift_${id}`;

  return "unknown_gift";
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
      .png()
      .toFile(filePath);

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

tiktok.on(WebcastEvent.CHAT, data => {
  const user = getUserName(data);
  const userId = getUserId(data);
  const message = String(data.comment || "");
  const pfpUrl = getPfpUrl(data);

  console.log("CHAT:", user, "->", message);

  downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "chat",
    user,
    userId,
    message,
    pfp: getLocalPfp(userId)
  });
});

tiktok.on(WebcastEvent.GIFT, data => {
  if (!data) return;
  if (data.repeatEnd === false) return;

  const user = getUserName(data);
  const userId = getUserId(data);
  const giftName = getGiftName(data);
  const amount = data.repeatCount || 1;
  const pfpUrl = getPfpUrl(data);

  console.log("GIFT:", user, giftName, "x", amount);

  downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "gift",
    user,
    userId,
    gift: giftName,
    amount,
    pfp: getLocalPfp(userId)
  });
});

tiktok.on(WebcastEvent.MEMBER, data => {
  const user = getUserName(data);
  const userId = getUserId(data);
  const pfpUrl = getPfpUrl(data);

  console.log("JOIN:", user);

  downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "join",
    user,
    userId,
    pfp: getLocalPfp(userId)
  });
});

tiktok.on(WebcastEvent.LIKE, data => {
  const user = getUserName(data);
  const userId = getUserId(data);
  const pfpUrl = getPfpUrl(data);

  console.log("LIKE:", user, "count:", data.likeCount);

  downloadPfp(pfpUrl, userId).catch(() => {});

  broadcast({
    type: "like",
    user,
    userId,
    count: data.likeCount,
    pfp: getLocalPfp(userId)
  });
});

tiktok.on('error', err => {
  console.error("TikTok ERROR:", err);
});

tiktok.on('disconnected', () => {
  console.log("TikTok disconnected");
});