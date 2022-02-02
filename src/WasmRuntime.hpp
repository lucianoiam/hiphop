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

#ifndef WASM_RUNTIME_HPP
#define WASM_RUNTIME_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef HIPHOP_WASM_RUNTIME_WAMR
# include "wasm_c_api.h"
#elif HIPHOP_WASM_RUNTIME_WASMER
# define WASM_API_EXTERN // link to static lib on win32
# include "wasm.h"
# include "wasmer.h"
#else
# error "Unknown WebAssembly runtime specified"
#endif

#include "src/DistrhoDefines.h"
#include "extra/LeakDetector.hpp"

#define MakeI32(x) WASM_I32_VAL(static_cast<int32_t>(x))
#define MakeI64(x) WASM_I64_VAL(static_cast<int64_t>(x))
#define MakeF32(x) WASM_F32_VAL(static_cast<float32_t>(x))
#define MakeF64(x) WASM_F64_VAL(static_cast<float64_t>(x))

START_NAMESPACE_DISTRHO

struct WasmFunctionDescriptor;

typedef wasm_val_t WasmValue;
typedef std::vector<WasmValue> WasmValueVector;
typedef std::vector<enum wasm_valkind_enum> WasmValueKindVector;
typedef std::function<WasmValueVector(WasmValueVector)> WasmFunction;
typedef std::vector<WasmFunction> WasmFunctionVector;
typedef std::unordered_map<std::string, WasmFunctionDescriptor> WasmFunctionMap;
typedef std::unordered_map<std::string, wasm_extern_t*> WasmExternMap;

struct WasmFunctionDescriptor
{
    WasmValueKindVector params;
    WasmValueKindVector result;
    WasmFunction        function;
};

class WasmRuntime
{
public:
    WasmRuntime();
    ~WasmRuntime();

    void load(const char* modulePath);
    void unload();

    bool isStarted() { return fStarted; }
    void start(WasmFunctionMap hostFunctions);
    void stop();

    byte_t* getMemory(const WasmValue& wPtr = MakeI32(0));
    char*   getMemoryAsCString(const WasmValue& wPtr);
    void    copyCStringToMemory(const WasmValue& wPtr, const char* s);

    WasmValue getGlobal(const char* name);
    void      setGlobal(const char* name, const WasmValue& value);
    char*     getGlobalAsCString(const char* name);

    WasmValueVector callFunction(const char* name, WasmValueVector params = {});
    WasmValue       callFunctionReturnSingleValue(const char* name, WasmValueVector params = {});
    const char*     callFunctionReturnCString(const char* name, WasmValueVector params = {});

private:
    static wasm_trap_t* callHostFunction(void *env, const wasm_val_vec_t* paramsVec, wasm_val_vec_t* resultVec);
    
    static void throwWasmLastError();

    static void toCValueTypeVector(WasmValueKindVector kinds, wasm_valtype_vec_t* types);
       
    const char* WTF16ToCString(const WasmValue& wPtr);
    WasmValue   CToWTF16String(const char* s);

    bool               fStarted;
    wasm_engine_t*     fEngine;
    wasm_store_t*      fStore;
    wasm_module_t*     fModule;
    wasm_instance_t*   fInstance;
    wasm_extern_vec_t  fExportsVec;
#ifdef HIPHOP_ENABLE_WASI
    wasi_env_t*        fWasiEnv;
#endif // HIPHOP_ENABLE_WASI
    WasmFunctionVector fHostFunctions;
    WasmExternMap      fModuleExports;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WasmRuntime)

};

END_NAMESPACE_DISTRHO

#endif  // WASM_RUNTIME_HPP