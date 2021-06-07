R"DPF_JS(
/*
 * dpf-webui
 * Copyright (C) 2021 Luciano Iam <lucianoiam@protonmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

{ // start private namespace

class DPF {

    constructor() {
        // TODO

        this.addHostMessageListener((message) => {
            console.log(message);
        });
    }

    // UI::editParameter(uint32_t index, bool started)
    editParameter(index, started) {
        this._builtinCall('editParameter', index, started);
    }

    /**
      Convenience methods
     */

    // WebViewEventHandler::webViewScriptMessageReceived(const ScriptValueVector& args)
    postScriptMessage(...args) {
        window.webviewHost.postMessage([...args]);
    }

    // BaseWebView::sendHostMessage(const ScriptValueVector& args)
    addHostMessageListener(listener) {
        window.webviewHost.addEventListener('message', (ev) => listener(ev.detail));
    }

    /**
      Private methods
     */

    _builtinCall(method, ...args) {
        this.postScriptMessage('DPF', method, ...args);
    }

}

window.DPF = new DPF;

} // end private namespace

)DPF_JS"
