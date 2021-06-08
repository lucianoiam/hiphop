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

#ifndef WEBUI_HPP
#define WEBUI_HPP

#include "DistrhoUI.hpp"

#ifdef DISTRHO_OS_LINUX
#include "arch/linux/ExternalGtkWebView.hpp"
#endif
#ifdef DISTRHO_OS_MAC
#include "arch/macos/CocoaWebView.hpp"
#endif
#ifdef DISTRHO_OS_WINDOWS
#include "arch/windows/EdgeWebView.hpp"
#endif

START_NAMESPACE_DISTRHO

class WebUI : public UI, protected WebViewEventHandler
{
public:
    WebUI();
    virtual ~WebUI() {};

protected:

    void onDisplay() override;
    void onResize(const ResizeEvent& ev) override;

    void parameterChanged(uint32_t index, float value) override;

    WEBVIEW_CLASS& webView() { return fWebView; }

    void webViewPostMessage(const ScriptValueVector& args) { fWebView.postMessage(args); }

    // WebViewEventHandler

    virtual void webViewLoadFinished() override;
    virtual bool webViewScriptMessageReceived(const ScriptValueVector& args) override;

private:
    WEBVIEW_CLASS fWebView;
    bool          fDisplayed;

};

END_NAMESPACE_DISTRHO

#endif  // WEBUI_HPP