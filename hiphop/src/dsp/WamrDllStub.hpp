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

// This file extends WasmRuntime.hpp on Windows when AOT compilation is enabled

#if defined(__GNUC__) && (__GNUC__ >= 9)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

//
// WAMR DLL lifecycle
//
void wamr_dll_load() noexcept
{
    String dllPath = Path::getPluginLibrary() + "\\" + "libiwasm.dll";
    fWamrDll = LoadLibrary(dllPath);
}

void wamr_dll_free() noexcept
{
    if (fWamrDll != nullptr) {
        FreeLibrary(fWamrDll);
        fWamrDll = nullptr;
    }
}

/**
 * WASM C API (subset)
 * 
 * These stub methods take precedence over the global functions defined in WAMR
 * wasm_c_api.h header. All platform / WASM mode combinations link to a static
 * library except AOT mode on Windows because MinGW-built libvmlib.a crashes.
 */

//
// Engine
//
inline own wasm_engine_t* wasm_engine_new(void)
{
    typedef own wasm_engine_t* (*FuncPtr)();
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_engine_new");
    return fp();
}

inline void wasm_engine_delete(own wasm_engine_t* a0)
{
    typedef void (*FuncPtr)(own wasm_engine_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_engine_delete");
    return fp(a0);
}

//
// Store
//
inline own wasm_store_t* wasm_store_new(wasm_engine_t* a0)
{
    typedef own wasm_store_t* (*FuncPtr)(wasm_engine_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_store_new");
    return fp(a0);
}

inline void wasm_store_delete(own wasm_store_t* a0)
{
    typedef void (*FuncPtr)(own wasm_store_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_store_delete");
    return fp(a0);
}

//
// Instance
//
inline own wasm_instance_t* wasm_instance_new(wasm_store_t* a0, const wasm_module_t* a1,
                                        const wasm_extern_vec_t * a2, own wasm_trap_t** a3)
{
    typedef own wasm_instance_t* (*FuncPtr)(wasm_store_t*, const wasm_module_t*,
        const wasm_extern_vec_t*, own wasm_trap_t**);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_instance_new");
    return fp(a0, a1, a2, a3);
}

inline void wasm_instance_delete(own wasm_instance_t* a0)
{
    typedef void (*FuncPtr)(own wasm_instance_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_instance_delete");
    return fp(a0);
}

inline void wasm_instance_exports(const wasm_instance_t* a0, own wasm_extern_vec_t* a1)
{
    typedef void (*FuncPtr)(const wasm_instance_t*, own wasm_extern_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_instance_exports");
    return fp(a0, a1);
}

//
// Byte vector
//
inline void wasm_byte_vec_new_uninitialized(own wasm_byte_vec_t* a0, size_t a1)
{
    typedef void (*FuncPtr)(own wasm_byte_vec_t*, size_t);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_byte_vec_new_uninitialized");
    return fp(a0, a1);
}

inline void wasm_byte_vec_delete(own wasm_byte_vec_t* a0)
{
    typedef void (*FuncPtr)(own wasm_byte_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_byte_vec_delete");
    return fp(a0);
}

//
// Module
//
inline own wasm_module_t* wasm_module_new(wasm_store_t* a0, const wasm_byte_vec_t* a1)
{
    typedef own wasm_module_t* (*FuncPtr)(wasm_store_t*, const wasm_byte_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_module_new");
    return fp(a0, a1);
}

inline void wasm_module_delete(own wasm_module_t* a0)
{
    typedef void (*FuncPtr)(own wasm_module_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_module_delete");
    return fp(a0);
}

inline void wasm_module_imports(const wasm_module_t* a0, own wasm_importtype_vec_t* a1)
{
    typedef void (*FuncPtr)(const wasm_module_t*, own wasm_importtype_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_module_imports");
    return fp(a0, a1);
}

inline void wasm_module_exports(const wasm_module_t* a0, own wasm_exporttype_vec_t* a1)
{
    typedef void (*FuncPtr)(const wasm_module_t*, own wasm_exporttype_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_module_exports");
    return fp(a0, a1);
}

//
// Import
//
inline const wasm_name_t* wasm_importtype_module(const wasm_importtype_t* a0)
{
    typedef const wasm_name_t* (*FuncPtr)(const wasm_importtype_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_importtype_module");
    return fp(a0);
}

inline const wasm_name_t* wasm_importtype_name(const wasm_importtype_t* a0)
{
    typedef const wasm_name_t* (*FuncPtr)(const wasm_importtype_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_importtype_name");
    return fp(a0);
}

inline void wasm_importtype_vec_delete(own wasm_importtype_vec_t* a0)
{
    typedef void (*FuncPtr)(own wasm_importtype_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_importtype_vec_delete");
    return fp(a0);
}

//
// Export
//
inline const wasm_name_t* wasm_exporttype_name(const wasm_exporttype_t* a0)
{
    typedef const wasm_name_t* (*FuncPtr)(const wasm_exporttype_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_exporttype_name");
    return fp(a0);
}

inline void wasm_exporttype_vec_delete(own wasm_exporttype_vec_t* a0)
{
    typedef void (*FuncPtr)(own wasm_exporttype_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_exporttype_vec_delete");
    return fp(a0);
}

//
// Extern
//
inline void wasm_extern_vec_new_uninitialized(own wasm_extern_vec_t* a0, size_t a1)
{
    typedef void (*FuncPtr)(own wasm_extern_vec_t*, size_t);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_extern_vec_new_uninitialized");
    return fp(a0, a1);
}

inline void wasm_extern_vec_delete(own wasm_extern_vec_t* a0)
{
    typedef void (*FuncPtr)(own wasm_extern_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_extern_vec_delete");
    return fp(a0);
}


inline wasm_func_t* wasm_extern_as_func(wasm_extern_t* a0)
{
    typedef wasm_func_t* (*FuncPtr)(wasm_extern_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_extern_as_func");
    return fp(a0);
}

inline wasm_global_t* wasm_extern_as_global(wasm_extern_t* a0)
{
    typedef wasm_global_t* (*FuncPtr)(wasm_extern_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_extern_as_global");
    return fp(a0);
}

inline wasm_memory_t* wasm_extern_as_memory(wasm_extern_t* a0)
{
    typedef wasm_memory_t* (*FuncPtr)(wasm_extern_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_extern_as_memory");
    return fp(a0);
}

//
// Value
//
inline own wasm_valtype_t* wasm_valtype_new(wasm_valkind_t a0)
{
    typedef own wasm_valtype_t* (*FuncPtr)(wasm_valkind_t);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_valtype_new");
    return fp(a0);
}

inline void wasm_valtype_vec_new(own wasm_valtype_vec_t* a0, size_t a1, own wasm_valtype_t* const a2[])
{
    typedef void (*FuncPtr)(own wasm_valtype_vec_t*, size_t, own wasm_valtype_t* const[]);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_valtype_vec_new");
    return fp(a0, a1, a2);
}

inline void wasm_valtype_vec_delete(own wasm_valtype_vec_t* a0)
{
    typedef void (*FuncPtr)(own wasm_valtype_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_valtype_vec_delete");
    return fp(a0);
}

//
// Function
//
inline own wasm_functype_t* wasm_functype_new(own wasm_valtype_vec_t* a0, own wasm_valtype_vec_t* a1)
{
    typedef own wasm_functype_t* (*FuncPtr)(own wasm_valtype_vec_t*, own wasm_valtype_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_functype_new");
    return fp(a0, a1);
}

inline own wasm_func_t* wasm_func_new_with_env(wasm_store_t* a0, const wasm_functype_t* a1,
                                        wasm_func_callback_with_env_t a2, void* a3,
                                        void (*a4)(void*))
{
    typedef own wasm_func_t* (*FuncPtr)(wasm_store_t*, const wasm_functype_t*,
    wasm_func_callback_with_env_t, void*, void (*)(void*));
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_func_new_with_env");
    return fp(a0, a1, a2, a3, a4);
}

inline wasm_extern_t* wasm_func_as_extern(wasm_func_t* a0)
{
    typedef wasm_extern_t* (*FuncPtr)(wasm_func_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_func_as_extern");
    return fp(a0);
}

inline own wasm_trap_t* wasm_func_call(const wasm_func_t* a0, const wasm_val_vec_t* a1,
                                        wasm_val_vec_t* a2)
{
    typedef own wasm_trap_t* (*FuncPtr)(const wasm_func_t*, const wasm_val_vec_t*,
    wasm_val_vec_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_func_call");
    return fp(a0, a1, a2);
}

//
// Memory
//
inline byte_t* wasm_memory_data(wasm_memory_t* a0)
{
    typedef byte_t* (*FuncPtr)(wasm_memory_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_memory_data");
    return fp(a0);
}

//
// Global
//
inline void wasm_global_get(const wasm_global_t* a0, own wasm_val_t* a1)
{
    typedef void (*FuncPtr)(const wasm_global_t*, own wasm_val_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_global_get");
    return fp(a0, a1);
}

inline void wasm_global_set(wasm_global_t* a0, const wasm_val_t* a1)
{
    typedef void (*FuncPtr)(wasm_global_t* a0, const wasm_val_t* a1);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_global_set");
    return fp(a0, a1);
}

//
// Trap
//
inline void wasm_trap_message(const wasm_trap_t* a0, own wasm_message_t* a1)
{
    typedef void (*FuncPtr)(const wasm_trap_t*, own wasm_message_t*);
    FuncPtr fp = (FuncPtr)GetProcAddress(fWamrDll, "wasm_trap_message");
    return fp(a0, a1);
}

#if defined(__GNUC__) && (__GNUC__ >= 9)
# pragma GCC diagnostic pop
#endif
