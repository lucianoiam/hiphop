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

#include "BaseWebView.hpp"

#include <iostream>

#define JS_CONSOLE_OUTPUT       "window.console = {log: (s) => window.webviewHost.postMessage(['console.log', String(s)])};"
#define JS_DISABLE_CONTEXT_MENU "window.oncontextmenu = (e) => e.preventDefault();"
#define CSS_DISABLE_PINCH_ZOOM  "body { touch-action: pan-x pan-y; }"
#define CSS_DISABLE_SELECTION   "body { user-select: none; -webkit-user-select: none; }"

void BaseWebView::injectDefaultScripts()
{
    // Injected scripts run before any user script starts running
    injectScript(String(JS_CONSOLE_OUTPUT));
    injectScript(String(JS_DISABLE_CONTEXT_MENU));
}

void BaseWebView::handleLoadFinished()
{
    // User scripts may have started running already
    addStylesheet(String(CSS_DISABLE_PINCH_ZOOM));
    addStylesheet(String(CSS_DISABLE_SELECTION));
    fHandler.handleWebViewLoadFinished();
}

void BaseWebView::handleScriptMessage(ScriptMessageArguments& args)
{
    if (GET_STRING_ARGUMENT(args) == "console.log") {
        POP_ARGUMENT(args);
        std::cerr << GET_STRING_ARGUMENT(args) << std::endl;
    } else {
        fHandler.handleWebViewScriptMessage(args);
    }
}

void BaseWebView::addStylesheet(String source)
{
    String js;
    js += "document.head.insertAdjacentHTML('beforeend', '<style>" + source + "</style>');";
    runScript(js);
}
