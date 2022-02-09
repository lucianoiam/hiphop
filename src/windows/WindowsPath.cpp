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

#include <cstring>
#include <errhandlingapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shtypes.h>

#include "DistrhoPluginUtils.hpp"

#include "util/Path.hpp"
#include "util/macro.h"

USE_NAMESPACE_DISTRHO

String path::getLibraryPath()
{
    String path = String(getBinaryFilename());
    path.truncate(path.rfind('\\'));

    const char* format = getPluginFormatName();

    if (strcmp(format, "LV2") == 0) {
        return path + "\\" + kBundleLibrarySubdirectory;
    } else if (strcmp(format, "VST2") == 0) {
        return path + "\\" + kNoBundleLibrarySubdirectory;
    } else if (strcmp(format, "VST3") == 0) {
        return path.truncate(path.rfind('\\')) + "\\Resources";
    }

    return path + "\\" + kNoBundleLibrarySubdirectory;
}

String path::getCachesPath()
{
    // Get path inside user files folder: C:\Users\< USERNAME >\AppData\Local\PluginName\cache
    char dataPath[MAX_PATH];
    const HRESULT result = SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, 0, SHGFP_TYPE_DEFAULT, dataPath);
    
    if (FAILED(result)) {
        d_stderr("Could not determine user app data folder - %x", result);
        return String();
    }

    String path = String(dataPath) + "\\" XSTR(PLUGIN_BIN_BASENAME) "\\" + kCacheSubdirectory;

    // Append host executable name to the cache path otherwise WebView2 controller initialization
    // fails with HRESULT 0x8007139f when trying to load plugin into more than a single host
    // simultaneously due to permissions. C:\Users\< USERNAME >\AppData\Local\PluginName\cache\< HOST_BIN >
    char exePath[MAX_PATH];
    
    if (GetModuleFileName(0, exePath, sizeof(exePath)) == 0) {
        d_stderr("Could not determine host executable path - %x", GetLastError());
        return String();
    }

    LPSTR exeFilename = PathFindFileName(exePath);

    // The following call relies on a further Windows library called Pathcch, which is implemented
    // in api-ms-win-core-path-l1-1-0.dll and requires Windows 8.
    // Since the minimum plugin target is Windows 7 it is acceptable to use a deprecated function.
    //PathCchRemoveExtension(exeFilename, sizeof(exeFilename));
    PathRemoveExtension(exeFilename);
    path += "\\";
    path += exeFilename;

    return path;
}
