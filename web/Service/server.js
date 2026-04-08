import { TikTokLiveConnection, WebcastEvent } from 'tiktok-live-connector';
import { WebSocketServer } from 'ws';

const TIKTOK_USERNAME = "irfanrifki123";

const wss = new WebSocketServer({ port: 8080 });

console.log("WS server running on ws://localhost:8080");

const giftMap = {
  5655: "Rose",
  5487: "TikTok",
  5566: "Finger Heart",
  5658: "Perfume",
  5760: "Galaxy",
  9875: "Shamrock",
  8913: "Rosa",
  5879: "Donut"
};

wss.on('connection', (ws) => {
  console.log("C++ client connected");

  ws.on('close', () => {
    console.log("C++ client disconnected");
  });
});

const tiktok = new TikTokLiveConnection(TIKTOK_USERNAME, {
  enableExtendedGiftInfo: false,
  enableWebsocketUpgrade: true,
  disableEulerFallbacks: true
});

try {
  await tiktok.connect();
  console.log("Connected to TikTok");
} catch (err) {
  console.error("Failed to connect to TikTok:", err.message);
}

function getUserName(data) {
  const user = data.user;

  if (user) {
    if (user.nickname && user.nickname !== "...") {
      return user.nickname;
    }

    if (user.uniqueId) {
      return user.uniqueId;
    }

    if (user.userId) {
      return user.userId;
    }
  }

  if (data.nickname && data.nickname !== "...") {
    return data.nickname;
  }

  if (data.uniqueId) {
    return data.uniqueId;
  }

  return "unknown";
}

function getGiftName(data) {
  const id = data?.giftId;

  if (id && giftMap[id]) {
    return giftMap[id];
  }

  if (data?.gift?.name) {
    return data.gift.name;
  }

  if (id) {
    return `gift_${id}`;
  }

  return "unknown_gift";
}

// -------------------------------
// Broadcast helper
// -------------------------------
function broadcast(data) {
  const msg = JSON.stringify(data);

  wss.clients.forEach(client => {
    if (client.readyState === 1) {
      client.send(msg);
    }
  });
}

// -------------------------------
// TikTok Events
// -------------------------------

// 💬 Chat
tiktok.on(WebcastEvent.CHAT, data => {
  const user = getUserName(data);

  console.log("CHAT:", user, data.comment);

  broadcast({
    type: "chat",
    user: user,
    message: data.comment
  });
});

// 🎁 Gift
tiktok.on(WebcastEvent.GIFT, data => {
  try {
    // skip broken events
    if (!data) return;

    // optional: wait for combo end
    if (data.repeatEnd === false) return;

    const user = getUserName(data);
    const giftName = getGiftName(data);
    const amount = data.repeatCount || 1;

    console.log("GIFT:", user, giftName, "x", amount);

    broadcast({
      type: "gift",
      user: user,
      gift: giftName,
      amount: amount
    });

  } catch (err) {
    console.log("Gift parse error (ignored):", err.message);
  }
});

// 👋 Join
tiktok.on(WebcastEvent.MEMBER, data => {
  const user = getUserName(data);

  console.log("JOIN:", user);

  broadcast({
    type: "join",
    user: user
  });
});

// ❤️ Likes
tiktok.on(WebcastEvent.LIKE, data => {
  const user = getUserName(data);

  console.log("LIKE:", user, data.likeCount);

  broadcast({
    type: "like",
    user: user,
    count: data.likeCount
  });
});

// -------------------------------
// Error handling
// -------------------------------
tiktok.on('error', err => {
  console.error("TikTok ERROR:", err);
});

tiktok.on('disconnected', () => {
  console.log("TikTok disconnected");
});
