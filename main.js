const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const fs = require('fs');
const { execFile } = require('child_process');

// Basic log helper
function log(msg) {
    try {
        const p = app && app.getPath ? app.getPath('userData') : __dirname;
        const logFile = path.join(p, 'run.log');
        const time = new Date().toISOString();
        fs.appendFileSync(logFile, `[${time}] ${msg}\n`);
    } catch (e) {
        // ignore logging failures
    }
}

// Disable GPU acceleration early — fixes many white/blank window issues on some machines
try {
    app.disableHardwareAcceleration();
    // log must run after app is defined
} catch (e) {
    // ignore
}

function createWindow() {
    log('Creating BrowserWindow');
    const win = new BrowserWindow({
        width: 600,
        height: 500,
        show: false,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: false
        }
    });

    // Attach diagnostic listeners
    win.webContents.on('did-finish-load', () => {
        log('Renderer finished load: ' + path.join(__dirname, 'index.html'));
        win.show();
    });

    win.webContents.on('did-fail-load', (event, errorCode, errorDescription, validatedURL) => {
        log(`did-fail-load code=${errorCode} desc=${errorDescription} url=${validatedURL}`);
        // show devtools for diagnosis
        try { win.webContents.openDevTools({ mode: 'detach' }); } catch (e) {}
        win.show();
    });

    win.webContents.on('crashed', () => {
        log('Renderer process crashed');
        try { win.webContents.openDevTools({ mode: 'detach' }); } catch (e) {}
        win.show();
    });

    // Use absolute path based on this script location to avoid cwd issues when launching from USB
    const indexPath = path.join(__dirname, 'index.html');
    log('Loading file: ' + indexPath);
    win.loadFile(indexPath).catch(err => {
        log('loadFile error: ' + (err && err.message ? err.message : String(err)));
        try { win.webContents.openDevTools({ mode: 'detach' }); } catch (e) {}
        win.show();
    });
}

app.whenReady().then(() => {
    log('App ready');
    createWindow();
});

// IPC handlers
ipcMain.handle('browse-file', async () => {
    const { canceled, filePaths } = await dialog.showOpenDialog({ properties: ['openFile'] });
    if (canceled) return null;
    return filePaths[0];
});

ipcMain.handle('encrypt-file', async (event, input, output, pin) => {
    return new Promise(resolve => {
        execFile(path.join(__dirname, 'vault.exe'), ['encrypt', input, output, pin], (err, stdout, stderr) => {
            if (err) {
                resolve(stderr || 'Error running backend');
            } else {
                resolve(stdout.trim());
            }
        });
    });
});

ipcMain.handle('decrypt-file', async (event, input, output, pin) => {
    return new Promise(resolve => {
        execFile(path.join(__dirname, 'vault.exe'), ['decrypt', input, output, pin], (err, stdout, stderr) => {
            if (err) {
                resolve(stderr || 'Error running backend');
            } else {
                resolve(stdout.trim());
            }
        });
    });
});
