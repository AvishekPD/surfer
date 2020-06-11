
/*  Copyright © 2017 nihilowy@gmail.com
 *
 *  This file is part of surfer
 *
 *  Surfer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Surfer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Surfer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gdk/gdk.h>

#include <sys/file.h>

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
//#include <sys/wait.h>
#include "config.h"


typedef struct Client{
    GtkWidget *main_window;
    GtkWidget *entry_find;
    GtkWidget *entry_open;
    GtkWidget *box_find;

    GtkWidget *button_goback;
    GtkWidget *button_goforward;
    GtkWidget *button_dm;
    GtkWidget *button_js;
    GtkWidget *button_history;
    GtkWidget *button_find_back;
    GtkWidget *button_find_close;


    GtkWidget *box_open;

    GtkWidget *vbox;

    WebKitWebView *webView;
//  WebKitPolicyDecision *decision1;
    WebKitFindController *fc;
    WebKitHitTestResult *mousepos;

    gboolean f;
    gboolean fs;
    gboolean s;
    gboolean o;
    
    gboolean enablejs;

    int progress;
    const gchar *title,*targeturi,*overtitle,*url;


  /* TLS information. */
  GTlsCertificate *certificate;
  GTlsCertificateFlags tls_errors;

  gboolean bypass_safe_browsing;
  gboolean loading_error_page;
  char *tls_error_failing_uri;




} Client;


static GtkWidget *menuitem1;
static GtkWidget *menu;

GHashTable *tablecss;



static gint clients = 0,downloads = 0;
gchar *home;
gchar *history;
gchar *favpath;
gchar *histpath;
gchar *downloads_dir;
gchar *surfer_dir;



static gchar *fullname = "";

static gboolean isrelated = TRUE;
static gboolean recordhistory= TRUE;
static gboolean wc_setup_done = FALSE;
static gboolean istmpdownload =FALSE;

static gboolean enablehist=FALSE;


static void destroy_window(GtkWidget* w,Client *rc);
//static void tls_certs(WebKitWebContext *wc);
static void allow_tls_cert (Client *c);
static void display_webview( WebKitWebView *rc,Client *c);

static gboolean close_request(WebKitWebView *view,Client *c);

static Client *client_new(Client *rc);
static WebKitWebView *clientview(Client *c, WebKitWebView *rv);

static gboolean crashed(WebKitWebView *v, Client *c);
static gboolean permission_request_cb (WebKitWebView *web_view,WebKitPermissionRequest *request,Client *c);

static void loadurl(Client *rc, gchar *url);

static WebKitWebView *create_request(WebKitWebView *rv,WebKitNavigationAction *navact, Client *c);

static gboolean decide_policy(WebKitWebView *v,WebKitPolicyDecision *decision,WebKitPolicyDecisionType type, Client *c);
static void decide_navaction(WebKitPolicyDecision *decision,Client *c);
static void decide_newwindow(WebKitPolicyDecision *decision,Client *c);
static void decide_response(WebKitPolicyDecision *decision,Client *c);

static gboolean download_button_press( Client *c);
static gboolean download_handle(WebKitDownload *, gchar *, gpointer);
static void download_handle_start(WebKitWebView *, WebKitDownload *, gpointer);
static void download_cancel( GtkWidget *,WebKitDownload *download);
static void download_handle_finished(WebKitDownload *download, gpointer data);
static void download_progress( WebKitDownload *download,GParamSpec *pspec,    GtkWidget *tb);


static void changed_title(WebKitWebView *view, GParamSpec *ps, Client *c);
static void changed_url(WebKitWebView *rv,Client *c );
static void changed_webload(WebKitWebView *webview,WebKitLoadEvent event, Client *c);
static void changed_estimated(WebKitWebView *webview, GParamSpec *pspec,Client *c);

static void update_title(Client *c);

static gboolean menucreate_cb (WebKitWebView *web_view,WebKitContextMenu *context_menu,GdkEvent *event, WebKitHitTestResult *h,Client *c);
static void downloadtmphandler(Client *c);
static void mpvhandler(Client *c);
static void mousetargetchanged(WebKitWebView *v, WebKitHitTestResult *h,guint modifiers, Client *c);



static gboolean keyboard(GtkWidget *widget, GdkEvent *event, Client *c,  gpointer );
static void openlink(GtkWidget *widget,Client *c);

static void find(GtkWidget *widget,Client *c);
static void find_close( Client *c);
static void find_back(GtkWidget * widget,Client *c);

static void togglejs_cb(GtkWidget * widget,Client *c);
static void togglehistory_cb(GtkWidget * widget,Client *c);
static void togglefind_cb(Client *c);
static void toggleopen_cb(GtkWidget *widget,Client *c);
static void togglefullscreen_cb(Client *c);
static void toggleuserstyle_cb(Client *c);
static void goback(WebKitWebView *rv,Client *c);
static void goforward(WebKitWebView *rv,Client *c);
static void bookmark_cb(Client *c);

static gboolean setup();
static GHashTable *create_hash_table_from_file (gchar *tablepath);
static void remove_newline(char buffer[]);
static void destroy_hash_table (GHashTable *table);



static void
destroy_window(GtkWidget* w,Client *c) {
    webkit_web_view_stop_loading(c->webView);
    free(c);
    clients--;

    if (clients == 0)
        gtk_main_quit();
}

gboolean
close_request(WebKitWebView *view,Client *c) {


    gtk_widget_destroy(c->main_window);

    return TRUE;
}


gboolean crashed(WebKitWebView *v, Client *c){


webkit_web_view_reload(c->webView);

return TRUE;

}


void
display_webview(WebKitWebView *rc, Client *c)
{
 //   printf("new\n");
    gtk_widget_show_all(c->main_window);
    gtk_widget_grab_focus(GTK_WIDGET(c->webView));
    gtk_widget_hide(c->box_find);
    gtk_widget_hide(c->box_open);
}

WebKitWebView *create_request(WebKitWebView *rv,WebKitNavigationAction *navact, Client *c)
{
    Client *rc;

     rc = client_new(c);
    // rc->webView = rv;
    return rc->webView;
}




Client *client_new(Client *rc) {

    Client *c;

// c = malloc(sizeof(Client));
    c = calloc(1, sizeof(Client));


    c->o = FALSE;
    c->f = FALSE;
    c->s = FALSE;
    c->fs = FALSE;
    c->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_default_size(GTK_WINDOW(c->main_window), SURFER_WINDOW_WIDTH, SURFER_WINDOW_HEIGHT);

 //   c->webView = clientview(c);
    c->webView = clientview(c, rc ? rc->webView : NULL);


    c->fc= webkit_web_view_get_find_controller(c->webView);



    c->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    c->box_find = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    c->box_open = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    c->button_goback = gtk_button_new_with_label("<");
    c->button_goforward = gtk_button_new_with_label(">");
    c->button_dm = gtk_button_new_with_label("[...]");



    c->button_js = gtk_button_new_with_label("JS");


    c->button_history = gtk_button_new_with_label("H");

    gtk_widget_set_tooltip_text(c->button_dm,"Downloads");
    gtk_widget_set_tooltip_text(c->button_js,"JS toogle");
    gtk_widget_set_tooltip_text(c->button_history,"History toogle");

    gtk_widget_show_all (menuitem1);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem1);

    c->button_find_close = gtk_button_new_with_label ("Close");
    c->button_find_back = gtk_button_new_with_label ("Find Back");
    c->entry_find = gtk_entry_new();
    c->entry_open= gtk_entry_new();


    gtk_box_pack_start(GTK_BOX(c->box_find), c->entry_find, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(c->box_find), c->button_find_back, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(c->box_find), c->button_find_close,TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(c->box_open), c->button_goback,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c->box_open), c->button_goforward,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c->box_open), c->button_dm,FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c->box_open),c->entry_open, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(c->box_open),c->button_history, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(c->box_open),c->button_js, FALSE, FALSE, 0);


    gtk_box_pack_start(GTK_BOX (c->vbox),  c->box_open, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c->vbox),GTK_WIDGET(c->webView), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(c->vbox),c->box_find, FALSE, FALSE, 0);

    //gtk_container_add(GTK_CONTAINER(c->button_dm), GTK_WIDGET(menu));

    gtk_container_add(GTK_CONTAINER(c->main_window), GTK_WIDGET(c->vbox));



    g_signal_connect(G_OBJECT(c->box_open), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(G_OBJECT(c->entry_open), "activate", G_CALLBACK(openlink), c);


    g_signal_connect(G_OBJECT(c->button_goback), "clicked", G_CALLBACK(goback), c);
    g_signal_connect(G_OBJECT(c->button_goforward), "clicked", G_CALLBACK(goforward), c);
    g_signal_connect(G_OBJECT(c->button_dm), "clicked", G_CALLBACK(download_button_press), c);
    g_signal_connect(G_OBJECT(c->button_history), "clicked", G_CALLBACK(togglehistory_cb), c);
    g_signal_connect(G_OBJECT(c->button_js), "clicked", G_CALLBACK(togglejs_cb), c);

    g_signal_connect(G_OBJECT(c->entry_find), "activate", G_CALLBACK(find), c);
    g_signal_connect(G_OBJECT(c->button_find_back), "clicked", G_CALLBACK(find_back), c);
    g_signal_connect_swapped (G_OBJECT (c->button_find_close), "clicked",G_CALLBACK (find_close),c);

    g_signal_connect(G_OBJECT(c->main_window), "key-press-event", G_CALLBACK(keyboard),c);
    g_signal_connect(G_OBJECT(c->main_window), "destroy", G_CALLBACK(destroy_window), c);

   // g_signal_connect_object (G_OBJECT(c->main_window), "allow-tls-certificate",G_CALLBACK (allow_tls_cert),c);

    gtk_widget_show_all(c->main_window);
//    gtk_widget_grab_focus(GTK_WIDGET(c->webView));

    display_webview(NULL, c);

 if (DEFAULT_STYLE_ENABLE == 1){
    toggleuserstyle_cb(c);
 }


 if (HISTORY_ENABLE == 1)
 enablehist = TRUE;


 c->enablejs =TRUE;
 
    clients++;
 return c;
}



WebKitWebView *clientview(Client *c,WebKitWebView *rv)
{
WebKitWebView *view;

WebKitUserContentManager *contentmanager;
WebKitWebsiteDataManager *mgr;
WebKitCookieManager *cookiemgr;

WebKitWebContext *wc;
WebKitSettings *settings;



FILE *File;

//gchar *cookies_path = g_build_filename(getenv("HOME"), surfer_dir, NULL);
gchar *cookie_file = g_build_filename(surfer_dir, "cookie", NULL);

gchar *datadir  = g_build_filename(g_get_user_data_dir() , fullname, NULL);
gchar *cachedir = g_build_filename(g_get_user_cache_dir(), fullname, NULL);



if (rv) {
		if(isrelated)
		view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_related_view(rv));
                 //  printf("related\n");
		else{
		view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(webkit_web_view_get_context(rv)));
		isrelated = TRUE;
		}
	}


else {

wc= webkit_web_context_new();

settings = webkit_settings_new();

contentmanager = webkit_user_content_manager_new();


//mgr = webkit_website_data_manager_new("base-data-directory" , datadir,"base-cache-directory", cachedir,NULL);
//wc = webkit_web_context_new_with_website_data_manager(mgr);

   // view= WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(wc));
 //   char *value = "Mozilla/5.0";
 //   g_object_set(settings, "user-agent", &value, NULL);

    if (!wc_setup_done)
    {

    g_signal_connect(G_OBJECT(wc), "download-started", G_CALLBACK(download_handle_start), NULL);
    wc_setup_done = TRUE;
    }

    g_object_set(G_OBJECT(settings), "minimum-font-size", FONT_MIN_SIZE, NULL);
    g_object_set(G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);
//    g_object_set(G_OBJECT(settings), "enable-webgl", TRUE, NULL);

    g_object_set(G_OBJECT(settings), "enable-mediasource", TRUE, NULL);
    g_object_set(G_OBJECT(settings),"enable-javascript", TRUE, NULL);



webkit_web_context_set_process_model(wc,WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);

webkit_web_context_set_web_extensions_directory(wc, WEB_EXTENSIONS_DIRECTORY);

//webkit_web_context_set_sandbox_enabled(wc,enabled);


    if (!g_file_test(cookie_file, G_FILE_TEST_EXISTS)) {
        File = fopen(cookie_file, "wb+");

        fclose(File);
    }




 // cookiemgr = webkit_website_data_manager_get_cookie_manager(mgr);

  cookiemgr = webkit_web_context_get_cookie_manager(wc);

   webkit_cookie_manager_set_persistent_storage(cookiemgr, cookie_file,WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
   webkit_cookie_manager_set_accept_policy(cookiemgr,SURFER_COOKIE_POLICY);


   webkit_web_context_set_tls_errors_policy(wc, WEBKIT_TLS_ERRORS_POLICY_IGNORE);


view=g_object_new(WEBKIT_TYPE_WEB_VIEW,"settings",settings,"user-content-manager",contentmanager,"web-context",wc,NULL);

 //printf("new\n");
}

g_object_connect(
        G_OBJECT(view),"signal::load-changed", G_CALLBACK(changed_webload),c,
                       "signal::notify::title", G_CALLBACK(changed_title),c ,
//                       "signal::notify::url", G_CALLBACK(changed_url), c,
                       "signal::notify::estimated-load-progress",G_CALLBACK(changed_estimated),c,
                       "signal::decide-policy", G_CALLBACK(decide_policy),c,
                       "signal::close", G_CALLBACK(close_request), c,
                       "signal::ready-to-show",G_CALLBACK(display_webview), c,
                       "signal::create",G_CALLBACK(create_request), c,
                       "signal::context-menu",G_CALLBACK(menucreate_cb), c,
                       "signal::mouse-target-changed",G_CALLBACK(mousetargetchanged), c,
//                       "signal::web-process-crashed",G_CALLBACK(crashed), c,
                       "signal::permission-request",G_CALLBACK(permission_request_cb), c,
//                       "signal::load-failed-with-tls-errors", G_CALLBACK(allow_tls_cert), c,
    NULL
    );





return view;
}


void
loadurl(Client *c,  gchar *url)
{
    gchar *link;
    gchar *nospaces;

    nospaces = g_strchug(url);
    link = g_ascii_strdown(nospaces, -1);

    if (!g_str_has_prefix(link, "http:") &&
        !g_str_has_prefix(link, "https:") &&
        !g_str_has_prefix(link, "file:") &&
        !g_str_has_prefix(link, "about:")) {
        g_free(link);
        link = g_strdup_printf("http://%s", url);
    } else

       link = g_strdup(url);



  // link = soup_uri_normalize(url,NULL);
 //printf("%s/n",link);
   webkit_web_view_load_uri(WEBKIT_WEB_VIEW(c->webView), link);

   g_free(link);



}


gboolean permission_request_cb (WebKitWebView *web_view,WebKitPermissionRequest *request,Client *c)
{

  char *msg= NULL;

     if (WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request))
        msg = "Allow access your location";
      else if (WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(request)) {
        if (webkit_user_media_permission_is_for_audio_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
            msg = "Allow access the microphone";
        }
        else if (webkit_user_media_permission_is_for_video_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
            msg = "Alllow to access webcam";
        }
    }
     else
        return FALSE;

    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(c->main_window),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_YES_NO,
                                                msg);
    gtk_widget_show (dialog);
    gint result = gtk_dialog_run (GTK_DIALOG (dialog));



    switch (result) {
    case GTK_RESPONSE_YES:
        webkit_permission_request_allow (request);
        break;
    default:
        webkit_permission_request_deny (request);
        break;
    }
    gtk_widget_destroy (dialog);

    return TRUE;
}



gboolean
download_button_press( Client *c )
{

    gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);

    return FALSE;
}


void
download_handle_start(WebKitWebView *web_view, WebKitDownload *download,
                      gpointer data)
{
    g_signal_connect(G_OBJECT(download), "decide-destination",
                     G_CALLBACK(download_handle), data);
}


gboolean
download_handle(WebKitDownload *download, gchar *suggested_filename, gpointer data)
{
    gchar *sug, *path, *path2 = NULL, *basename;
    GtkWidget *tb;
    int suffix = 1;
    size_t i;
    gchar *uri = NULL;
    const gchar *download_uri;
    gchar *downloadsfilename;


    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;



   if (!suggested_filename || !*suggested_filename) {
        download_uri = webkit_uri_request_get_uri(webkit_download_get_request(download));
        uri  = soup_uri_decode(download_uri);
        basename     = g_filename_display_basename(uri);
        g_free(uri);

        suggested_filename = basename;
      }

    sug = g_strdup(suggested_filename);

    //path = g_build_filename(downloads_dir, sug, NULL);
    //path2 = g_strdup(path);

    dialog = gtk_file_chooser_dialog_new ("Save File",
                                      GTK_WINDOW_TOPLEVEL,
                                      action,
                                      ("Cancel"),
                                      GTK_RESPONSE_CANCEL,
                                      ("Save"),
                                      GTK_RESPONSE_ACCEPT,
                                      NULL);
    chooser = GTK_FILE_CHOOSER (dialog);

    gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);
    webkit_download_set_allow_overwrite (download,TRUE);

    gtk_file_chooser_set_current_folder(chooser,downloads_dir);
    gtk_file_chooser_set_current_name (chooser,sug);

    res = gtk_dialog_run (GTK_DIALOG (dialog));

   if (res == GTK_RESPONSE_ACCEPT)
   {
   char *filename;


   filename = gtk_file_chooser_get_filename (chooser);
   uri = g_filename_to_uri(filename, NULL, NULL);
   webkit_download_set_destination(download, uri);
   g_free(uri);
   downloads++;

   tb = gtk_menu_item_new_with_label(sug);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu),tb);
   gtk_widget_show_all (GTK_WIDGET(menu));
   g_free (filename);


   gtk_widget_destroy (dialog);

     g_signal_connect(G_OBJECT(download), "notify::estimated-progress",G_CALLBACK(download_progress), tb);

     g_signal_connect(G_OBJECT(download), "finished",G_CALLBACK(download_handle_finished), NULL);

     g_object_ref(download);
     g_signal_connect(G_OBJECT(tb), "activate",G_CALLBACK(download_cancel), download);

   }
   else if (res == GTK_RESPONSE_CANCEL)
   {
   gtk_widget_destroy (dialog);
   webkit_download_cancel(download);
   }

    if (istmpdownload){

   downloadsfilename = g_strdup_printf("%s", SURFER_DOWNLOADS);
   downloads_dir = g_build_filename(downloadsfilename, NULL);
   g_free(downloadsfilename);


   } 



    g_free(sug);
    //g_free(path);
    //g_free(path2);

    return FALSE;
}

void
download_cancel( GtkWidget *tb,WebKitDownload *download)
{

    webkit_download_cancel(download);
    g_object_unref(download);

    gtk_widget_destroy(GTK_WIDGET(tb));

}

void
download_handle_finished(WebKitDownload *download, gpointer data)
{
    downloads--;
//    g_object_unref(download);

}


void
download_progress( WebKitDownload *download,GParamSpec *pspec,    GtkWidget *tb)
{
    WebKitURIResponse *resp;

    const gchar *uri;
    gchar *t, *filename, *base;
    gdouble p,size_mb;
    int b;
    p = webkit_download_get_estimated_progress(download);
    p = p > 1 ? 1 : p;
    p = p < 0 ? 0 : p;
    p *= 100;
//    b = (int) p;



    resp = webkit_download_get_response(download);
    size_mb = webkit_uri_response_get_content_length(resp) / 1e6;

    uri = webkit_download_get_destination(download);
    filename = g_filename_from_uri(uri, NULL, NULL);

    base = g_path_get_basename(filename);
        t = g_strdup_printf("%s (%.0f%% of %.1f MB)", base, p, size_mb);
        g_free(filename);
        g_free(base);

    gtk_menu_item_set_label(GTK_MENU_ITEM(tb), t);
    g_free(t);



}

void
mpvhandler(Client *c)
{
    char* t;
    char* argv[2];
    pid_t child_pid;
    int child_status;

    t = (char*)c->targeturi;
    argv[0] = SURFER_PLAYER;
    argv[1] = t;
    child_pid = fork();
    if(child_pid == 0) {

    execvp(argv[0], argv);

    printf("Unknown command\n");
    exit(0);
  }
/*   else
   {

    waitpid(child_pid, &child_status, 0);
   }
*/ 
}

void
downloadtmphandler(Client *c)
{

   gchar *downloadsfilename;

   downloadsfilename = g_strdup_printf("%s", SURFER_TMPDOWNLOADS);
   downloads_dir = g_build_filename(downloadsfilename, NULL);

   istmpdownload = TRUE;
   g_free(downloadsfilename);

   webkit_web_context_download_uri(webkit_web_view_get_context(c->webView),c->targeturi);


}

gboolean
menucreate_cb (WebKitWebView *web_view, WebKitContextMenu *context_menu,GdkEvent *event, WebKitHitTestResult *h,Client *c)
{


    WebKitContextMenuItem *menu_item;
    GSimpleAction *action;

    if (webkit_hit_test_result_context_is_link(h)) {
        menu_item = webkit_context_menu_item_new_separator();
        webkit_context_menu_append(context_menu, menu_item);

        action = g_simple_action_new("mpv-handler", NULL);

        g_signal_connect_swapped(G_OBJECT(action), "activate",G_CALLBACK(mpvhandler), c);
        menu_item = webkit_context_menu_item_new_from_gaction(G_ACTION(action),
                "Play in mpv (if supported)", NULL);
        webkit_context_menu_append(context_menu, menu_item);
        g_object_unref(action);

        action = g_simple_action_new("downloadtmp-handler", NULL);

        g_signal_connect_swapped(G_OBJECT(action), "activate",G_CALLBACK(downloadtmphandler), c);
        menu_item = webkit_context_menu_item_new_from_gaction(G_ACTION(action),
                "Download to /tmp", NULL);
        webkit_context_menu_append(context_menu, menu_item);
        g_object_unref(action);

    }
    return FALSE;
}





void
update_title(Client *c){

   char *title;
   const gchar *url;

 //  url = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(c->webView));


   if (c->progress != 100)
   title = g_strdup_printf("[%i%%] %s",c->progress, c->title);
   else
   title = g_strdup_printf("%s", c->title);

   gtk_window_set_title(GTK_WINDOW(c->main_window), title);


 //  gtk_entry_set_text(GTK_ENTRY(c->entry_open), url);



}



void
changed_estimated(WebKitWebView *webview, GParamSpec *pspec,Client *c)
{
    gdouble prog;

 if (!c->fs){
    c->progress = webkit_web_view_get_estimated_load_progress(WEBKIT_WEB_VIEW(c->webView))*100;

   if (prog==1)
     prog=0;

 }
 else {
   prog= webkit_web_view_get_estimated_load_progress(WEBKIT_WEB_VIEW(c->webView));
   if (prog==1)
     prog=0;

   gtk_entry_set_progress_fraction(GTK_ENTRY(c->entry_open), prog);
 }
 update_title(c);

}




void
changed_title(WebKitWebView *view, GParamSpec *ps, Client *c) {


    c->title = webkit_web_view_get_title(WEBKIT_WEB_VIEW(c->webView));
    update_title(c);



}

void
changed_url(WebKitWebView *rv,Client *c) {


//   update_title(c); 

}


static void changed_webload(WebKitWebView *webview, WebKitLoadEvent event,Client *c)
{
   FILE *File;
   const gchar *url;
   char *path2,*path;
   char textdate[100];
   const gchar *csspath;

    switch (event) {
       case WEBKIT_LOAD_STARTED:
         break;
       case WEBKIT_LOAD_REDIRECTED:
         break;
        case WEBKIT_LOAD_COMMITTED:
         url = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(c->webView));

         gtk_entry_set_text(GTK_ENTRY(c->entry_open), url);

        if( recordhistory==TRUE  && enablehist==TRUE ){

           time_t now = time(NULL);
           struct tm *t = localtime(&now);
           strftime(textdate, sizeof(textdate)-1, "%d %m %Y %H:%M", t);


           File = fopen(histpath, "a");
           fprintf(File, "%s	%.100s\n<br>",textdate,url );
           fclose(File);

          }
          recordhistory= TRUE;

  	if (url) {
  	 path2 = strchr(url, '/');
         path = strtok(path2, "/");
         csspath = g_hash_table_lookup(tablecss,path);

         gchar *contents;

         if(csspath){
           if(g_file_get_contents(csspath,&contents,NULL,NULL)){

            webkit_user_content_manager_add_style_sheet(
	     webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(c->webView)),
	     webkit_user_style_sheet_new(contents,WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,WEBKIT_USER_STYLE_LEVEL_USER,NULL, NULL));

	   g_free(contents);
	  }

	 }
	 else{
		if(c->s);
		else
             webkit_user_content_manager_remove_all_style_sheets(webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(c->webView)));
	 }
	}
            break;

        case WEBKIT_LOAD_FINISHED:
          break;
        default:
          break;
        }
}



void
mousetargetchanged(WebKitWebView *v, WebKitHitTestResult *h, guint modifiers,Client *c)
{
   WebKitHitTestResultContext hc = webkit_hit_test_result_get_context(h);


   c->mousepos = h;

   if (hc & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK)
 		c->targeturi = webkit_hit_test_result_get_link_uri(h);
   else if (hc & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE)
  		c->targeturi = webkit_hit_test_result_get_image_uri(h);
   else if (hc & WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA)
  		c->targeturi = webkit_hit_test_result_get_media_uri(h);
   else
  		c->targeturi = NULL;

  //	update_title(c);
}




gboolean
decide_policy( WebKitWebView *v,WebKitPolicyDecision *decision, WebKitPolicyDecisionType type,Client *c) {


    switch (type) {
        case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
             decide_navaction(decision,c);
            break;

        case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
           decide_newwindow(decision,c);
            break;

        case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
           decide_response(decision,c);
            break;

        default:
            webkit_policy_decision_ignore(decision);
           break;
    }
    return TRUE;
}

void decide_navaction(WebKitPolicyDecision *decision,Client *c) {

    WebKitNavigationType navigation_type;
    WebKitNavigationPolicyDecision *navigationDecision;
    WebKitNavigationAction *navigation_action;
    WebKitURIRequest *request;
    guint button, mods;
    gchar *link;
    gchar *t;
    Client *rc;


    navigation_action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
    request = webkit_navigation_action_get_request(navigation_action);
    navigation_type = webkit_navigation_action_get_navigation_type(navigation_action);

    t =  ( gchar *) webkit_uri_request_get_uri(request);
    mods = webkit_navigation_action_get_modifiers(navigation_action);
    button = webkit_navigation_action_get_mouse_button(navigation_action);



    if (navigation_type == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED && button == 1 && mods & SURFER_META_MASK) {

       webkit_policy_decision_ignore(decision);
   //                 printf("new\n");
       isrelated = FALSE;

       rc = client_new(c);
       loadurl(rc,t);
      // g_free(t);
     }
    else webkit_policy_decision_use(decision);
 //    printf("no\n");
}

void decide_newwindow(WebKitPolicyDecision *decision,Client *c)
{

    WebKitNavigationType navigation_type;
    WebKitNavigationAction *navigation_action;
    WebKitURIRequest *request;
    gchar *t;
    Client *rc;

    navigation_action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
    request = webkit_navigation_action_get_request(navigation_action);
    navigation_type =webkit_navigation_action_get_navigation_type(navigation_action);

    switch (navigation_type) {
        case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED:

             t = (gchar *) webkit_uri_request_get_uri(request);
             rc = client_new(c);
             loadurl(rc,t);
        case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED: 
        case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD:
        case WEBKIT_NAVIGATION_TYPE_RELOAD:
        case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED:
        case WEBKIT_NAVIGATION_TYPE_OTHER:
        default:

           break;
     }
    webkit_policy_decision_ignore(decision);

}

void decide_response(WebKitPolicyDecision *decision,Client *c){

   guint status;
   WebKitURIResponse *response;

   response = webkit_response_policy_decision_get_response(WEBKIT_RESPONSE_POLICY_DECISION(decision));
   status = webkit_uri_response_get_status_code(response);


   if (webkit_response_policy_decision_is_mime_type_supported(WEBKIT_RESPONSE_POLICY_DECISION(decision)))
        webkit_policy_decision_use(decision);
   else if (SOUP_STATUS_IS_SUCCESSFUL(status) || status == SOUP_STATUS_NONE)
        webkit_policy_decision_download(decision);
   else
        webkit_policy_decision_ignore(decision);

}




gboolean
keyboard(GtkWidget *widget,GdkEvent *event, Client *c,  gpointer data) {


    WebKitWebInspector *inspector;

    Client *rc;
    const gchar *url;
    gdouble z;
    guint meta_key_pressed;
    int key_pressed;
//   GdkEvent *event = c->eventt;


    if (event->type == GDK_KEY_PRESS) {
        key_pressed = ((GdkEventKey *) event)->keyval;
        meta_key_pressed = ((GdkEventKey *) event)->state & SURFER_META_MASK;

        if (meta_key_pressed) {
            switch (key_pressed) {
                case SURFER_SCROLL_DOWN_KEY:
                    event->key.keyval = GDK_KEY_Down;
                    gdk_event_put(event);
                    return TRUE;

                case SURFER_SCROLL_UP_KEY:
                    event->key.keyval = GDK_KEY_Up;
                    gdk_event_put(event);
                    return TRUE;

                case SURFER_SCROLL_PAGE_UP_KEY:
                    event->key.keyval = GDK_KEY_Page_Up;
                    gdk_event_put(event);
                    return TRUE;

                case SURFER_SCROLL_PAGE_DOWN_KEY:
                    event->key.keyval = GDK_KEY_Page_Down;
                    gdk_event_put(event);
                    return TRUE;

                case SURFER_CLOSE_KEY:
                    //gtk_widget_destroy(c->main_window);
                    close_request(c->webView,c);
                    return TRUE;

                case SURFER_BACK_KEY:
          	     goback(c->webView,c);

                    return TRUE;

                case SURFER_FORWARD_KEY:
                     goforward(c->webView,c);

                    return TRUE;

                case SURFER_INSPECTOR_KEY:
                    inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(c->webView));
                    webkit_web_inspector_show(inspector);
                    return TRUE;

                case SURFER_OPEN_KEY:
                   toggleopen_cb(c->box_open,c);
                   return TRUE;

		case SURFER_HISTORY_KEY:
		    recordhistory=FALSE;
                    loadurl(c,history);
                    return TRUE;

                case SURFER_NEW_WINDOW_KEY:
                    recordhistory=FALSE;
		    isrelated = FALSE;
                    rc = client_new(c);
                    loadurl(rc,home);
                    return TRUE;

                case SURFER_HOME_KEY:
                    recordhistory=FALSE;
                    loadurl(c,home);
                    return TRUE;

                case SURFER_RELOAD_KEY:
                    recordhistory=FALSE;
                    webkit_web_view_reload(c->webView);
                    return TRUE;

                case SURFER_FIND_KEY:
                    togglefind_cb(c);
                    return TRUE;

                case SURFER_BOOKMARK_KEY:
                    bookmark_cb(c);
                    return TRUE;

                case SURFER_ZOOM_OUT_KEY:
                    z = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(c->webView));
                    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(c->webView), z - 0.1);
                    return TRUE;

                case SURFER_ZOOM_IN_KEY:
                    z = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(c->webView));
                    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(c->webView), z + 0.1);
                    return TRUE;
		case SURFER_STYLE_KEY:
		    toggleuserstyle_cb(c);
                    return TRUE;

                default:
                    return FALSE;
            }
        } else {
            switch (key_pressed) {
                case SURFER_FULLSCREEN_KEY:
                    togglefullscreen_cb(c);
                    return TRUE;

                case SURFER_STOP_KEY:
                    webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(c->webView));
                    return TRUE;


                default:
                    return FALSE;
            }
        }
    }
    return FALSE;
}

void
openlink(GtkWidget * widget,Client *c){


   gchar *link;
   const gchar *p;

   p = gtk_entry_get_text(GTK_ENTRY(c->entry_open));
   link = g_strdup(p);
   loadurl(c, link);
   g_free(link);
   gtk_widget_hide(c->box_open);
}

void
find(GtkWidget * widget,Client *c) {

    static gchar *search_text;
    const gchar *p;


    p = gtk_entry_get_text(GTK_ENTRY(c->entry_find));
    search_text = g_strdup(p);

    if(search_text != NULL){

    webkit_find_controller_search(c->fc, search_text,WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE | WEBKIT_FIND_OPTIONS_WRAP_AROUND, G_MAXUINT);
    g_free(search_text);

    }

}

void
find_close(Client *c) {

   gtk_entry_set_text(GTK_ENTRY(c->entry_find), "");
 find(c->entry_find,c);
    gtk_widget_hide(c->box_find);
    gtk_widget_grab_focus(GTK_WIDGET(c->webView));

}


void
find_back(GtkWidget * widget,Client *c){


webkit_find_controller_search_previous(c->fc);


}


void
bookmark_cb(Client *c){
   FILE *File;
   char buffer[256] = "</body></html>";
   const gchar *tmp,*title;
   File = fopen(favpath, "a");

   tmp = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(c->webView));
   title =  webkit_web_view_get_title(WEBKIT_WEB_VIEW(c->webView));
   fprintf(File, "<a href=\"%s\" >%.110s</a><br>", (char *) tmp, (char *) title);
   fprintf(File, "%s\n", buffer);
   fclose(File);

}


void
goback(WebKitWebView *rv,Client *c){

  char *title;
  if (webkit_web_view_can_go_back(c->webView)){ 
   webkit_web_view_go_back(WEBKIT_WEB_VIEW(c->webView));
   recordhistory= FALSE;
   }
  else {
   title = g_strdup_printf(" %s", "Can't go back!");
   gtk_window_set_title(GTK_WINDOW(c->main_window), title);

  }
}



void
goforward(WebKitWebView *rv,Client *c){

  char *title;
 if (webkit_web_view_can_go_forward(c->webView)){ 
  webkit_web_view_go_forward(WEBKIT_WEB_VIEW(c->webView));
  recordhistory= FALSE;
  }
   else {
   title = g_strdup_printf(" %s", "Can't go forward!");
   gtk_window_set_title(GTK_WINDOW(c->main_window), title);

  }

}



void
toggleuserstyle_cb(Client *c){

	gchar *contents;

         if (!c->s) {
        g_file_get_contents(USER_STYLESHEET_FILENAME,&contents,NULL,NULL);
        webkit_user_content_manager_add_style_sheet(
	    webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(c->webView)),
	    webkit_user_style_sheet_new(contents,
	    WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
	    WEBKIT_USER_STYLE_LEVEL_USER,
	    NULL, NULL));

	g_free(contents);

          c->s = TRUE;
                    }
           else {
               webkit_user_content_manager_remove_all_style_sheets(
	    webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(c->webView)));
                 c->s = FALSE;
               }


}


void
togglejs_cb(GtkWidget * widget,Client *c){

   //GdkColor color;

   WebKitSettings *ssettings;

   ssettings=webkit_web_view_get_settings( WEBKIT_WEB_VIEW(c->webView));

   if (c->enablejs){

   g_object_set(G_OBJECT(ssettings),"enable-javascript", FALSE, NULL);
   c->enablejs=FALSE;

//  gtk_button_set_label(GTK_BUTTON(c->button_js), "JS-");

   }
   else{
  //gtk_button_set_label(GTK_BUTTON(c->button_js), "JS+");


   g_object_set(G_OBJECT(ssettings),"enable-javascript", TRUE, NULL);
   c->enablejs=TRUE;
   }
}
void togglefind_cb(Client *c)
{

    if (!c->f) {

                   gtk_widget_show_all(c->box_find);
                   gtk_widget_grab_focus(c->entry_find);

                    c->f = TRUE;
                   }
    else {

                   gtk_widget_hide(c->box_find);
                   gtk_widget_grab_focus(GTK_WIDGET(c->webView));
                   c->f = FALSE;
                   }


}

void toggleopen_cb(GtkWidget *widget,Client *c)
{
     const gchar *url;
    if (!c->o) {

                   gtk_widget_show_all(c->box_open);
                   gtk_widget_grab_focus(c->entry_open);
//                   url = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(c->webView));
  //                 gtk_entry_set_text(GTK_ENTRY(c->entry_open), url);
                    c->o = TRUE;
                   }
    else {

                   gtk_widget_hide(c->box_open);
                   gtk_widget_grab_focus(GTK_WIDGET(c->webView));
                   c->o = FALSE;
                   }


}


void
togglehistory_cb(GtkWidget *widget,Client *c)
{
   if (!enablehist){
      enablehist=TRUE;
    //  gtk_button_set_label(GTK_BUTTON(c->button_history), "HIST+");
   }
   else
   {

   enablehist=FALSE;
   //gtk_button_set_label(GTK_BUTTON(c->button_history), "HIST-");
   }


}

void togglefullscreen_cb(Client *c)
{

                    if (!c->fs ) {
                        gtk_window_fullscreen(GTK_WINDOW(c->main_window));
                        c->fs = TRUE;
                    } else {
                        gtk_window_unfullscreen(GTK_WINDOW(c->main_window));
                        c->fs = FALSE;
                    }

}

void
allow_tls_cert(Client *c)
{
  SoupURI *uri;
  gchar *url;
  gchar *tls_message;

  g_assert (G_IS_TLS_CERTIFICATE (c->certificate));
  g_assert (c->tls_error_failing_uri != NULL);

  uri = soup_uri_new (c->tls_error_failing_uri);
  webkit_web_context_allow_tls_certificate_for_host (webkit_web_view_get_context(c->webView),c->certificate,uri->host);

}

void remove_newline(char buffer[]) {
    size_t slen;

    slen = strlen(buffer);

    /* safe way to remove '\n' and check for bufferoverflow */
    if (slen > 0) {
        if (buffer[slen-1] == '\n') {
            buffer[slen-1] = '\0';
        } else {
            printf("Buffer overflow detected.\n");
            exit(EXIT_FAILURE);
        }
    }
}


GHashTable
*create_hash_table_from_file (gchar *tablepath)
{

    FILE *fp;

    char buf[1024];
    GHashTable *table;
    char *key;
    char *value;

    table = g_hash_table_new (g_str_hash, g_str_equal);

    fp = fopen (tablepath, "r");

    if (!fp)

    {
        exit (1);
    }

    while (fgets (buf, sizeof (buf), fp) !=NULL)
    {
	remove_newline(buf);
        key = strtok (buf, "=");
        if (!key) continue;
        value = strtok (NULL, "=");
        if (!value) continue;
            g_hash_table_insert (table, g_strdup (key), g_strdup (value));
    }




    fclose (fp);

    return table;

}



void
destroy_hash_table (GHashTable *table)
{

    g_hash_table_destroy (table);

}


gboolean setup(){



    gchar *downloadsfilename,*surferdirfilename;
    gchar *tablecsspath;

    FILE *File,*File1,*File2;
    char buffer[256] = "<!DOCTYPE html><html><head><title>Surfer</title><meta charset=utf8><style>body {background-color: #000009;}p\
    {color: yellow;} a:link { color: #00e900; } a:visited { color: green } a:clicked { color: red } </style></head><body><p>";

    char buffercss[40]= "example.com=/usr/share/surfer/black.css\n";


    downloadsfilename = g_strdup_printf("%s", SURFER_DOWNLOADS);

    downloads_dir = g_build_filename(getenv("HOME"), downloadsfilename, NULL);
    g_free(downloadsfilename);


    surferdirfilename = g_strdup_printf("%s", SURFER_DIR);

    surfer_dir = g_build_filename(getenv("HOME"), surferdirfilename, NULL);
    g_free(surferdirfilename);



  if (!g_file_test(downloads_dir, G_FILE_TEST_EXISTS)) {
        mkdir(downloads_dir, 0700);

    }

  if (!g_file_test(surfer_dir, G_FILE_TEST_EXISTS)) {
        mkdir(surfer_dir, 0700);

    }


    favpath = g_build_filename(surfer_dir,"fav", NULL);

    histpath = g_build_filename(surfer_dir, "hist", NULL);

    tablecsspath = g_build_filename(surfer_dir, "tablecss.txt", NULL);

    if (!g_file_test(favpath, G_FILE_TEST_EXISTS)) {
        File = fopen(favpath, "wb+");
        fprintf(File, "%s", buffer);
        fclose(File);

    }


  //if (HISTORY_ENABLE == 1)

    if (!g_file_test(histpath, G_FILE_TEST_EXISTS)) {
        File1 = fopen(histpath, "wb+");
        fprintf(File1, "%s", buffer);
        fclose(File1);

    }

   if (!g_file_test(tablecsspath, G_FILE_TEST_EXISTS)) {
        File2 = fopen(tablecsspath, "wb+");
        fprintf(File2, "%s", buffercss);
        fclose(File2);
   }

    history = (gchar *) g_strdup_printf("file://%s", histpath);

    home = (gchar *) g_strdup_printf("file://%s", favpath);

    tablecss = create_hash_table_from_file (tablecsspath);

    istmpdownload=FALSE;

return TRUE;
}


int main(int argc, char *argv[]) {

    Client *c;
    int i;
    FILE *File1;
    gchar *link;
    //char textdate[100];

    gtk_init(&argc, &argv);


    menu =gtk_menu_new();
    menuitem1 = gtk_menu_item_new_with_label("Click to cancel");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem1);
    //gtk_widget_show_all(menu);

    setup();
    c = client_new(NULL);

     if (argc > 1){

         for (i = 1; i < argc; i++) {
         link = (gchar *) argv[i];
         loadurl(c,link);
         }
     }

     else

     loadurl(c,home);



    gtk_main();

    return 0;
}
