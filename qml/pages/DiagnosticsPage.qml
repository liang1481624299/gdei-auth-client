import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import "../theme"
import "../components"

// 诊断页（规范 6.6 / FR-8）—— 工具集卡片网格。
// 后端：网卡识别(app.network)、会话连通(app.auth)、日志导出(app.logs)、
// HTTP 短探测 / DoH 对比 / 编码嗅探(app.diagnostics)。
// 无后端：DoT / 路由追踪 / Windows 卸载修复 / LibreSpeed → 置灰 + 此平台暂不支持，不伪造数据。
PageScaffold {
    id: root
    Theme { id: th }

    title: "diagnostics.title"
    subtitle: "diagnostics.subtitle"
    showBack: true
    contentMaxWidth: th.dp(960)

    function localized(key) { return app.i18n.localize(key); }

    // 异步结果捕获（网卡 / 会话探测结果通过属性变化回传）。
    property var pendingNic: null
    property var pendingConn: null
    // 后端诊断 Controller 异步回填的卡片引用。
    property var pendingHttp: null
    property var pendingDoh: null
    property var pendingEnc: null

    function begin(card) {
        card.running = true;
        card.statusText = root.localized("diagnostics.running");
        card.statusTint = th.warning;
        card.resultText = "";
    }
    // ok: true 绿 / false 红 / null 橙
    function done(card, ok, statusKey, detail, titleKey) {
        card.running = false;
        card.statusText = root.localized(statusKey);
        card.statusTint = ok === true ? th.success : (ok === false ? th.danger : th.warning);
        card.resultText = detail;
        try {
            app.reportDiagnostic(ok === true ? 1 : (ok === false ? 3 : 2), titleKey, detail);
        } catch (e) {}
    }

    // ── 会话 / 连通探测（后端 app.auth）──
    function doConn(card) {
        root.pendingConn = card;
        root.begin(card);
        connGuard.restart();
        app.auth.refreshStatus();
    }
    function finishConn() {
        var card = root.pendingConn;
        if (!card) return;
        root.pendingConn = null; connGuard.stop();
        var state = app.auth.state;
        var online = app.auth.online;
        var msg = app.auth.lastMessage ? root.localized(app.auth.lastMessage) : "";
        var txt = "";
        txt += root.localized("diagnostics.connState") + ": "
               + (online ? root.localized("layer.probeOnline") : root.localized("layer.probeOffline")) + "\n";
        txt += root.localized("diagnostics.connAuthState") + ": " + state + "\n";
        if (msg.length) txt += root.localized("diagnostics.connMessage") + ": " + msg + "\n";
        root.done(card, online, online ? "diagnostics.connOk" : "diagnostics.connFail", txt, "diagnostics.connTitle");
    }

    // ── 网卡信息识别（后端 app.network）──
    function doNic(card) {
        root.pendingNic = card;
        root.begin(card);
        nicGuard.restart();
        app.network.refresh();
    }
    function finishNic() {
        var card = root.pendingNic;
        if (!card) return;
        root.pendingNic = null; nicGuard.stop();
        var txt = "";
        txt += root.localized("network.interfaceName") + ": " + (app.network.interfaceName || "-") + "\n";
        txt += root.localized("network.localMac") + ": " + (app.network.localMac || "-") + "\n";
        txt += root.localized("network.sourceIp") + ": " + (app.network.sourceIp || "-") + "\n";
        var ifs = app.network.interfaces;
        if (ifs && ifs.length > 0) {
            txt += "\n" + root.localized("diagnostics.nicList") + ":\n";
            for (var i = 0; i < ifs.length; i++) {
                var it = ifs[i];
                txt += "  - " + (it.name || "-")
                       + ((it.mac && String(it.mac).length) ? ("  [" + it.mac + "]") : "")
                       + ((it.ip && String(it.ip).length) ? ("  " + it.ip) : "") + "\n";
            }
        }
        var name = app.network.interfaceName || "";
        var isAx201 = /ax201/i.test(name);
        if (isAx201) txt += "\n" + root.localized("diagnostics.ax201Hint");
        root.done(card, !isAx201, isAx201 ? "diagnostics.ax201Found" : "diagnostics.nicOk", txt, "diagnostics.nicTitle");
    }

    // ── HTTP 短探测（后端 DiagnosticsController）──
    function doHttp(card) {
        root.begin(card);
        root.pendingHttp = card;
        app.diagnostics.probeHttp();
    }

    // ── DNS / DoH 对比（后端并行探测，公共 DoH 端点比延迟）──
    function doDoh(card) {
        root.begin(card);
        root.pendingDoh = card;
        app.diagnostics.probeDoh();
    }

    // ── 编码探测（后端嗅探门户页 charset；UI 固定 UTF-8，不受发包编码影响）──
    function doEncoding(card) {
        root.begin(card);
        root.pendingEnc = card;
        app.diagnostics.sniffCharset();
    }

    // ── 日志导出（后端 app.logs）──
    function doLogExport(card) {
        root.begin(card);
        var path = "";
        try { path = app.logs.exportLog(""); } catch (e) {}
        if (path && path.length > 0)
            root.done(card, true, "diagnostics.logDone",
                      root.localized("diagnostics.logDoneDetail").arg(path), "diagnostics.logTitle");
        else
            root.done(card, false, "diagnostics.logFail",
                      root.localized("diagnostics.logFail"), "diagnostics.logTitle");
    }

    Connections {
        target: app.network
        function onInterfacesChanged() { if (root.pendingNic) root.finishNic(); }
        function onSourceIpChanged() { if (root.pendingNic) root.finishNic(); }
        function onAvailableChanged() { if (root.pendingNic) root.finishNic(); }
    }
    Timer { id: nicGuard; interval: 1500; onTriggered: { if (root.pendingNic) root.finishNic(); } }

    Connections {
        target: app.auth
        function onStateChanged() { if (root.pendingConn) root.finishConn(); }
        function onOnlineChanged() { if (root.pendingConn) root.finishConn(); }
        function onLastMessageChanged() { if (root.pendingConn) root.finishConn(); }
    }
    Timer { id: connGuard; interval: 3000; onTriggered: { if (root.pendingConn) root.finishConn(); } }

    Connections {
        target: app.diagnostics
        function onHttpProbeFinished(ok, status, ms, url, error) {
            var card = root.pendingHttp; if (!card) return; root.pendingHttp = null;
            if (ok) {
                root.done(card, true, "diagnostics.httpOk",
                    root.localized("diagnostics.httpDetail").arg(url).arg(ms), "diagnostics.httpTitle");
            } else if (error === "timeout") {
                root.done(card, false, "diagnostics.timeout",
                    root.localized("diagnostics.timeoutDetail").arg(url), "diagnostics.httpTitle");
            } else {
                root.done(card, false, "diagnostics.netError",
                    root.localized("diagnostics.netErrorDetail").arg(url), "diagnostics.httpTitle");
            }
        }
        function onDohProbeFinished(results) {
            var card = root.pendingDoh; if (!card) return; root.pendingDoh = null;
            var allOk = true, anyOk = false, txt = "";
            for (var i = 0; i < results.length; i++) {
                var r = results[i];
                txt += r.name + ": " + (r.ok ? (r.ms + " ms") : root.localized("diagnostics.fail"))
                       + (r.extra ? ("  (" + r.extra + ")") : "") + "\n";
                if (r.ok) anyOk = true; else allOk = false;
            }
            txt += "\n" + root.localized("diagnostics.dohNote");
            root.done(card, allOk, allOk ? "diagnostics.dohOk" : (anyOk ? "diagnostics.dohPartial" : "diagnostics.fail"),
                txt, "diagnostics.dohTitle");
        }
        function onEncodingFinished(ok, charset, url, error) {
            var card = root.pendingEnc; if (!card) return; root.pendingEnc = null;
            var detected = charset || "";
            var txt = root.localized("diagnostics.encTarget") + ": " + url + "\n"
                + root.localized("diagnostics.encDetected") + ": "
                + (detected.length ? detected : root.localized("diagnostics.encNone")) + "\n"
                + root.localized("diagnostics.encUiFixed");
            if (detected.length > 0) {
                root.done(card, true, "diagnostics.encOk", txt, "diagnostics.encTitle");
            } else if (error === "timeout") {
                root.done(card, false, "diagnostics.timeout",
                    root.localized("diagnostics.timeoutDetail").arg(url), "diagnostics.encTitle");
            } else if (error === "network") {
                root.done(card, false, "diagnostics.netError",
                    root.localized("diagnostics.netErrorDetail").arg(url), "diagnostics.encTitle");
            } else {
                root.done(card, false, "diagnostics.encNone", txt, "diagnostics.encTitle");
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: width < th.compactBreakpoint ? 1 : 2
        columnSpacing: th.sp8
        rowSpacing: th.sp8

        DiagnosticToolCard {
            id: connCard
            toolTitle: root.localized("diagnostics.connTitle")
            toolDescription: root.localized("diagnostics.connDesc")
            onRunRequested: root.doConn(connCard)
        }
        DiagnosticToolCard {
            id: httpCard
            toolTitle: root.localized("diagnostics.httpTitle")
            toolDescription: root.localized("diagnostics.httpDesc")
            onRunRequested: root.doHttp(httpCard)
        }
        DiagnosticToolCard {
            id: dohCard
            toolTitle: root.localized("diagnostics.dohTitle")
            toolDescription: root.localized("diagnostics.dohDesc")
            onRunRequested: root.doDoh(dohCard)
        }
        DiagnosticToolCard {
            id: nicCard
            toolTitle: root.localized("diagnostics.nicTitle")
            toolDescription: root.localized("diagnostics.nicDesc")
            onRunRequested: root.doNic(nicCard)
        }
        DiagnosticToolCard {
            id: encCard
            toolTitle: root.localized("diagnostics.encTitle")
            toolDescription: root.localized("diagnostics.encDesc")
            onRunRequested: root.doEncoding(encCard)
        }
        DiagnosticToolCard {
            id: logCard
            toolTitle: root.localized("diagnostics.logTitle")
            toolDescription: root.localized("diagnostics.logDesc")
            onRunRequested: root.doLogExport(logCard)
        }
        DiagnosticToolCard {
            toolTitle: root.localized("diagnostics.dotTitle")
            toolDescription: root.localized("diagnostics.dotDesc")
            supported: false
        }
        DiagnosticToolCard {
            toolTitle: root.localized("diagnostics.traceTitle")
            toolDescription: root.localized("diagnostics.traceDesc")
            supported: false
        }
        DiagnosticToolCard {
            toolTitle: root.localized("diagnostics.offloadTitle")
            toolDescription: root.localized("diagnostics.offloadDesc")
            supported: false
        }
        DiagnosticToolCard {
            toolTitle: root.localized("diagnostics.speedTitle")
            toolDescription: root.localized("diagnostics.speedDesc")
            supported: false
        }
    }
}
