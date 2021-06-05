/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "BaseWebView.hpp"

#include <iostream>

#define JS_CONSOLE_REDIRECT     "window.console = {log: (s) => window.webkit.messageHandlers.console_log.postMessage([s])};"
#define JS_DISABLE_CONTEXT_MENU "window.oncontextmenu = (e) => e.preventDefault();"
#define CSS_DISABLE_PINCH_ZOOM  "body { touch-action: pan-x pan-y; }"
#define CSS_DISABLE_SELECTION   "body { user-select: none; -webkit-user-select: none; }"

void BaseWebView::redirectConsole()
{
    // Script runs before any user script starts running
    injectScript(String(JS_CONSOLE_REDIRECT));
}

void BaseWebView::loadFinished()
{
    // User scripts may have started running already
    runScript(String(JS_DISABLE_CONTEXT_MENU));
    addStylesheet(String(CSS_DISABLE_PINCH_ZOOM));
    addStylesheet(String(CSS_DISABLE_SELECTION));
}

void BaseWebView::handleWebViewScriptMessage(String name, ScriptValue arg1, ScriptValue arg2)
{
    if (name == "console_log") {
        std::cerr << static_cast<const char*>(arg1) << std::endl;
    } else {
        fHandler.handleWebViewScriptMessage(name, arg1, arg2);
    }
}

void BaseWebView::addStylesheet(String source)
{
    String js;
    js += "document.head.insertAdjacentHTML('beforeend', '<style>" + source + "</style>');";
    runScript(js);
}