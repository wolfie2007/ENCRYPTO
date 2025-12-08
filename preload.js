const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    browseFile: () => ipcRenderer.invoke('browse-file'),
    encryptFile: (input, output, pin) => ipcRenderer.invoke('encrypt-file', input, output, pin),
    decryptFile: (input, output, pin) => ipcRenderer.invoke('decrypt-file', input, output, pin)
});
