const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

// ANSI / OSC (shell integration dahil) kaçış dizilerini temizler
const ANSI_RE = /\x1b\][^\x07\x1b]*(?:\x07|\x1b\\)|\x1b\[[0-?]*[ -/]*[@-~]|\x1b[@-Z\\-_]/g;
// Sadece reel sayılar
const NUM_RE = /^[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?$/;

let panel;
let selected;          // izlenen terminal
let buffer = '';       // parçalı gelen çıktı için
let statusItem;

// ">>a:1,b:2.5" gövdesini {a:1, b:2.5} nesnesine çevirir. Sayı olmayan değerler atlanır.
function parseBody(body) {
  const values = {};
  for (const part of body.split(',')) {
    const i = part.lastIndexOf(':');   // key içinde "::" olsa bile çalışsın
    if (i <= 0) continue;
    const key = part.slice(0, i).trim();
    const raw = part.slice(i + 1).trim();
    if (key && NUM_RE.test(raw)) values[key] = Number(raw);
  }
  return values;
}

// Terminalden gelen ham veri parçasını işler.
function feed(chunk) {
  buffer += chunk.replace(ANSI_RE, '').replace(/\r/g, '');
  const re = />>([^;\n]*);/g;
  let m, last = 0;
  while ((m = re.exec(buffer))) {
    last = re.lastIndex;
    const values = parseBody(m[1]);
    if (Object.keys(values).length && panel) {
      panel.webview.postMessage({ type: 'sample', t: Date.now(), values });
    }
  }
  buffer = buffer.slice(last);
  // Henüz ';' gelmemiş yarım satırı koru, gerisini at
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
  if (!vscode.window.onDidStartTerminalShellExecution) {
    vscode.window.showErrorMessage('Bu eklenti VS Code 1.93+ (Shell Integration API) gerektirir.');
    return;
  }
  const terms = vscode.window.terminals;
  if (!terms.length) {
    vscode.window.showInformationMessage('Açık terminal yok. Önce bir terminal aç.');
    return;
  }
  const pick = await vscode.window.showQuickPick(
    terms.map(t => ({
      label: t.name,
      description: t.shellIntegration ? 'shell integration ✓' : 'shell integration henüz aktif değil',
      terminal: t
    })),
    { placeHolder: 'İzlenecek terminali seç' }
  );
  if (!pick) return;

  selected = pick.terminal;
  buffer = '';
  ensurePanel(context);
  statusItem.text = `$(graph) Plotter: ${selected.name}`;
  statusItem.show();
  vscode.window.showInformationMessage(
    `"${selected.name}" izleniyor. Programı bu noktadan SONRA çalıştır (çalışan komuta bağlanılamaz).`
  );
}

function stop() {
  selected = undefined;
  buffer = '';
  statusItem.hide();
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
    vscode.window.onDidCloseTerminal(t => { if (t === selected) stop(); })
  );

  if (vscode.window.onDidStartTerminalShellExecution) {
    context.subscriptions.push(
      vscode.window.onDidStartTerminalShellExecution(async e => {
        if (e.terminal !== selected) return;
        buffer = '';
        try {
          for await (const data of e.execution.read()) feed(data);
        } catch (err) {
          console.error('Terminal Plotter read error', err);
        }
      })
    );
  }
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