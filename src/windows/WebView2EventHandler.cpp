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

#include "WebView2EventHandler.hpp"
#include "com.hpp"

using namespace edge;
using namespace com;


// EnvironmentCompleted

static HRESULT STDMETHODCALLTYPE Impl_EnvironmentCompletedHandler(
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This,
    HRESULT result, ICoreWebView2Environment* environment)
{
    return static_cast<WebView2EventHandler*>(This)->handleWebViewEnvironmentCompleted(result, environment);
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl Vtbl_EnvironmentCompletedHandler = {
    Null_QueryInterface,
    Null_AddRef,
    Null_Release,
    Impl_EnvironmentCompletedHandler,
};


// ControllerCompleted

static HRESULT STDMETHODCALLTYPE Impl_ControllerCompletedHandler(
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This,
    HRESULT result, ICoreWebView2Controller* controller)
{
    return static_cast<WebView2EventHandler*>(This)->handleWebViewControllerCompleted(result, controller);
}

static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl Vtbl_ControllerCompletedHandler = {
    Null_QueryInterface,
    Null_AddRef,
    Null_Release,
    Impl_ControllerCompletedHandler,
};


// NavigationCompletedEvent

static HRESULT STDMETHODCALLTYPE Impl_NavigationCompletedEventHandler(
    ICoreWebView2NavigationCompletedEventHandler* This,
    ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args)
{
    return static_cast<WebView2EventHandler*>(This)->handleWebViewNavigationCompleted(sender, args);
}

static ICoreWebView2NavigationCompletedEventHandlerVtbl Vtbl_NavigationCompletedEventHandler = {
    Null_QueryInterface,
    Null_AddRef,
    Null_Release,
    Impl_NavigationCompletedEventHandler,
};


WebView2EventHandler::WebView2EventHandler()
    : ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler{&Vtbl_EnvironmentCompletedHandler}
    , ICoreWebView2CreateCoreWebView2ControllerCompletedHandler{&Vtbl_ControllerCompletedHandler}
    , ICoreWebView2NavigationCompletedEventHandler{&Vtbl_NavigationCompletedEventHandler}
{}
