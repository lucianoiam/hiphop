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

#include "AbstractWebView.hpp"

#include <iostream>
#include <sstream>

#define JS_DISABLE_CONTEXT_MENU  "window.oncontextmenu = (e) => e.preventDefault();"
#define JS_DISABLE_PRINT         "window.onkeydown = (e) => { if ((e.key == 'p') && (e.ctrlKey || e.metaKey)) e.preventDefault(); };"
#define JS_CREATE_CONSOLE        "window.console = {" \
                                 "   log  : (s) => window.webviewHost.postMessage(['console', 'log'  , String(s)])," \
                                 "   info : (s) => window.webviewHost.postMessage(['console', 'info' , String(s)])," \
                                 "   warn : (s) => window.webviewHost.postMessage(['console', 'warn' , String(s)])," \
                                 "   error: (s) => window.webviewHost.postMessage(['console', 'error', String(s)])" \
                                 "};"
#define JS_CREATE_HOST_OBJECT    "window.webviewHost = new EventTarget;" \
                                 "window.webviewHost.addMessageListener = (lr) => {" \
                                    "window.webviewHost.addEventListener('message', (ev) => lr(ev.detail))" \
                                 "};"

#define CSS_DISABLE_IMAGE_DRAG   "img { user-drag: none; -webkit-user-drag: none; }"
#define CSS_DISABLE_SELECTION    "body { user-select: none; -webkit-user-select: none; }"
#define CSS_DISABLE_PINCH_ZOOM   "body { touch-action: pan-x pan-y; }"
#define CSS_DISABLE_OVERFLOW     "body { overflow: hidden; }"

/**
   Keep this class generic; specific plugin features belong to WebHostUI.
 */

USE_NAMESPACE_DISTRHO

AbstractWebView::AbstractWebView()
        : fWidth(0)
        , fHeight(0)
        , fBackgroundColor(0)
        , fParent(0)
        , fKeyboardFocus(false)
        , fPrintTraffic(false)
        , fHandler(nullptr)
{}

uint AbstractWebView::getWidth()
{
    return fWidth;
}

uint AbstractWebView::getHeight()
{
    return fHeight;
}

void AbstractWebView::setSize(uint width, uint height)
{
    fWidth = width;
    fHeight = height;
    onSize(width, height);
}

uint32_t AbstractWebView::getBackgroundColor()
{
    return fBackgroundColor;
}

void AbstractWebView::setBackgroundColor(uint32_t rgba)
{
    fBackgroundColor = rgba;
}

uintptr_t AbstractWebView::getParent()
{
    return fParent;
}

void AbstractWebView::setParent(uintptr_t parent)
{
    fParent = parent;
}

bool AbstractWebView::getKeyboardFocus()
{
    return fKeyboardFocus;
}    

void AbstractWebView::setKeyboardFocus(bool focus)
{
    fKeyboardFocus = focus;
    onKeyboardFocus(focus);
}

void AbstractWebView::setPrintTraffic(bool printTraffic)
{
    fPrintTraffic = printTraffic;
}

void AbstractWebView::setEventHandler(WebViewEventHandler* handler)
{
    fHandler = handler;
}

void AbstractWebView::postMessage(const JsValueVector& args)
{
    // This method implements something like a "reverse postMessage()" aiming to keep the bridge
    // symmetrical. Global window.webviewHost is an EventTarget that can be listened for messages.
    String payload = serializeJsValues(args);

    if (fPrintTraffic) {
        std::cerr << "cpp -> js : " << payload.buffer() << std::endl << std::flush;
    }
    
    String js = "window.webviewHost.dispatchEvent(new CustomEvent('message',{detail:" + payload + "}));";
    runScript(js);
}

void AbstractWebView::injectDefaultScripts()
{
    String js = String(JS_DISABLE_CONTEXT_MENU)
              + String(JS_DISABLE_PRINT)
              + String(JS_CREATE_CONSOLE)
              + String(JS_CREATE_HOST_OBJECT)
    ;
    injectScript(js);
}

void AbstractWebView::handleLoadFinished()
{
    String css = String(CSS_DISABLE_IMAGE_DRAG)
               + String(CSS_DISABLE_SELECTION)
               + String(CSS_DISABLE_PINCH_ZOOM)
               + String(CSS_DISABLE_OVERFLOW)
    ;
    addStylesheet(css);

    if (fHandler != nullptr) {
        fHandler->handleWebViewLoadFinished();
    }
}

void AbstractWebView::handleScriptMessage(const JsValueVector& args)
{
    if ((args.size() == 3) && (args[0].getString() == "console")) {
        if (fHandler != nullptr) {
            fHandler->handleWebViewConsole(args[1].getString(), args[2].getString());
        }
    } else {
        if (fPrintTraffic) {
            std::cerr << "cpp <- js : " << serializeJsValues(args).buffer()
                << std::endl << std::flush;
        }
        
        if (fHandler != nullptr) {
            fHandler->handleWebViewScriptMessage(args);
        }
    }
}

String AbstractWebView::serializeJsValues(const JsValueVector& args)
{
    std::stringstream ss;
    ss << '[';

    for (JsValueVector::const_iterator it = args.cbegin(); it != args.cend(); ++it) {
        if (it != args.cbegin()) {
            ss << ',';
        }
        ss << *it;
    }

    ss << ']';

    return String(ss.str().c_str());
}

void AbstractWebView::addStylesheet(String& source)
{
    String js = "document.head.insertAdjacentHTML('beforeend', '<style>" + source + "</style>');";
    runScript(js);
}
