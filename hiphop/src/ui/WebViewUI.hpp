/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
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

#ifndef WEBVIEW_UI_HPP
#define WEBVIEW_UI_HPP

#include <vector>

#include "WebUIBase.hpp"
#include "WebViewBase.hpp"

#if defined(HIPHOP_NETWORK_UI)
# include "NetworkUI.hpp"
#endif

#define ORIGIN_EMBEDDED_WEB_VIEW 0

START_NAMESPACE_DISTRHO

#if defined(HIPHOP_NETWORK_UI)
typedef NetworkUI WebViewUIBase; // https:// + WebSockets messaging
#else
typedef WebUIBase WebViewUIBase; // file://  + Native interface [postMessage()]
#endif

class WebViewUI : public WebViewUIBase, private WebViewEventHandler
{
public:
    WebViewUI(uint widthCssPx, uint heightCssPx, const char* backgroundCssColor,
              float initPixelRatio);
    virtual ~WebViewUI();

    uintptr_t getPlatformWindow() const { return fPlatformWindow; }

    WebViewBase* getWebView() { return fWebView; }

    virtual void openSystemWebBrowser(String& url) = 0;

protected:
    void setWebView(WebViewBase* webView);

    void load();
    
    void runScript(String& source);
    void injectScript(String& source);

    void ready();
    void setKeyboardFocus(bool focus);

#if ! defined(HIPHOP_NETWORK_UI)
    void postMessage(const Variant& args, uintptr_t destination, uintptr_t exclude) override;
#endif

    void uiIdle() override;
#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif
    void sizeChanged(uint width, uint height) override;

    virtual void onDocumentReady() {}

    virtual uintptr_t createStandaloneWindow() = 0;
    virtual void      processStandaloneEvents() = 0;

private:
    void setBuiltInFunctionHandlers();

    // WebViewEventHandler

    virtual void handleWebViewLoadFinished() override;
    virtual void handleWebViewScriptMessage(const Variant& args) override;
    virtual void handleWebViewConsole(const String& tag, const String& text) override;

    typedef std::vector<Variant> MessageBuffer;

    uint32_t      fBackgroundColor;
    bool          fJsUiReady;
    uintptr_t     fPlatformWindow;
    WebViewBase*  fWebView;
    MessageBuffer fMessageBuffer;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewUI)

};

END_NAMESPACE_DISTRHO

#endif  // WEBVIEW_UI_HPP
