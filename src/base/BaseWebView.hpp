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

#ifndef BASEWEBVIEW_HPP
#define BASEWEBVIEW_HPP

#include <cstdint>
#include <vector>

#include "dgl/Geometry.hpp"
#include "extra/String.hpp"

#include "ScriptValue.hpp"

START_NAMESPACE_DISTRHO

typedef std::vector<ScriptValue> ScriptValueVector;

class WebViewEventHandler
{
public:
    virtual void webViewLoadFinished() = 0;
    virtual bool webViewScriptMessageReceived(const ScriptValueVector& args) = 0;

};

class BaseWebView
{
public:
    BaseWebView(WebViewEventHandler& handler) : fHandler(handler) {}
    virtual ~BaseWebView() {};

    virtual void reparent(uintptr_t windowId) = 0;
    virtual void resize(const DGL::Size<uint>& size) = 0;
    virtual void navigate(String url) = 0;
    virtual void runScript(String source) = 0;
    virtual void injectScript(String source) = 0;

    void sendHostMessage(const ScriptValueVector& args);

protected:
    void injectDefaultScripts(String platformSpecificScript);
    
    void handleLoadFinished();
    void handleScriptMessage(ScriptValueVector& args);

private:
    void addStylesheet(String source);

    WebViewEventHandler& fHandler;

};

END_NAMESPACE_DISTRHO

#endif // BASEWEBVIEW_HPP
