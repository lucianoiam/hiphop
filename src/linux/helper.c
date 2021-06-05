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

#include "helper.h"

#include <stdint.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gdk/gdkwayland.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "../DistrhoPluginInfo.h"
#include "../common/log.h"
#include "../common/macro.h"

#include "ipc.h"

typedef struct {
    ipc_t*         ipc;
    Display*       display;
    GtkWindow*     window;
    WebKitWebView* webView;
} helper_context_t;

static void create_webview(helper_context_t *ctx);
static void reparent(const helper_context_t *ctx, uintptr_t parentId);
static void inject_script(const helper_context_t *ctx, const char* js);
static void window_destroy_cb(GtkWidget* widget, GtkWidget* window);
static void web_view_load_changed_cb(WebKitWebView *view, WebKitLoadEvent event, gpointer data);
static void web_view_script_message_cb(WebKitUserContentManager *manager, WebKitJavascriptResult *res, gpointer data);
static gboolean ipc_read_cb(GIOChannel *source, GIOCondition condition, gpointer data);
static int ipc_write_simple(helper_context_t *ctx, helper_opcode_t opcode, const void *payload, int payload_sz);

int main(int argc, char* argv[])
{
    int i;
    helper_context_t ctx;
    ipc_conf_t conf;
    GIOChannel* channel;
    if (argc < 3) {
        LOG_STDERR("Invalid argument count");
        return -1;
    }
    if ((sscanf(argv[1], "%d", &conf.fd_r) == 0) || (sscanf(argv[2], "%d", &conf.fd_w) == 0)) {
        LOG_STDERR("Invalid file descriptor");
        return -1;
    }

    ctx.ipc = ipc_init(&conf);
    gtk_init(0, NULL);
    if (GDK_IS_X11_DISPLAY(gdk_display_get_default())
            && ((ctx.display = XOpenDisplay(NULL)) == NULL)) {
        LOG_STDERR("Cannot open display");
        return -1;
    }

    create_webview(&ctx);
    channel = g_io_channel_unix_new(conf.fd_r);    
    g_io_add_watch(channel, G_IO_IN|G_IO_ERR|G_IO_HUP, ipc_read_cb, &ctx);

    gtk_main();

    g_io_channel_shutdown(channel, TRUE, NULL);
    ipc_destroy(ctx.ipc);

    return 0;
}

static void create_webview(helper_context_t *ctx)
{
    ctx->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    if (!GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        // Do not remove decorations on Wayland
        gtk_window_set_decorated(ctx->window, FALSE);
    }
    // Set up callback so that if the main window is closed, the program will exit
    g_signal_connect(ctx->window, "destroy", G_CALLBACK(window_destroy_cb), NULL);
    // TODO: gtk_widget_override_background_color() is deprecated
    GdkRGBA color = {UNPACK_RGBA(DISTRHO_UI_BACKGROUND_COLOR, gdouble)};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_override_background_color(GTK_WIDGET(ctx->window), GTK_STATE_NORMAL, &color);
#pragma GCC diagnostic pop
    gtk_widget_show(GTK_WIDGET(ctx->window));
    // Create web view
    ctx->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_container_add(GTK_CONTAINER(ctx->window), GTK_WIDGET(ctx->webView));
    g_signal_connect(ctx->webView, "load-changed", G_CALLBACK(web_view_load_changed_cb), ctx);
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(ctx->webView);
    g_signal_connect(manager, "script-message-received", G_CALLBACK(web_view_script_message_cb), NULL);
    webkit_user_content_manager_register_script_message_handler(manager, "console_log");
}

static void reparent(const helper_context_t *ctx, uintptr_t parentId)
{
    GdkDisplay *gdkDisplay = gdk_display_get_default();
    if (GDK_IS_X11_DISPLAY(gdk_display_get_default())) {
        Window childId = gdk_x11_window_get_xid(gtk_widget_get_window(GTK_WIDGET(ctx->window)));
        XReparentWindow(ctx->display, childId, parentId, 0, 0);
        XFlush(ctx->display);
    } else if (GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        // TODO: show a message in parent plugin window explaining that Wayland is not supported
        //       yet and because of that the plugin web user interface will be displayed in a
        //       separate window. Ideally include a button to focus such separate window.
        LOG_STDERR_COLOR("Running Wayland, plugin will be displayed in a separate window");
    }
}

static void inject_script(const helper_context_t *ctx, const char* js)
{
    WebKitUserScript *script = webkit_user_script_new(js, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(ctx->webView);
    webkit_user_content_manager_add_script(manager, script);
    webkit_user_script_unref(script);
}

static void window_destroy_cb(GtkWidget* widget, GtkWidget* window)
{
    gtk_main_quit();
}

static void web_view_load_changed_cb(WebKitWebView *view, WebKitLoadEvent event, gpointer data)
{
    helper_context_t *ctx = (helper_context_t *)data;
    switch (event) {
        case WEBKIT_LOAD_FINISHED:
            // Load completed. All resources are done loading or there was an error during the load operation. 
            ipc_write_simple(ctx, OPC_HANDLE_LOAD_FINISHED, NULL, 0);
            gtk_widget_show(GTK_WIDGET(view));
            break;
        default:
            break;
    }
}

static void web_view_script_message_cb(WebKitUserContentManager *manager, WebKitJavascriptResult *res, gpointer data)
{
    // TODO: create union typed_value_t for IPC transfer, also add a ScriptValue(typed_value_t) constructor
    gint32 jsArgsNum;
    JSCValue *jsArg;
    JSCValue *jsArgs = webkit_javascript_result_get_js_value(res);
    if (!jsc_value_is_array(jsArgs)) {
        webkit_javascript_result_unref(res);
        return;
    }
    jsArgsNum = jsc_value_to_int32(jsc_value_object_get_property(jsArgs, "length"));
    if (jsArgsNum > 0) {
        jsArg = jsc_value_object_get_property_at_index(jsArgs, 0);
        printf("arg1: %s\n", jsc_value_to_string(jsArg));
        if (jsArgsNum > 1) {
            jsArg = jsc_value_object_get_property_at_index(jsArgs, 1);
            printf("arg2: %s\n", jsc_value_to_string(jsArg));
        }
    }
    fflush(stdout);
    webkit_javascript_result_unref(res);
}

static gboolean ipc_read_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
    helper_context_t *ctx = (helper_context_t *)data;
    tlv_t packet;

    if ((condition & G_IO_IN) == 0) {
        return TRUE;
    }
    if (ipc_read(ctx->ipc, &packet) == -1) {
        LOG_STDERR_ERRNO("Could not read from IPC channel");
        return TRUE;
    }

    switch (packet.t) {
        case OPC_NAVIGATE:
            webkit_web_view_load_uri(ctx->webView, packet.v);
            break;
        case OPC_REPARENT:
            reparent(ctx, *((uintptr_t *)packet.v));
            break;
        case OPC_RESIZE: {
            const helper_size_t *size = (const helper_size_t *)packet.v;
            gtk_window_resize(ctx->window, size->width, size->height);
            break;
        }
        case OPC_RUN_SCRIPT:
            webkit_web_view_run_javascript(ctx->webView, packet.v, NULL, NULL, NULL);
            break;
        case OPC_INJECT_SCRIPT:
            inject_script(ctx, packet.v);
            break;
        default:
            break;
    }

    return TRUE;
}

static int ipc_write_simple(helper_context_t *ctx, helper_opcode_t opcode, const void *payload, int payload_sz)
{
    int retval;
    tlv_t packet;
    packet.t = (short)opcode;
    packet.l = payload_sz;
    packet.v = payload;
    if ((retval = ipc_write(ctx->ipc, &packet)) == -1) {
        LOG_STDERR_ERRNO("Could not write to IPC channel");
    }
    return retval;
}
