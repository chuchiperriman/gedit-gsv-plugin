/*
 * docwordscompletion-plugin.c - Adds (auto)completion support to gedit
 *
 * Copyright (C) 2007 - chuchiperriman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "docwordscompletion-plugin.h"

#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gconf/gconf-client.h>
#include <gtksourcecompletion/gsc-completion.h>
#include "gsc-provider-words.h"

#define WINDOW_DATA_KEY	"DocwordscompletionPluginWindowData"

#define GCONF_BASE_KEY "/apps/gedit-2/plugins/docwordscompletion"
#define GCONF_AUTOCOMPLETION_ENABLED GCONF_BASE_KEY "/enable_autocompletion"
#define GCONF_OPEN_ENABLED GCONF_BASE_KEY "/enable_open_documents"
#define GCONF_RECENT_ENABLED GCONF_BASE_KEY "/enable_recent_documents"
#define GCONF_AUTOSELECT_ENABLED GCONF_BASE_KEY "/enable_autoselect_documents"
#define GCONF_AUTOCOMPLETION_DELAY GCONF_BASE_KEY "/autocompletion_delay"
#define GCONF_USER_REQUEST_EVENT_KEYS GCONF_BASE_KEY "/user_request_event_keys"
#define GCONF_OPEN_DOCUMENTS_EVENT_KEYS GCONF_BASE_KEY "/open_documents_event_keys"
#define GCONF_SHOW_INFO_KEYS GCONF_BASE_KEY "/show_info_keys"

#define DOCWORDSCOMPLETION_PLUGIN_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ((object), TYPE_DOCWORDSCOMPLETION_PLUGIN, DocwordscompletionPluginPrivate))

struct _ConfData
{
	gboolean ac_enabled;
	gboolean open_enabled;
	gboolean recent_enabled;
	gboolean autoselect_enabled;
	guint ac_delay;
	gchar* ure_keys;
	gchar* od_keys;
	gchar* si_keys;
};

typedef struct _ConfData ConfData;

struct _DocwordscompletionPluginPrivate
{
	GeditWindow *gedit_window;
	GtkWidget *window;
	GtkWidget *check_auto;
	GConfClient *gconf_cli;
	ConfData *conf;
};

typedef struct _ViewAndCompletion ViewAndCompletion;

GEDIT_PLUGIN_REGISTER_TYPE (DocwordscompletionPlugin, docwordscompletion_plugin)

static void
docwordscompletion_plugin_init (DocwordscompletionPlugin *plugin)
{
	plugin->priv = DOCWORDSCOMPLETION_PLUGIN_GET_PRIVATE (plugin);
	plugin->priv->gconf_cli = gconf_client_get_default ();
	plugin->priv->conf = g_malloc0(sizeof(ConfData));
	plugin->priv->conf->ac_enabled = TRUE;
	plugin->priv->conf->open_enabled = TRUE;
	plugin->priv->conf->recent_enabled = TRUE;
	plugin->priv->conf->ac_delay = 300;
	plugin->priv->conf->ure_keys = g_strdup("<Control>Return");
	plugin->priv->conf->od_keys = g_strdup("<Control>d");
	plugin->priv->conf->si_keys = g_strdup("<Control>i");
	/*TODO check if gconf is null*/
	GConfValue *value = gconf_client_get(plugin->priv->gconf_cli,GCONF_AUTOCOMPLETION_ENABLED,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->ac_enabled =  gconf_value_get_bool(value);
		gconf_value_free(value);
	}
	
	value = gconf_client_get(plugin->priv->gconf_cli,GCONF_OPEN_ENABLED,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->open_enabled =  gconf_value_get_bool(value);
		gconf_value_free(value);
	}
	
	value = gconf_client_get(plugin->priv->gconf_cli,GCONF_RECENT_ENABLED,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->recent_enabled =  gconf_value_get_bool(value);
		gconf_value_free(value);
	}
	
	value = gconf_client_get(plugin->priv->gconf_cli,GCONF_AUTOSELECT_ENABLED,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->autoselect_enabled =  gconf_value_get_bool(value);
		gconf_value_free(value);
	}

	value = gconf_client_get(plugin->priv->gconf_cli,GCONF_AUTOCOMPLETION_DELAY,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->ac_delay = gconf_value_get_int(value);
		gconf_value_free(value);
	}
	
	value = gconf_client_get(plugin->priv->gconf_cli,GCONF_USER_REQUEST_EVENT_KEYS,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->ure_keys = g_strdup(gconf_value_get_string(value));
		gconf_value_free(value);
	}
	
	value = gconf_client_get(plugin->priv->gconf_cli,GCONF_OPEN_DOCUMENTS_EVENT_KEYS,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->od_keys = g_strdup(gconf_value_get_string(value));
		gconf_value_free(value);
	}
	
	value = gconf_client_get(plugin->priv->gconf_cli,GCONF_SHOW_INFO_KEYS,NULL);
	if (value!=NULL)
	{
		plugin->priv->conf->si_keys = g_strdup(gconf_value_get_string(value));
		gconf_value_free(value);
	}

	gedit_debug_message (DEBUG_PLUGINS,
			     "DocwordscompletionPlugin initializing");
}

static void
docwordscompletion_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS,
			     "DocwordscompletionPlugin finalizing");
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)object;
	g_object_unref(dw_plugin->priv->gconf_cli);
	g_free(dw_plugin->priv->conf->ure_keys);
	g_free(dw_plugin->priv->conf->od_keys);
	g_free(dw_plugin->priv->conf->si_keys);
	g_free(dw_plugin->priv->conf);
	G_OBJECT_CLASS (docwordscompletion_plugin_parent_class)->finalize (object);
}

static void
tab_added_cb (GeditWindow *geditwindow,
              GeditTab    *tab,
              gpointer     user_data)
{
        GeditView *view = gedit_tab_get_view (tab);
        GscCompletion *comp = gsc_completion_new (GTK_TEXT_VIEW (view));
        g_debug ("Adding Words provider");
        GscProviderWords *dw  = gsc_provider_words_new();
        gsc_completion_add_provider(comp,GSC_PROVIDER(dw), NULL);
	
        g_object_unref(dw);
        g_debug ("provider registered");
}


static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)plugin;
	dw_plugin->priv->gedit_window = window;
	gedit_debug (DEBUG_PLUGINS);

	g_signal_connect (window, "tab-added",
                          G_CALLBACK (tab_added_cb),
                          NULL);


}

static void
impl_deactivate (GeditPlugin *plugin,
		 GeditWindow *window)
{
	gedit_debug (DEBUG_PLUGINS);
}

static void
impl_update_ui (GeditPlugin *plugin,
		GeditWindow *window)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)plugin;
	dw_plugin->priv->gedit_window = window;
	gedit_debug (DEBUG_PLUGINS);

}

/*************** Configuration window ****************/

static void
docwordscompletion_plugin_class_init (DocwordscompletionPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = docwordscompletion_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;

	g_type_class_add_private (object_class, 
				  sizeof (DocwordscompletionPluginPrivate));
}
