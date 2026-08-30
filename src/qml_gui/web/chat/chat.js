// OWzx AI chat page logic. Talks to AiChatBridge over QWebChannel; renders
// the same card language as the previous QML sidebar (user bubbles right,
// activity cards, quiet query strips, permission cards, collapsed raw JSON).
// Security: assistant markdown is parsed with marked and sanitized with
// DOMPurify; every other value (user text, tool names, JSON) is inserted via
// textContent only.
"use strict";

(function () {
  const chat = document.getElementById("chat");
  const welcome = document.getElementById("welcome");
  const busyBar = document.getElementById("busy");
  const input = document.getElementById("input");
  const btnSend = document.getElementById("btnSend");
  const btnClear = document.getElementById("btnClear");
  const btnClose = document.getElementById("btnClose");
  const noticeDisabled = document.getElementById("noticeDisabled");
  const noticeNotInstalled = document.getElementById("noticeNotInstalled");

  let bridge = null;
  let state = { busy: false, available: false, enabled: false, installed: false };

  function text(el, str) { el.textContent = str == null ? "" : String(str); return el; }
  function el(tag, cls) { const e = document.createElement(tag); if (cls) e.className = cls; return e; }

  function scrollBottom() { chat.scrollTop = chat.scrollHeight; }

  function updateEmptyState() {
    const hasMessages = chat.querySelector(
      ".msg-user, .msg-assistant, .tool-card, .query-strip, .perm-card, .error-card");
    welcome.classList.toggle("hidden", !!hasMessages);
  }

  function updateInputState() {
    const canType = state.enabled && !state.busy;
    input.disabled = !canType;
    btnSend.disabled = !canType || input.value.trim().length === 0;
  }

  function updateNotices() {
    noticeDisabled.classList.toggle("hidden", state.enabled);
    noticeNotInstalled.classList.toggle(
      "hidden", !(state.enabled && !state.installed));
  }

  // ── renderers ────────────────────────────────────────────────────────────

  function appendUser(t) {
    const b = el("div", "msg-user");
    text(b, t);
    chat.appendChild(b);
  }

  function renderMarkdown(md) {
    const raw = window.marked ? window.marked.parse(String(md)) : String(md);
    return window.DOMPurify
      ? window.DOMPurify.sanitize(raw, { USE_PROFILES: { html: true } })
      : String(md);
  }

  function appendAssistant(t) {
    const b = el("div", "msg-assistant");
    b.innerHTML = renderMarkdown(t);
    chat.appendChild(b);
  }

  function spinner() {
    const s = el("span", "spinner");
    return s;
  }

  // entry: decorated tool entry from the bridge (toolLabel/toolDetail/…)
  function appendToolCard(entry, id) {
    const readOnly = entry.readOnly === true;
    let card;
    if (readOnly) {
      card = el("div", "query-strip");
      card.dataset.toolId = id;
      card.appendChild(el("span", "q-prefix")).textContent = "查看";
      text(card.appendChild(el("span", "q-label")), entry.toolLabel || entry.toolName);
      const stateSpan = el("span", "q-state");
      stateSpan.textContent = "";
      card.appendChild(stateSpan);
    } else {
      card = el("div", "tool-card");
      card.dataset.toolId = id;
      const row = card.appendChild(el("div", "row1"));
      const glyph = text(el("span", "glyph"), "·");
      row.appendChild(glyph);
      const label = text(el("span", "label"), entry.toolLabel || entry.toolName);
      row.appendChild(label);
      row.appendChild(spinner());
      if (entry.toolDetail) {
        text(card.appendChild(el("div", "detail")), entry.toolDetail);
      }
      const details = card.appendChild(el("details"));
      text(details.appendChild(el("summary")), "技术详情");
      const pre = details.appendChild(el("pre"));
      pre.textContent = (entry.toolName || "") +
        (entry.toolInput && Object.keys(entry.toolInput).length
          ? "\n" + JSON.stringify(entry.toolInput) : "");
    }
    chat.appendChild(card);
  }

  function updateToolCard(id, ok, resultText, summaryText) {
    const card = chat.querySelector('[data-tool-id="' + CSS.escape(id) + '"]');
    if (!card) return;
    if (card.classList.contains("query-strip")) {
      card.querySelector(".spinner")?.remove();
      const q = card.querySelector(".q-state");
      if (q) {
        q.className = "q-state " + (ok ? "q-state-ok" : "q-state-err");
        q.textContent = ok ? "✓" : "✗";
      }
      return;
    }
    card.classList.add(ok ? "ok" : "failed");
    card.querySelector(".spinner")?.remove();
    const glyph = card.querySelector(".glyph");
    if (glyph) {
      glyph.textContent = ok ? "✓" : "✗";
      glyph.style.color = ok ? "var(--accent)" : "var(--err)";
    }
    if (resultText) {
      const r = text(el("div", "result"), resultText);
      card.insertBefore(r, card.querySelector("details"));
    }
    const pre = card.querySelector("details pre");
    if (pre && summaryText) {
      pre.textContent += "\n→ " + summaryText;
    }
  }

  function appendPermissionCard(entry) {
    const card = el("div", "perm-card");
    card.dataset.permId = entry.callId;
    const row = card.appendChild(el("div", "row1"));
    text(row.appendChild(el("span", "glyph")), "⚠").style.color = "var(--warn)";
    text(row.appendChild(el("span", "title")),
      "AI 请求：" + (entry.toolLabel || entry.toolName));
    if (entry.riskText) {
      text(card.appendChild(el("div", "risk")), entry.riskText);
    }
    const buttons = card.appendChild(el("div", "buttons"));
    const deny = buttons.appendChild(el("button", "perm-btn secondary"));
    deny.textContent = "拒绝";
    const allow = buttons.appendChild(el("button", "perm-btn primary"));
    allow.textContent = "允许";
    deny.addEventListener("click", function () {
      bridge.answerPermission(entry.callId, false);
    });
    allow.addEventListener("click", function () {
      bridge.answerPermission(entry.callId, true);
    });
    chat.appendChild(card);
  }

  function resolvePermissionCard(callId, granted) {
    const card = chat.querySelector('[data-perm-id="' + callId + '"]');
    if (!card) return;
    card.className = "perm-card resolved";
    card.innerHTML = "";
    text(card.appendChild(el("span", "glyph")), granted ? "✓" : "✕")
      .style.color = granted ? "var(--accent)" : "var(--text-tertiary)";
    text(card.appendChild(el("span", "resolved-label")),
      (granted ? "已允许" : "已拒绝"));
  }

  function appendError(t) {
    const card = el("div", "error-card");
    text(card, t);
    chat.appendChild(card);
  }

  function renderHistory(messages) {
    chat.querySelectorAll(
      ".msg-user, .msg-assistant, .tool-card, .query-strip, .perm-card, .error-card")
      .forEach(function (n) { n.remove(); });
    (messages || []).forEach(function (m) {
      switch (m.kind) {
        case "user": appendUser(m.text); break;
        case "assistant": appendAssistant(m.text); break;
        case "error": appendError(m.text); break;
        case "tool": {
          appendToolCard(m, m.id);
          if (m.toolOk !== undefined) {
            updateToolCard(m.id, m.toolOk === true,
              m.toolResult || m.toolSummary || "", m.toolSummary || "");
          }
          break;
        }
        case "permission": {
          appendPermissionCard(m);
          if (!m.pending) resolvePermissionCard(m.callId, m.granted === true);
          break;
        }
      }
    });
  }

  // ── bridge wiring ────────────────────────────────────────────────────────

  function setState(s) {
    state = Object.assign(state, s || {});
    busyBar.classList.toggle("hidden", !state.busy);
    updateNotices();
    updateInputState();
    updateEmptyState();
  }

  function connect(bridgeObj) {
    bridge = bridgeObj;
    bridge.historyReplayed.connect(function (messagesJson) {
      renderHistory(JSON.parse(messagesJson));
      updateEmptyState();
      scrollBottom();
    });
    bridge.stateChanged.connect(function (stateJson) {
      setState(JSON.parse(stateJson));
    });
    bridge.assistantTextAppended.connect(function (t) {
      appendAssistant(t);
      updateEmptyState();
      scrollBottom();
    });
    bridge.toolStarted.connect(function (id, entryJson) {
      appendToolCard(JSON.parse(entryJson), id);
      updateEmptyState();
      scrollBottom();
    });
    bridge.toolFinished.connect(function (id, ok, resultText, summaryText) {
      updateToolCard(id, ok, resultText, summaryText);
      scrollBottom();
    });
    bridge.permissionRequested.connect(function (callId, entryJson) {
      appendPermissionCard(JSON.parse(entryJson));
      updateEmptyState();
      scrollBottom();
    });
    bridge.permissionResolved.connect(function (callId, granted) {
      resolvePermissionCard(callId, granted);
    });
    bridge.historyCleared.connect(function () {
      renderHistory([]);
      updateEmptyState();
    });
    bridge.errorOccurred.connect(function (t) {
      appendError(t);
      scrollBottom();
    });
    bridge.pageReady();
  }

  if (typeof qt !== "undefined" && qt.webChannelTransport) {
    new QWebChannel(qt.webChannelTransport, function (channel) {
      if (channel.objects.bridge) connect(channel.objects.bridge);
      else setState({ enabled: true, installed: true });
    });
  } else {
    // Standalone browser preview (no bridge): still show the empty state.
    setState({ enabled: true, installed: true });
    updateEmptyState();
  }

  // ── input handling ───────────────────────────────────────────────────────

  function send() {
    const t = input.value.trim();
    if (!t || !bridge || input.disabled) return;
    appendUser(t);
    updateEmptyState();
    scrollBottom();
    input.value = "";
    updateInputState();
    bridge.sendMessage(t);
  }

  btnSend.addEventListener("click", send);
  input.addEventListener("keydown", function (ev) {
    if (ev.key === "Enter") { ev.preventDefault(); send(); }
    else if (ev.key === "Escape" && bridge && state.busy) bridge.cancelTurn();
  });
  input.addEventListener("input", updateInputState);
  btnClear.addEventListener("click", function () {
    if (bridge) bridge.clearHistory();
    else { renderHistory([]); updateEmptyState(); }
  });
  btnClose.addEventListener("click", function () {
    if (bridge) bridge.requestClose();
  });
  document.querySelectorAll(".chip").forEach(function (chip) {
    chip.addEventListener("click", function () {
      input.value = chip.textContent;
      send();
    });
  });
})();
