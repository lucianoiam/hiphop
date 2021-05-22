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

#include "GtkWebView.hpp"

/*
  Need to launch a separate process hosting the GTK web view because linking
  plugins to UI toolkit libraries like GTK or QT is known to be problematic.
*/

USE_NAMESPACE_DISTRHO

GtkWebView::GtkWebView()
{
    // Do not start subprocess here, some hosts like REAPER can trigger repeated
    // creation-destruction cycles of this class before settling.
}

GtkWebView::~GtkWebView()
{
    fView.terminate();
}

void GtkWebView::reparent(uintptr_t parentWindowId)
{
    if (!fView.isRunning()) {
        fView.spawn();
        fView.navigate(getContentUrl());
    }

    fView.reparent(parentWindowId);
}
