const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

const ANSI_RE = /\x1b\][^\x07\x1b]*(?:\x07|\x1b\\)|\x1b\[[0-?]*[ -/]*[@-~]|\x1b[@-Z\\-_]/g;
const NUM_RE = /^[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?$/;

let panel;
let selected;         
let buffer = '';      
let statusItem;

function parseBody(body) {
  const values = {};
  for (const part of body.split(',')) {
    const i = part.lastIndexOf(':');
    if (i <= 0) continue;
    const key = part.slice(0, i).trim();
    const raw = part.slice(i + 1).trim();
    if (key && NUM_RE.test(raw)) values[key] = Number(raw);
  }
  return values;
}

function feed(chunk) {
  console.log("[AGPLOT] [Plotter DEBUG] Hammadde Gelen Chunk:", JSON.stringify(chunk));

  buffer += chunk.replace(ANSI_RE, '').replace(/\r/g, '');
  
  console.log("[AGPLOT] [Plotter DEBUG] Temizlenen Buffer:", JSON.stringify(buffer));

  const re = />>([^;\n]*);/g;
  let m, last = 0;
  while ((m = re.exec(buffer))) {
    last = re.lastIndex;
    const values = parseBody(m[1]);
    console.log("[AGPLOT] [Plotter DEBUG] Parse Edilen Değerler:", values);

    if (Object.keys(values).length && panel) {
      panel.webview.postMessage({ type: 'sample', t: Date.now(), values });
      console.log("[AGPLOT] [Plotter DEBUG] Webview'e veri gönderildi!");
    }
  }
  buffer = buffer.slice(last);
  const i = buffer.lastIndexOf('>>');
  buffer = i >= 0 ? buffer.slice(i) : buffer.slice(-1);
  if (buffer.length > 4096) buffer = '';
}

function ensurePanel(context) {
  if (panel) { panel.reveal(vscode.ViewColumn.Beside, true); return; }
  panel = vscode.window.createWebviewPanel(
    'terminalPlotter', 'Terminal Plotter',
    { viewColumn: vscode.ViewColumn.Beside, preserveFocus: true },
    { enableScripts: true, retainContextWhenHidden: true }
  );
  panel.webview.html = getHtml();
  panel.onDidDispose(() => { panel = undefined; }, null, context.subscriptions);
}

async function start(context) {
  const terms = vscode.window.terminals;
  if (!terms.length) {
    vscode.window.showInformationMessage('Açık terminal yok. Önce bir terminal aç.');
    return;
  }
  
  const pick = await vscode.window.showQuickPick(
    terms.map(t => ({
      label: t.name,
      description: 'Aktif terminal',
      terminal: t
    })),
    { placeHolder: 'İzlenecek terminali seç (VEX PROS dahil)' }
  );
  if (!pick) return;

  selected = pick.terminal;
  buffer = '';
  ensurePanel(context);
  statusItem.text = `$(graph) Plotter: ${selected.name}`;
  statusItem.show();
  console.log("[AGPLOT] [Plotter DEBUG] İzleme başlatıldı, seçilen terminal:", selected.name);
  vscode.window.showInformationMessage(
    `"${selected.name}" izleniyor. Verileri göndermeye başlayabilirsiniz.`
  );
}

function stop() {
  selected = undefined;
  buffer = '';
  statusItem.hide();
  console.log("[AGPLOT] [Plotter DEBUG] İzleme durduruldu.");
}

function activate(context) {
  statusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left);
  statusItem.command = 'terminalPlotter.stop';
  statusItem.tooltip = 'Terminal Plotter: izlemeyi durdurmak için tıkla';

  context.subscriptions.push(
    statusItem,
    vscode.commands.registerCommand('terminalPlotter.start', () => start(context)),
    vscode.commands.registerCommand('terminalPlotter.stop', stop),
    vscode.commands.registerCommand('terminalPlotter.clear', () => panel?.webview.postMessage({ type: 'clear' })),
    vscode.window.onDidCloseTerminal(t => { if (t === selected) stop(); }),
    
    vscode.window.onDidWriteTerminalData(e => {
        console.log(`[AGPLOT] [GENEL TERMINAL VERISI] (${e.terminal.name}):`, JSON.stringify(e.data));
        if (e.terminal === selected) {
            feed(e.data);
        }
    })
  );
  console.log("[AGPLOT] [Plotter DEBUG] Eklenti aktifleşti.");
}

function deactivate() {}

function getNonce() {
  let s = '';
  const c = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  for (let i = 0; i < 32; i++) s += c[Math.floor(Math.random() * c.length)];
  return s;
}

function getHtml() {
  const nonce = getNonce();
  const file = path.join(__dirname, 'src', 'extension.html');
  return fs.readFileSync(file, 'utf8').replace(/\{\{nonce\}\}/g, nonce);
}

module.exports = { activate, deactivate };