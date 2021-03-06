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

#ifndef WEB_UI_BASE_HPP
#define WEB_UI_BASE_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "extra/UIEx.hpp"
#include "extra/JSValue.hpp"

START_NAMESPACE_DISTRHO

class WebUIBase : public UIEx
{
public:
    WebUIBase(uint widthCssPx, uint heightCssPx);
    virtual ~WebUIBase() {}

    typedef std::function<void()> UiBlock;

    void queue(const UiBlock& block);

protected:
    bool isDryRun();
    
    uint getInitWidthCSS() const { return fInitWidthCssPx; }
    uint getInitHeightCSS() const { return fInitHeightCssPx; }

    void uiIdle() override;

    void parameterChanged(uint32_t index, float value) override;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(uint32_t index) override;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif
#if HIPHOP_SHARED_MEMORY_SIZE
    void sharedMemoryReady() override;
    void sharedMemoryChanged(const unsigned char* data, size_t size, uint32_t hints) override;
#endif

    virtual void postMessage(const JSValue& args, uintptr_t context) = 0;
    virtual void onMessageReceived(const JSValue& args, uintptr_t context);

    void handleMessage(const JSValue& args, uintptr_t context);

    typedef std::function<void(const JSValue& args, uintptr_t context)> MessageHandler;
    typedef std::pair<int, MessageHandler> ArgumentCountAndMessageHandler;
    typedef std::unordered_map<std::string, ArgumentCountAndMessageHandler> MessageHandlerMap;

    MessageHandlerMap fHandler;

private:
    void initHandlers();

    uint    fInitWidthCssPx;
    uint    fInitHeightCssPx;
    bool    fUiBlockQueued;
    UiBlock fUiBlock;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebUIBase)

};

END_NAMESPACE_DISTRHO

#endif  // WEB_UI_BASE_HPP
