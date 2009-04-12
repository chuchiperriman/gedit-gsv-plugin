/*
 * gwp-plugin.c - Adds (auto)completion support to gedit
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

#include "gwp-plugin.h"

#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gconf/gconf-client.h>
#include <gtksourceview/gtksourcecompletiontrigger.h>
#include <gtksourceview/gtksourcecompletiontriggerkey.h>
#include <gtksourceview/gtksourcecompletiontriggerwords.h>
#include "gwp-provider-words.h"

#define GWP_PLUGIN_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GWP_TYPE_PLUGIN, GwpPluginPrivate))

#define TEMP_TRIGGER_NAME "UserRequestTrigger"

struct _GwpPluginPrivate
{
	GeditWindow *gedit_window;
	GtkWidget *window;
};

typedef struct _ViewAndCompletion ViewAndCompletion;

GEDIT_PLUGIN_REGISTER_TYPE (GwpPlugin, gwp_plugin)

static void
gwp_plugin_init (GwpPlugin *plugin)
{
	plugin->priv = GWP_PLUGIN_GET_PRIVATE (plugin);
	gedit_debug_message (DEBUG_PLUGINS,
			     "GwpPlugin initializing");
}

static void
gwp_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS,
			     "GwpPlugin finalizing");
	G_OBJECT_CLASS (gwp_plugin_parent_class)->finalize (object);
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GwpPlugin * dw_plugin = (GwpPlugin*)plugin;
	dw_plugin->priv->gedit_window = window;
	gedit_debug (DEBUG_PLUGINS);

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
	GwpPlugin * dw_plugin = (GwpPlugin*)plugin;
	GtkSourceCompletionTrigger *ur_trigger, *ac_trigger;
	dw_plugin->priv->gedit_window = window;
	gedit_debug (DEBUG_PLUGINS);
	GtkSourceView* view = GTK_SOURCE_VIEW(gedit_window_get_active_view(window));
	if (view!=NULL)
	{
		GtkSourceCompletion *comp = gtk_source_view_get_completion (view);

		if (gtk_source_completion_get_provider(comp,GWP_PROVIDER_WORDS_NAME)==NULL)
		{
			g_debug ("Adding wordsprovider");
			GwpProviderWords *dw  = gwp_provider_words_new(view);
			
			//TODO get the default user request trigger
			ur_trigger = gtk_source_completion_get_trigger(comp, TEMP_TRIGGER_NAME);
			if (ur_trigger==NULL)
			{
				ur_trigger = GTK_SOURCE_COMPLETION_TRIGGER(gtk_source_completion_trigger_key_new (comp, TEMP_TRIGGER_NAME));
				gtk_source_completion_add_trigger(comp,ur_trigger);
				g_object_unref(ur_trigger);
				g_debug ("added user request trigger");
			}
			
			gtk_source_completion_add_provider(comp,GTK_SOURCE_COMPLETION_PROVIDER(dw),ur_trigger);
			
			ac_trigger = gtk_source_completion_get_trigger(comp, GTK_SOURCE_COMPLETION_TRIGGER_WORDS_NAME);
			if (ac_trigger==NULL)
			{
				ac_trigger = GTK_SOURCE_COMPLETION_TRIGGER (gtk_source_completion_trigger_words_new(comp));
				gtk_source_completion_add_trigger(comp,GTK_SOURCE_COMPLETION_TRIGGER(ac_trigger));
				gtk_source_completion_trigger_words_set_delay(GTK_SOURCE_COMPLETION_TRIGGER_WORDS (ac_trigger),300);
				g_object_unref(ac_trigger);
				g_debug ("added words trigger");
			}
			gtk_source_completion_add_provider(comp,GTK_SOURCE_COMPLETION_PROVIDER(dw),ac_trigger);
			
			g_object_unref(dw);
			g_debug ("provider registered");
		}

	}
}

static void
gwp_plugin_class_init (GwpPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gwp_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;

	g_type_class_add_private (object_class, 
				  sizeof (GwpPluginPrivate));
}

