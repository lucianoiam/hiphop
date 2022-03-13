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

#import <AppKit/Appkit.h>

#include "MacWebViewUI.hpp"

#define fNsWindow ((NSWindow*)fWindow)

USE_NAMESPACE_DISTRHO

// DPF implementation of VST3 on macOS needs the DISTRHO::UI constructor to be
// called with already scaled dimensions because the setSize() request in
// WebViewUI::setWebView() is ignored https://github.com/DISTRHO/DPF/issues/359
// Since the parent native window only becomes available later on UI lifecycle,
// scale factor for secondary displays cannot be determined on UI construction.
// VST3/Mac plugins on secondary displays might open with wrong dimensions.

MacWebViewUI::MacWebViewUI(uint baseWidth, uint baseHeight,
        uint32_t backgroundColor, bool startLoading)
    : WebViewUI(baseWidth, baseHeight, backgroundColor, startLoading,
                [NSScreen mainScreen].backingScaleFactor)
    , fWindow(0)
{
    if (!shouldCreateWebView()) {
        return;
    }

    setWebView(new CocoaWebView()); // base class owns web view

    if (startLoading) {
        load();
    }
}

MacWebViewUI::~MacWebViewUI()
{
    [fNsWindow orderOut:nil];
    [fNsWindow release];
}

void MacWebViewUI::openSystemWebBrowser(String& url)
{
    NSString *s = [[NSString alloc] initWithCString:url.buffer() encoding:NSUTF8StringEncoding];
    NSURL *nsUrl = [[NSURL alloc] initWithString:s];
    [[NSWorkspace sharedWorkspace] openURL:nsUrl];
    [nsUrl release];
    [s release];
}

uintptr_t MacWebViewUI::createStandaloneWindow()
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    CGFloat k = [NSScreen mainScreen].backingScaleFactor;

    CGRect contentRect = NSMakeRect(0, 0, (CGFloat)getWidth() / k, (CGFloat)getHeight() / k);
    NSWindowStyleMask styleMask = NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;
    NSWindow* window = [[NSWindow alloc] initWithContentRect:contentRect
                                                   styleMask:styleMask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [window makeKeyAndOrderFront:window];
    fWindow = (uintptr_t)window;

    [pool release];

    return (uintptr_t)window.contentView;
}

void MacWebViewUI::processStandaloneEvents()
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSDate* date = [NSDate distantPast];

    for (NSEvent* event ;;) {
        event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                   untilDate:date
                                      inMode:NSDefaultRunLoopMode
                                     dequeue:YES];
        if (event == nil) {
            break;
        }

        [NSApp sendEvent:event];
    }

    [pool release];
}
