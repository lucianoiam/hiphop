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

#include "Platform.hpp"

#include <cstring>
#include <dlfcn.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/syslimits.h>

#include "log.h"

USE_NAMESPACE_DISTRHO

String platform::getBinaryDirectoryPath()
{
    char path[PATH_MAX];
    ::strcpy(path, getBinaryPath());
    return String(::dirname(path));
}

String platform::getBinaryPath()
{
    Dl_info dl_info;
    if (::dladdr((void *)&__PRETTY_FUNCTION__, &dl_info) == 0) {
        LOG_STDERR(::dlerror());
        return String();
    }
    return String(dl_info.dli_fname);
}

String platform::getSharedLibraryPath()
{
    return getBinaryPath();
}

String platform::getExecutablePath()
{
    return getBinaryPath();
}

String platform::getTemporaryPath()
{
    return String(); // not implemented
}

float platform::getSystemDisplayScaleFactor()
{
    return 1.f; // not implemented
}
