/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
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

#include "WebUIBase.hpp"
#include "WebViewBase.hpp"

#if defined(HIPHOP_NETWORK_UI)
# include "NetworkUI.hpp"
#endif

START_NAMESPACE_DISTRHO

class WebViewUI;
float getDisplayScaleFactor(WebViewUI* ui); // see [Platform]WebViewuI

#if defined(HIPHOP_NETWORK_UI)
typedef NetworkUI WebViewUIBase; // https:// + WebSockets messaging
#else
typedef WebUIBase WebViewUIBase; // file://  + Local web view messaging
#endif

class WebViewUI : public WebViewUIBase, private WebViewEventHandler
{
public:
    WebViewUI(uint widthCssPx, uint heightCssPx, uint32_t backgroundColor, 
        bool startLoading = true);
    virtual ~WebViewUI();

    typedef std::function<void()> UiBlock;

    void queue(const UiBlock& block);
    
    uint getInitialWidth() const { return fInitialWidth; }
    uint getInitialHeight() const { return fInitialHeight; }

    uintptr_t getPlatformWindow() const { return fPlatformWindow; }

    WebViewBase* getWebView() { return fWebView; }

    virtual void openSystemWebBrowser(String& url) = 0;

protected:
    bool shouldCreateWebView();
    void setWebView(WebViewBase* webView);

    void load();
    
    void runScript(String& source);
    void injectScript(String& source);

    void flushInitMessageQueue();
    void setKeyboardFocus(bool focus);

    void postMessage(const JsValueVector& args) override;

    void uiIdle() override;
    void sizeChanged(uint width, uint height) override;

    virtual void sizeRequest(const UiBlock& block);
    
    virtual void onDocumentReady() {}

    virtual uintptr_t createStandaloneWindow() = 0;
    virtual void      processStandaloneEvents() = 0;

private:
    void initHandlers();

    // WebViewEventHandler

    virtual void handleWebViewLoadFinished() override;
    virtual void handleWebViewScriptMessage(const JsValueVector& args) override;
    virtual void handleWebViewConsole(const String& tag, const String& text) override;

    typedef std::vector<JsValueVector> InitMessageQueue;

    uint              fInitialWidth;
    uint              fInitialHeight;
    uint32_t          fBackgroundColor;
    bool              fMessageQueueReady;
    bool              fUiBlockQueued;
    uintptr_t         fPlatformWindow;
    WebViewBase*      fWebView;
    UiBlock           fUiBlock;
    InitMessageQueue  fInitMessageQueue;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewUI)

};

END_NAMESPACE_DISTRHO

#endif  // WEBVIEW_UI_HPP