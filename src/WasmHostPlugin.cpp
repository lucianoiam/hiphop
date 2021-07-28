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

#include <sstream>

#include "WasmHostPlugin.hpp"

#include "Platform.hpp"
#include "macro.h"

#define own
#define WASM_DECLARE_NATIVE_FUNC(func) static own wasm_trap_t* func(void *env, \
                                            const wasm_val_vec_t* args, wasm_val_vec_t* results);
#define WASM_MEMORY() wasm_memory_data(wasm_extern_as_memory(fExternMap["memory"]))
#define WASM_MEMORY_CSTR(wptr) static_cast<char *>(&WASM_MEMORY()[wptr.of.i32]) 
#define WASM_FUNC_CALL(name,args,res) wasm_func_call(wasm_extern_as_func(fExternMap[name]), args, res)
#define WASM_GLOBAL_GET(name,pval) wasm_global_get(wasm_extern_as_global(fExternMap[name]), pval)
#define WASM_GLOBAL_SET(name,pval) wasm_global_set(wasm_extern_as_global(fExternMap[name]), pval)
#define WASM_DEFINE_ARGS_VAL_VEC_1(var,arg0) wasm_val_t var[1] = { arg0 }; \
                                             wasm_val_vec_t var##_val_vec = WASM_ARRAY_VEC(var);
#define WASM_DEFINE_ARGS_VAL_VEC_2(var,arg0,arg1) wasm_val_t var[2] = { arg0, arg1 }; \
                                                  wasm_val_vec_t var##_val_vec = WASM_ARRAY_VEC(var);
#define WASM_DEFINE_RES_VAL_VEC_1(var) wasm_val_t var[1] = { WASM_INIT_VAL }; \
                                       wasm_val_vec_t var##_val_vec = WASM_ARRAY_VEC(var);

USE_NAMESPACE_DISTRHO

static wasm_val_vec_t empty_val_vec = WASM_EMPTY_VEC;

static void log_wasmer_last_error();

WASM_DECLARE_NATIVE_FUNC(get_sample_rate)

#ifndef HIPHOP_ENABLE_WASI
WASM_DECLARE_NATIVE_FUNC(ascript_abort)

static own wasm_functype_t* wasm_functype_new_4_0(own wasm_valtype_t* p1, own wasm_valtype_t* p2,
                                                  own wasm_valtype_t* p3, own wasm_valtype_t* p4);
#endif

WasmHostPlugin::WasmHostPlugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
    : Plugin(parameterCount, programCount, stateCount)
    , fWasmReady(false)
    , fWasmEngine(0)
    , fWasmStore(0)
    , fWasmInstance(0)
    , fWasmModule(0)
#ifdef HIPHOP_ENABLE_WASI
    , fWasiEnv(0)
#endif
{
    memset(&fWasmExports, 0, sizeof(fWasmExports));

    // -------------------------------------------------------------------------
    // Load and initialize binary module file

    String path = platform::getLibraryPath() + "/dsp/plugin.wasm";
    FILE* file = fopen(path, "rb");

    if (file == 0) {
        log_wasmer_last_error();
        return;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    wasm_byte_vec_t fileBytes;
    wasm_byte_vec_new_uninitialized(&fileBytes, fileSize);
    
    if (fread(fileBytes.data, fileSize, 1, file) != 1) {
        wasm_byte_vec_delete(&fileBytes);
        fclose(file);
        log_wasmer_last_error();
        return;
    }

    fclose(file);

    fWasmEngine = wasm_engine_new();

    if (fWasmEngine == 0) {
        wasm_byte_vec_delete(&fileBytes);
        log_wasmer_last_error();
        return;
    }

    fWasmStore = wasm_store_new(fWasmEngine);

    if (fWasmEngine == 0) {
        wasm_byte_vec_delete(&fileBytes);
        log_wasmer_last_error();
        return;
    }

    fWasmModule = wasm_module_new(fWasmStore, &fileBytes);
    
    wasm_byte_vec_delete(&fileBytes);

    if (fWasmModule == 0) {
        log_wasmer_last_error();
        return;
    }

    char name[128];

#ifdef HIPHOP_ENABLE_WASI
    // -------------------------------------------------------------------------
    // Build a map of WASI imports
    // Call to wasi_get_imports() fails because of missing host imports, use
    // wasi_get_unordered_imports() https://github.com/wasmerio/wasmer/issues/2450

    wasi_config_t* config = wasi_config_new(DISTRHO_PLUGIN_NAME);
    fWasiEnv = wasi_env_new(config);

    if (fWasiEnv == 0) {
        log_wasmer_last_error();
        return;
    }

    wasmer_named_extern_vec_t wasiImports;
    bool res = wasi_get_unordered_imports(fWasmStore, fWasmModule, fWasiEnv, &wasiImports);

    if (!res) {
        log_wasmer_last_error();
        return;
    }

    std::unordered_map<std::string, int> wasiImportIndex;

    for (size_t i = 0; i < wasiImports.size; i++) {
        wasmer_named_extern_t *ne = wasiImports.data[i];
        const wasm_name_t *wn = wasmer_named_extern_name(ne);
        memcpy(name, wn->data, wn->size);
        name[wn->size] = 0;
        wasiImportIndex[name] = i;
    }
#endif // HIPHOP_ENABLE_WASI

    // -------------------------------------------------------------------------
    // Build module imports vector

    wasm_importtype_vec_t importTypes;
    wasm_module_imports(fWasmModule, &importTypes);
    wasm_extern_vec_t imports;
    wasm_extern_vec_new_uninitialized(&imports, importTypes.size);

    std::unordered_map<std::string, int> importIndex;
    bool needsWasi = false;

    for (size_t i = 0; i < importTypes.size; i++) {
        const wasm_name_t *wn = wasm_importtype_name(importTypes.data[i]);
        memcpy(name, wn->data, wn->size);
        name[wn->size] = 0;
        importIndex[name] = i;

#ifdef HIPHOP_ENABLE_WASI
        if (wasiImportIndex.find(name) != wasiImportIndex.end()) {
            wasmer_named_extern_t* ne = wasiImports.data[wasiImportIndex[name]];
            imports.data[i] = const_cast<wasm_extern_t *>(wasmer_named_extern_unwrap(ne));
        }
#endif
        if (!needsWasi) {
            wn = wasm_importtype_module(importTypes.data[i]);
            memcpy(name, wn->data, wn->size);
            name[wn->size] = 0;
            if (strstr(name, "wasi_") == name) { // eg, wasi_snapshot_preview1
                needsWasi = true;
            }
        }
    }

    wasm_importtype_vec_delete(&importTypes);

#ifdef HIPHOP_ENABLE_WASI
    if (!needsWasi) {
        HIPHOP_LOG_STDERR_COLOR("WASI is enabled but module is non-WASI, add missing 'import \"wasi\"' directive.");
        return;
    }
#else
    if (needsWasi) {
        HIPHOP_LOG_STDERR_COLOR("WASI is not enabled but module requires WASI, remove 'import \"wasi\"' directive.");
        return;
    }
#endif

    // -------------------------------------------------------------------------
    // Insert host functions into imports vector

    wasm_functype_t *funcType;
    wasm_func_t *func;

#ifndef HIPHOP_ENABLE_WASI
    funcType = wasm_functype_new_4_0(wasm_valtype_new_i32(), wasm_valtype_new_i32(),
                                     wasm_valtype_new_i32(), wasm_valtype_new_i32());
    func = wasm_func_new_with_env(fWasmStore, funcType, ascript_abort, this, 0);
    wasm_functype_delete(funcType);
    imports.data[importIndex["abort"]] = wasm_func_as_extern(func);
#endif
    funcType = wasm_functype_new_0_1(wasm_valtype_new_f32());
    func = wasm_func_new_with_env(fWasmStore, funcType, get_sample_rate, this, 0);
    wasm_functype_delete(funcType);
    imports.data[importIndex["_get_sample_rate"]] = wasm_func_as_extern(func);

    // -------------------------------------------------------------------------
    // Create Wasm instance and start WASI

    fWasmInstance = wasm_instance_new(fWasmStore, fWasmModule, &imports, 0);

    wasm_extern_vec_delete(&imports);

    if (fWasmInstance == 0) {
        log_wasmer_last_error();
        return;
    }
#ifdef HIPHOP_ENABLE_WASI
    wasm_func_t* wasiStart = wasi_get_start_function(fWasmInstance);
    
    if (wasiStart == 0) {
        log_wasmer_last_error();
        return;
    }

    wasm_func_call(wasiStart, &empty_val_vec, &empty_val_vec);
    wasm_func_delete(wasiStart);
#endif
    // -------------------------------------------------------------------------
    // Build a map of externs indexed by name

    fWasmExports.size = 0;
    wasm_instance_exports(fWasmInstance, &fWasmExports);
    wasm_exporttype_vec_t exportTypes;
    wasm_module_exports(fWasmModule, &exportTypes);

    for (size_t i = 0; i < fWasmExports.size; i++) {
        const wasm_name_t *wn = wasm_exporttype_name(exportTypes.data[i]);
        memcpy(name, wn->data, wn->size);
        name[wn->size] = 0;
        fExternMap[name] = fWasmExports.data[i];
    }

    wasm_exporttype_vec_delete(&exportTypes);

    // -------------------------------------------------------------------------
    // Set globals needed by run()

    wasm_val_t numInputs WASM_I32_VAL(static_cast<int32_t>(DISTRHO_PLUGIN_NUM_INPUTS));
    WASM_GLOBAL_SET("_num_inputs", &numInputs);
    wasm_val_t numOutputs WASM_I32_VAL(static_cast<int32_t>(DISTRHO_PLUGIN_NUM_OUTPUTS));
    WASM_GLOBAL_SET("_num_outputs", &numOutputs);

    fWasmReady = true;
}

WasmHostPlugin::~WasmHostPlugin()
{
    if (fWasmExports.size != 0) {
        wasm_extern_vec_delete(&fWasmExports);
    }
#ifdef HIPHOP_ENABLE_WASI
    if (fWasiEnv != 0) {
        wasi_env_delete(fWasiEnv);
    }
#endif
    if (fWasmModule != 0) {
        wasm_module_delete(fWasmModule);
    }

    if (fWasmInstance != 0) {
        wasm_instance_delete(fWasmInstance);
    }

    if (fWasmStore != 0) {
        wasm_store_delete(fWasmStore);
    }

    if (fWasmEngine != 0) {
        wasm_engine_delete(fWasmEngine);
    }
}

const char* WasmHostPlugin::getLabel() const
{
    if (!fWasmReady) {
        return "Uninitialized";
    }

    WASM_DEFINE_RES_VAL_VEC_1(res);

    if (WASM_FUNC_CALL("_get_label", &empty_val_vec, &res_val_vec) != 0) {
        log_wasmer_last_error();
        return 0;
    }

    return WASM_MEMORY_CSTR(res[0]);
}

const char* WasmHostPlugin::getMaker() const
{
    if (!fWasmReady) {
        return "Uninitialized";
    }

    WASM_DEFINE_RES_VAL_VEC_1(res);

    if (WASM_FUNC_CALL("_get_maker", &empty_val_vec, &res_val_vec) != 0) {
        log_wasmer_last_error();
        return 0;
    }

    return WASM_MEMORY_CSTR(res[0]);
}

const char* WasmHostPlugin::getLicense() const
{
    if (!fWasmReady) {
        return "Uninitialized";
    }

    WASM_DEFINE_RES_VAL_VEC_1(res);

    if (WASM_FUNC_CALL("_get_license", &empty_val_vec, &res_val_vec) != 0) {
        log_wasmer_last_error();
        return 0;
    }

    return WASM_MEMORY_CSTR(res[0]);
}

uint32_t WasmHostPlugin::getVersion() const
{
    if (!fWasmReady) {
        return 0;
    }

    WASM_DEFINE_RES_VAL_VEC_1(res);

    if (WASM_FUNC_CALL("_get_version", &empty_val_vec, &res_val_vec) != 0) {
        log_wasmer_last_error();
        return 0;
    }

    return static_cast<uint32_t>(res[0].of.i32);
}

int64_t WasmHostPlugin::getUniqueId() const
{
    if (!fWasmReady) {
        return 0;
    }

    WASM_DEFINE_RES_VAL_VEC_1(res);

    if (WASM_FUNC_CALL("_get_unique_id", &empty_val_vec, &res_val_vec) != 0) {
        log_wasmer_last_error();
        return 0;
    }

    return static_cast<int64_t>(res[0].of.i64);
}

void WasmHostPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    if (!fWasmReady) {
        return;
    }

    WASM_DEFINE_ARGS_VAL_VEC_1(args, WASM_I32_VAL(static_cast<int32_t>(index)));

    if (WASM_FUNC_CALL("_init_parameter", &args_val_vec, &empty_val_vec) != 0) {
        log_wasmer_last_error();
        return;
    }

    wasm_val_t res;

    WASM_GLOBAL_GET("_rw_int_1", &res);    parameter.hints = res.of.i32;
    WASM_GLOBAL_GET("_ro_string_1", &res); parameter.name = String(WASM_MEMORY_CSTR(res));
    WASM_GLOBAL_GET("_rw_float_1", &res);  parameter.ranges.def = res.of.f32;
    WASM_GLOBAL_GET("_rw_float_2", &res);  parameter.ranges.min = res.of.f32;
    WASM_GLOBAL_GET("_rw_float_3", &res);  parameter.ranges.max = res.of.f32;
}

float WasmHostPlugin::getParameterValue(uint32_t index) const
{
    if (!fWasmReady) {
        return 0;
    }

    WASM_DEFINE_ARGS_VAL_VEC_1(args, WASM_I32_VAL(static_cast<int32_t>(index)));
    WASM_DEFINE_RES_VAL_VEC_1(res);

    if (WASM_FUNC_CALL("_get_parameter_value", &args_val_vec, &res_val_vec) != 0) {
        log_wasmer_last_error();
        return 0;
    }

    return res[0].of.f32;
}

void WasmHostPlugin::setParameterValue(uint32_t index, float value)
{
    if (!fWasmReady) {
        return;
    }

    WASM_DEFINE_ARGS_VAL_VEC_2(args, WASM_I32_VAL(static_cast<int32_t>(index)),
                                        WASM_F32_VAL(static_cast<float32_t>(value)));

    if (WASM_FUNC_CALL("_set_parameter_value", &args_val_vec, &empty_val_vec) != 0) {
        log_wasmer_last_error();
    }
}

#if (DISTRHO_PLUGIN_WANT_STATE == 1)

void WasmHostPlugin::initState(uint32_t index, String& stateKey, String& defaultStateValue)
{
    if (!fWasmReady) {
        return;
    }

    // TODO
    (void)index;
    (void)stateKey;
    (void)defaultStateValue;
}

void WasmHostPlugin::setState(const char* key, const char* value)
{
    if (!fWasmReady) {
        return;
    }

    // TODO
    (void)key;
    (void)value;
}

#if (DISTRHO_PLUGIN_WANT_FULL_STATE == 1)

String WasmHostPlugin::getState(const char* key) const
{
    if (!fWasmReady) {
        return "Uninitialized";
    }

    // TODO
    (void)key;

    return String();
}

#endif // DISTRHO_PLUGIN_WANT_FULL_STATE == 1

#endif // DISTRHO_PLUGIN_WANT_STATE == 1

void WasmHostPlugin::activate()
{
    if (!fWasmReady) {
        return;
    }

    if (WASM_FUNC_CALL("_activate", &empty_val_vec, &empty_val_vec) != 0) {
        log_wasmer_last_error();
    }
}

void WasmHostPlugin::deactivate()
{
    if (!fWasmReady) {
        return;
    }

    if (WASM_FUNC_CALL("_deactivate", &empty_val_vec, &empty_val_vec) != 0) {
        log_wasmer_last_error();
    }
}

void WasmHostPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    if (!fWasmReady) {
        return;
    }

    wasm_val_t blockPtr;
    
    WASM_GLOBAL_GET("_input_block", &blockPtr);
    float32_t* inputBlock = reinterpret_cast<float32_t*>(&WASM_MEMORY()[blockPtr.of.i32]);

    for (int i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; i++) {
        memcpy(inputBlock + i * frames, inputs[i], frames * 4);
    }

    WASM_DEFINE_ARGS_VAL_VEC_1(args, WASM_I32_VAL(static_cast<int32_t>(frames)));

    if (WASM_FUNC_CALL("_run", &args_val_vec, &empty_val_vec) != 0) {
        log_wasmer_last_error();
        return;
    }

    WASM_GLOBAL_GET("_output_block", &blockPtr);
    float32_t* outputBlock = reinterpret_cast<float32_t*>(&WASM_MEMORY()[blockPtr.of.i32]);

    for (int i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; i++) {
        memcpy(outputs[i], outputBlock + i * frames, frames * 4);
    }
}

const char* WasmHostPlugin::readWasmString(int32_t wasmStringPtr)
{
    if (wasmStringPtr == 0) {
        return "(null)";
    }

    WASM_DEFINE_ARGS_VAL_VEC_1(args, WASM_I32_VAL(static_cast<int32_t>(wasmStringPtr)));
    WASM_DEFINE_RES_VAL_VEC_1(res);

    if (WASM_FUNC_CALL("_c_string", &args_val_vec, &res_val_vec) != 0) {
        log_wasmer_last_error();
        return 0;
    }

    return WASM_MEMORY_CSTR(res[0]);
}

static void log_wasmer_last_error()
{
    int len = wasmer_last_error_length();
    
    if (len == 0) {
        HIPHOP_LOG_STDERR_COLOR("Unknown error");
        return;
    }

    char s[len];
    wasmer_last_error_message(s, len);
    
    HIPHOP_LOG_STDERR_COLOR(s);
}

static own wasm_trap_t* get_sample_rate(void* env,
                                            const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    (void)args;

    WasmHostPlugin* p = static_cast<WasmHostPlugin *>(env);
    float32_t value = static_cast<float32_t>(p->getSampleRate());
    wasm_val_t res[1] = { WASM_F32_VAL(value) };

    wasm_val_vec_new(results, 1, res);
    
    return 0;
}

#ifndef HIPHOP_ENABLE_WASI

// Convenience function, Wasmer provides up to wasm_functype_new_3_0()
static own wasm_functype_t* wasm_functype_new_4_0(own wasm_valtype_t* p1, own wasm_valtype_t* p2,
                                                    own wasm_valtype_t* p3, own wasm_valtype_t* p4)
{
    wasm_valtype_t* ps[4] = {p1, p2, p3, p4};
    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new(&params, 4, ps);
    wasm_valtype_vec_new_empty(&results);
    return wasm_functype_new(&params, &results);
}

// Only required when running in non-WASI mode
static own wasm_trap_t* ascript_abort(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    (void)results;

    WasmHostPlugin* p = static_cast<WasmHostPlugin *>(env);

    const char *msg = p->readWasmString(args->data[0].of.i32);
    const char *filename = p->readWasmString(args->data[1].of.i32);
    int32_t lineNumber = args->data[2].of.i32;
    int32_t columnNumber = args->data[3].of.i32;

    std::stringstream ss;
    ss << "AssemblyScript abort() called - msg: " << msg << ", filename: " << filename
        << ", lineNumber: " << lineNumber << ", columnNumber: " << columnNumber;

    HIPHOP_LOG_STDERR_COLOR(ss.str().c_str());

    return 0;
}

#endif // HIPHOP_ENABLE_WASI
