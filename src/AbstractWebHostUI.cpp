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

#include "AbstractWebHostUI.hpp"

#include "Path.hpp"
#include "macro.h"

USE_NAMESPACE_DISTRHO

AbstractWebHostUI::AbstractWebHostUI(uint baseWidth, uint baseHeight, uint32_t backgroundColor)
    : UI(baseWidth, baseHeight)
    , fFlushedInitMsgQueue(false)
    , fBackgroundColor(backgroundColor)
    , fRunUiBlock(false)
{}

void AbstractWebHostUI::initWebView(AbstractWebView& webView)
{
    // Web views adjust their contents following the system display scale factor,
    // adjust window size so it correctly wraps content on high density displays.
    uintptr_t parent = isStandalone() ? createStandaloneWindow() : getParentWindowHandle();

    if (parent == 0) {
        // This can happen during initialization when running as a plugin,
        // constructor/destructor can be repeatedly called for a few times.
        return;
    }

    float k = getDisplayScaleFactor(parent);
    fInitWidth = k * getWidth();
    fInitHeight = k * getHeight();

    webView.setSize(fInitWidth, fInitHeight);
    webView.setBackgroundColor(fBackgroundColor);
    webView.setParent(parent);
    webView.realize();

    webView.setEventHandler(this);
#ifdef HIPHOP_PRINT_TRAFFIC
    webView.setPrintTraffic(true);
#endif

    String js = String(
#include "ui/distrho-ui.js.inc"
    );
    js += "const DISTRHO = Object.freeze({ UI: UI });" \
          "UI = null;";
    webView.injectScript(js);

    String url = "file://" + path::getLibraryPath() + "/ui/index.html";
    webView.navigate(url);

    setSize(fInitWidth, fInitHeight);
}

void AbstractWebHostUI::sizeChanged(uint width, uint height)
{
    UI::sizeChanged(width, height);
    
    getWebView().setSize(width, height);
    webPostMessage({"UI", "sizeChanged", width, height});
}

void AbstractWebHostUI::parameterChanged(uint32_t index, float value)
{
    webPostMessage({"UI", "parameterChanged", index, value});
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS

void AbstractWebHostUI::programLoaded(uint32_t index)
{
    webPostMessage({"UI", "programLoaded", index});
}

#endif // DISTRHO_PLUGIN_WANT_PROGRAMS

#if DISTRHO_PLUGIN_WANT_STATE

void AbstractWebHostUI::stateChanged(const char* key, const char* value)
{
    webPostMessage({"UI", "stateChanged", key, value});
}

#endif // DISTRHO_PLUGIN_WANT_STATE

void AbstractWebHostUI::uiIdle()
{
    if (fRunUiBlock) {
        fRunUiBlock = false;
        fQueuedUiBlock();
    }

    if (isStandalone()) {
        processStandaloneEvents();
    }
}

void AbstractWebHostUI::webPostMessage(const JsValueVector& args) {
    if (fFlushedInitMsgQueue) {
        getWebView().postMessage(args);
    } else {
        fInitMsgQueue.push_back(args);
    }
}

void AbstractWebHostUI::flushInitMessageQueue()
{
    if (fFlushedInitMsgQueue) {
        return;
    }

    fFlushedInitMsgQueue = true;

    for (InitMessageQueue::iterator it = fInitMsgQueue.begin(); it != fInitMsgQueue.end(); ++it) {
        getWebView().postMessage(*it);
    }
    
    fInitMsgQueue.clear();
}

void AbstractWebHostUI::setKeyboardFocus(bool focus)
{
    getWebView().setKeyboardFocus(focus);
}

void AbstractWebHostUI::queue(const UiBlock& block)
{
    fQueuedUiBlock = block;
    fRunUiBlock = true;
}

void AbstractWebHostUI::handleWebViewContentLoadFinished()
{
    // Trigger the JavaScript sizeChanged() callback, useful for LXRESIZEBUG.
    sizeChanged(fInitWidth, fInitHeight);
    onWebContentReady();
}

#define kArg0 2
#define kArg1 3
#define kArg2 4

void AbstractWebHostUI::handleWebViewScriptMessageReceived(const JsValueVector& args)
{
    if (args[0].getString() != "UI") {
        onWebMessageReceived(args); // passthrough
        return;
    }

    // It is not possible to implement JS synchronous calls that return values
    // without resorting to dirty hacks. Use JS async functions instead, and
    // fulfill their promises here. See for example isResizable() below.

    String method = args[1].getString();
    int argc = args.size() - kArg0;

    if (method == "getWidth") {
        webPostMessage({"UI", "getWidth", static_cast<double>(getWidth())});

    } else if (method == "getHeight") {
        webPostMessage({"UI", "getHeight", static_cast<double>(getHeight())});

    } else if ((method == "setWidth") && (argc == 1)) {
        queue([this, args]() {
            setWidth(static_cast<uint>(args[kArg0].getDouble()));
        });

    } else if ((method == "setHeight") && (argc == 1)) {
        queue([this, args]() {
            setHeight(static_cast<uint>(args[kArg0].getDouble()));
        });

    } else if (method == "isResizable") {
        webPostMessage({"UI", "isResizable", isResizable()});

    } else if ((method == "setSize") && (argc == 2)) {
        // Queuing is needed for REAPER on Linux and does no harm on others
        queue([this, args]() {
            setSize(
                static_cast<uint>(args[kArg0].getDouble()), // width
                static_cast<uint>(args[kArg1].getDouble())  // height
            );
        });

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    } else if ((method == "sendNote") && (argc == 3)) {
        sendNote(
            static_cast<uint8_t>(args[kArg0].getDouble()),  // channel
            static_cast<uint8_t>(args[kArg1].getDouble()),  // note
            static_cast<uint8_t>(args[kArg2].getDouble())   // velocity
        );
#endif

    } else if ((method == "editParameter") && (argc == 2)) {
        editParameter(
            static_cast<uint32_t>(args[kArg0].getDouble()), // index
            static_cast<bool>(args[kArg1].getBool())        // started
        );

    } else if ((method == "setParameterValue") && (argc == 2)) {
        setParameterValue(
            static_cast<uint32_t>(args[kArg0].getDouble()), // index
            static_cast<float>(args[kArg1].getDouble())     // value
        );

#if DISTRHO_PLUGIN_WANT_STATE
    } else if ((method == "setState") && (argc == 2)) {
        setState(
            args[kArg0].getString(), // key
            args[kArg1].getString()  // value
        );
#endif // DISTRHO_PLUGIN_WANT_STATE

    } else if (method == "isStandalone") {
        webPostMessage({"UI", "isStandalone", isStandalone()});

    } else if ((method == "setKeyboardFocus") && (argc == 1)) {
        setKeyboardFocus(static_cast<bool>(args[kArg0].getBool()));

    } else if ((method == "openSystemWebBrowser") && (argc == 1)) {
        String url = args[kArg0].getString();
        openSystemWebBrowser(url);

    } else if (method == "getInitWidth") {
        webPostMessage({"UI", "getInitWidth", static_cast<double>(getInitWidth())});

    } else if (method == "getInitHeight") {
        webPostMessage({"UI", "getInitHeight", static_cast<double>(getInitHeight())});

    } else if (method == "flushInitMessageQueue") {
        flushInitMessageQueue();

    } else {
        HIPHOP_LOG_STDERR_COLOR("Invalid call to WebHostUI method");
    }
}