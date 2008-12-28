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
#include <gtksourcecompletion/gsc-trigger.h>
#include <gtksourcecompletion/gsc-trigger-customkey.h>
#include <gtksourcecompletion/gsc-trigger-autowords.h>
#include <gtksourcecompletion/gsc-documentwords-provider.h>

#include "gsc-geditopendoc-provider.h"
#include "gsc-geditrecent-provider.h"

#define OPEN_DOCS_TRIGGER_NAME "Open Documents Trigger"
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
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)plugin;
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
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)plugin;
	dw_plugin->priv->gedit_window = window;
	gedit_debug (DEBUG_PLUGINS);
	GtkTextView* view = GTK_TEXT_VIEW(gedit_window_get_active_view(window));
	if (view!=NULL)
	{
		GscManager *comp = gsc_manager_get_from_view(view);
		if (comp==NULL)
		{
			comp = gsc_manager_new(GTK_TEXT_VIEW(view));
			GValue value = {0,};
			g_value_init(&value,G_TYPE_STRING);
			g_value_set_string(&value,dw_plugin->priv->conf->si_keys);
			g_object_set (comp,
				      "info-keys",
				      dw_plugin->priv->conf->si_keys,
				      NULL);
		}

		if (gsc_manager_get_provider(comp,GSC_DOCUMENTWORDS_PROVIDER_NAME)==NULL)
		{
			
			GscTriggerCustomkey *ck_trigger = gsc_trigger_customkey_new(comp,
					OPEN_DOCS_TRIGGER_NAME, 
					dw_plugin->priv->conf->od_keys);
			gsc_manager_register_trigger(comp,GSC_TRIGGER(ck_trigger));
			g_object_unref(ck_trigger);
			
			/*FIXME: Create a funtion "update_conf" and we must to 
			 call it here and when the user changes the configuration.
			*/
							
			if (dw_plugin->priv->conf->ac_enabled)
			{
				GscTrigger *ur_trigger = 
					gsc_manager_get_trigger(comp,USER_REQUEST_TRIGGER_NAME);
				if (ur_trigger==NULL)
				{
					ur_trigger = GSC_TRIGGER(gsc_trigger_customkey_new(
							comp,
							USER_REQUEST_TRIGGER_NAME,
							dw_plugin->priv->conf->ure_keys));
					gsc_manager_register_trigger(comp,ur_trigger);
					g_object_unref(ur_trigger);
				}
				GscTriggerAutowords *ac_trigger = gsc_trigger_autowords_new(comp);
				gsc_manager_register_trigger(comp,GSC_TRIGGER(ac_trigger));
				g_object_unref(ac_trigger);
				gsc_trigger_autowords_set_delay(ac_trigger,dw_plugin->priv->conf->ac_delay);
				GscDocumentwordsProvider *dw  = gsc_documentwords_provider_new(view);
				gsc_manager_register_provider(comp,GSC_PROVIDER(dw),GSC_TRIGGER_AUTOWORDS_NAME);
				gsc_manager_register_provider(comp,GSC_PROVIDER(dw),USER_REQUEST_TRIGGER_NAME);
				g_object_unref(dw);
			}
			
			
			if (dw_plugin->priv->conf->open_enabled)
			{
				GscGeditopendocProvider* od = gsc_geditopendoc_provider_new(window);
				gsc_manager_register_provider(comp,GSC_PROVIDER(od),OPEN_DOCS_TRIGGER_NAME);
				g_object_unref(od);
			}
			
			if (dw_plugin->priv->conf->recent_enabled)
			{
				GscGeditrecentProvider *grp = gsc_geditrecent_provider_new(window);
				gsc_manager_register_provider(comp,GSC_PROVIDER(grp),OPEN_DOCS_TRIGGER_NAME);
				g_object_unref(grp);
			}
			
			g_object_set (comp,
				      "autoselect", dw_plugin->priv->conf->autoselect_enabled,
				      NULL);
			
			gsc_manager_activate(comp);
		}

	}
}

/*************** Configuration window ****************/

static gboolean delete_event( GtkWidget *widget,
								GdkEvent  *event,
								gpointer   data )
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)data;
	gtk_widget_hide(dw_plugin->priv->window);
	return FALSE;
}

static void
autocompletion_enabled_toggled(GtkWidget *button,
				gpointer user_data)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)user_data;
	gboolean active =	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	gconf_client_set_bool(dw_plugin->priv->gconf_cli,
				GCONF_AUTOCOMPLETION_ENABLED,
				active,
				NULL);
	dw_plugin->priv->conf->ac_enabled = active;

	GList *windows = gedit_window_get_views(dw_plugin->priv->gedit_window);
	while (windows!=NULL)
	{
		GscManager *comp = gsc_manager_get_from_view(GTK_TEXT_VIEW(windows->data));
		if (comp!=NULL)
		{
			GscTriggerAutowords * ac_trigger = GSC_TRIGGER_AUTOWORDS(
					gsc_manager_get_trigger(comp,GSC_TRIGGER_AUTOWORDS_NAME));
			if (ac_trigger!=NULL && !active)
			{
				gsc_manager_unregister_trigger(comp,GSC_TRIGGER(ac_trigger));
			}
		
			if (ac_trigger==NULL && active)
			{
				ac_trigger = gsc_trigger_autowords_new(comp);
				gsc_manager_register_trigger(comp,GSC_TRIGGER(ac_trigger));
				g_object_unref(ac_trigger);
				gsc_trigger_autowords_set_delay(ac_trigger,dw_plugin->priv->conf->ac_delay);
			}
		}
		windows = g_list_next(windows);
	}
}

static void
open_enabled_toggled(GtkWidget *button,
			gpointer user_data)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)user_data;
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	gconf_client_set_bool(dw_plugin->priv->gconf_cli,
				GCONF_OPEN_ENABLED,
				active,
				NULL);
	
	dw_plugin->priv->conf->open_enabled = active;

	GList *windows = gedit_window_get_views(dw_plugin->priv->gedit_window);
	while (windows!=NULL)
	{
		GscManager *comp = gsc_manager_get_from_view(GTK_TEXT_VIEW(windows->data));
		if (comp!=NULL)
		{
			GscGeditopendocProvider * prov = GSC_GEDITOPENDOC_PROVIDER(
					gsc_manager_get_provider(comp,GSC_GEDITOPENDOC_PROVIDER_NAME));
			if (prov!=NULL && !active)
			{
				gsc_manager_unregister_provider(comp,GSC_PROVIDER(prov),OPEN_DOCS_TRIGGER_NAME);
			}
			
			if (prov==NULL && active)
			{
				prov = gsc_geditopendoc_provider_new(dw_plugin->priv->gedit_window);
				gsc_manager_register_provider(comp,GSC_PROVIDER(prov),OPEN_DOCS_TRIGGER_NAME);
				g_object_unref(prov);
			}
		}
		windows = g_list_next(windows);
	}
}

static void
recent_enabled_toggled(GtkWidget *button,
			gpointer user_data)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)user_data;
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	gconf_client_set_bool(dw_plugin->priv->gconf_cli,
				GCONF_RECENT_ENABLED,
				active,
				NULL);
	
	dw_plugin->priv->conf->recent_enabled = active;

	GList *windows = gedit_window_get_views(dw_plugin->priv->gedit_window);
	while (windows!=NULL)
	{
		GscManager *comp = gsc_manager_get_from_view(GTK_TEXT_VIEW(windows->data));
		if (comp!=NULL)
		{
			GscGeditrecentProvider * prov = GSC_GEDITRECENT_PROVIDER(
					gsc_manager_get_provider(comp,GSC_GEDITRECENT_PROVIDER_NAME));
			if (prov!=NULL && !active)
			{
				gsc_manager_unregister_provider(comp,GSC_PROVIDER(prov),OPEN_DOCS_TRIGGER_NAME);
			}
			
			if (prov==NULL && active)
			{
				prov = gsc_geditrecent_provider_new(dw_plugin->priv->gedit_window);
				gsc_manager_register_provider(comp,GSC_PROVIDER(prov),OPEN_DOCS_TRIGGER_NAME);
				g_object_unref(prov);
			}
		}
		windows = g_list_next(windows);
	}
}

static void
autoselect_enabled_toggled(GtkWidget *button,
			   gpointer user_data)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)user_data;
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	gconf_client_set_bool(dw_plugin->priv->gconf_cli,
				GCONF_AUTOSELECT_ENABLED,
				active,
				NULL);
	
	dw_plugin->priv->conf->autoselect_enabled = active;

	GList *windows = gedit_window_get_views(dw_plugin->priv->gedit_window);
	while (windows!=NULL)
	{
		GscManager *comp = gsc_manager_get_from_view(GTK_TEXT_VIEW(windows->data));
		if (comp!=NULL)
		{
			g_object_set (comp,
				      "autoselect", active,
				      NULL);
		}
		windows = g_list_next(windows);
	}
}

static void
apply_configuration(GtkWidget *button,
								gpointer user_data)
{
	/*Save configuration and exit*/
	delete_event(button,NULL,user_data);
}

static gboolean
delay_changed(GtkWidget *widget,
								gpointer user_data)
{
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)user_data;
	guint delay = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	gconf_client_set_int(dw_plugin->priv->gconf_cli,
				GCONF_AUTOCOMPLETION_DELAY,
				delay,
				NULL);
	dw_plugin->priv->conf->ac_delay = delay;
	GList *windows = gedit_window_get_views(dw_plugin->priv->gedit_window);
	while (windows!=NULL)
	{
			GscManager *comp = gsc_manager_get_from_view(GTK_TEXT_VIEW(windows->data));
			if (comp!=NULL)
			{
				GscTriggerAutowords *ac_trigger = GSC_TRIGGER_AUTOWORDS(
						gsc_manager_get_trigger(comp,GSC_TRIGGER_AUTOWORDS_NAME));
				if (ac_trigger!=NULL)
				{
					gsc_trigger_autowords_set_delay(ac_trigger,dw_plugin->priv->conf->ac_delay);
				}
			}
			windows = g_list_next(windows);
	}
	return FALSE;
}

static void
_refresh_keys(DocwordscompletionPlugin *dw_plugin)
{
	GList *windows = gedit_window_get_views(dw_plugin->priv->gedit_window);
	while (windows!=NULL)
	{
			GscManager *comp = gsc_manager_get_from_view(GTK_TEXT_VIEW(windows->data));
			if (comp!=NULL)
			{
				GscTrigger *ur_trigger = 
						gsc_manager_get_trigger(comp,USER_REQUEST_TRIGGER_NAME);

				if (ur_trigger!=NULL && GSC_IS_TRIGGER_CUSTOMKEY(ur_trigger))
					gsc_trigger_customkey_set_keys(GSC_TRIGGER_CUSTOMKEY(ur_trigger),
						dw_plugin->priv->conf->ure_keys);
					
				GscTriggerCustomkey *od_trigger = GSC_TRIGGER_CUSTOMKEY(
						gsc_manager_get_trigger(comp,OPEN_DOCS_TRIGGER_NAME));
				if (od_trigger!=NULL)
					gsc_trigger_customkey_set_keys(od_trigger,
						dw_plugin->priv->conf->od_keys);
				
				GValue value = {0,};
				g_value_init(&value,G_TYPE_STRING);
				g_value_set_string(&value,dw_plugin->priv->conf->si_keys);
				g_object_set_property(G_OBJECT(comp),
							"info-keys",
							&value);
				
					
			}
			windows = g_list_next(windows);
	}
}

static gboolean
ure_keys_key_release_cb (GtkWidget *view,
			GdkEventKey *event, 
			gpointer user_data)
{
	DocwordscompletionPlugin *dw_plugin = (DocwordscompletionPlugin*)user_data;
	
	gchar* teclas = gtk_accelerator_name(event->keyval,event->state);
	gtk_entry_set_text(GTK_ENTRY(view),teclas);
	gconf_client_set_string(dw_plugin->priv->gconf_cli,
				GCONF_USER_REQUEST_EVENT_KEYS,
				teclas,
				NULL);
	dw_plugin->priv->conf->ure_keys = teclas;
	
	_refresh_keys(dw_plugin);
	
	return TRUE;
}

static gboolean
od_keys_key_release_cb (GtkWidget *view,
			GdkEventKey *event, 
			gpointer user_data)
{
	DocwordscompletionPlugin *dw_plugin = (DocwordscompletionPlugin*)user_data;
	
	gchar* teclas = gtk_accelerator_name(event->keyval,event->state);
	gtk_entry_set_text(GTK_ENTRY(view),teclas);
	gconf_client_set_string(dw_plugin->priv->gconf_cli,
				GCONF_OPEN_DOCUMENTS_EVENT_KEYS,
				teclas,
				NULL);
	dw_plugin->priv->conf->od_keys = teclas;
	
	_refresh_keys(dw_plugin);
	
	return TRUE;
}

static gboolean
si_keys_key_release_cb (GtkWidget *view,
			GdkEventKey *event, 
			gpointer user_data)
{
	DocwordscompletionPlugin *dw_plugin = (DocwordscompletionPlugin*)user_data;
	
	gchar* teclas = gtk_accelerator_name(event->keyval,event->state);
	gtk_entry_set_text(GTK_ENTRY(view),teclas);
	gconf_client_set_string(dw_plugin->priv->gconf_cli,
				GCONF_SHOW_INFO_KEYS,
				teclas,
				NULL);
	dw_plugin->priv->conf->si_keys = teclas;
	
	_refresh_keys(dw_plugin);
	
	return TRUE;
}

static GtkWidget*
create_configure_dialog(GeditPlugin *plugin)
{
	
	DocwordscompletionPlugin * dw_plugin = (DocwordscompletionPlugin*)plugin;
	dw_plugin->priv->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (dw_plugin->priv->window), "delete_event",
			G_CALLBACK (delete_event), dw_plugin);

	GtkWidget *table = gtk_table_new (8, 2, TRUE);
	gtk_container_add (GTK_CONTAINER (dw_plugin->priv->window), table);
	
	GtkWidget *check_auto = gtk_check_button_new_with_label("Enable autocompletion");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_auto),dw_plugin->priv->conf->ac_enabled);
	gtk_table_attach_defaults (GTK_TABLE (table), check_auto, 0, 2, 0, 1);
	gtk_widget_show (check_auto);
	g_signal_connect (G_OBJECT (check_auto), "toggled",
			G_CALLBACK (autocompletion_enabled_toggled), dw_plugin);

	GtkWidget *label = gtk_label_new("Autocompletion delay: ");
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	gtk_widget_show (label);
	
	/* creates the spinner, with no decimal places */
	GtkAdjustment *spinner_adj;
	spinner_adj = (GtkAdjustment *) gtk_adjustment_new (50.0, 100.0, 2000.0, 50.0, 100.0, 5.0);

	GtkWidget *delay = gtk_spin_button_new (spinner_adj, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(delay),dw_plugin->priv->conf->ac_delay);
	gtk_table_attach_defaults (GTK_TABLE (table), delay, 1, 2, 1, 2);
	gtk_widget_show (delay);
	
	g_signal_connect (G_OBJECT (delay), "value-changed",
			G_CALLBACK (delay_changed), dw_plugin);
									
	GtkWidget *label_ure = gtk_label_new("User Request trigger keys: ");
	gtk_table_attach_defaults (GTK_TABLE (table), label_ure, 0, 1, 2, 3);
	gtk_widget_show (label_ure);
	
	GtkWidget *ure_keys = gtk_entry_new ();
	gtk_entry_set_text(GTK_ENTRY(ure_keys),dw_plugin->priv->conf->ure_keys);
	gtk_table_attach_defaults (GTK_TABLE (table), ure_keys, 1, 2, 2, 3);
	gtk_widget_show (ure_keys);
	
	g_signal_connect (G_OBJECT (ure_keys), "key-press-event",
			G_CALLBACK (ure_keys_key_release_cb), dw_plugin);
			
	GtkWidget *check_open = gtk_check_button_new_with_label("Enable Open Documents");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_open),dw_plugin->priv->conf->open_enabled);
	gtk_table_attach_defaults (GTK_TABLE (table), check_open, 0, 2, 3, 4);
	gtk_widget_show (check_open);
	g_signal_connect (G_OBJECT (check_open), "toggled",
			G_CALLBACK (open_enabled_toggled), dw_plugin);
	
	GtkWidget *check_recent = gtk_check_button_new_with_label("Enable Recent Documents");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_recent),dw_plugin->priv->conf->recent_enabled);
	gtk_table_attach_defaults (GTK_TABLE (table), check_recent, 0, 2, 4, 5);
	gtk_widget_show (check_recent);
	g_signal_connect (G_OBJECT (check_recent), "toggled",
			G_CALLBACK (recent_enabled_toggled), dw_plugin);
	
	
	GtkWidget *label_od = gtk_label_new("Open/Recent Documents trigger keys: ");
	gtk_table_attach_defaults (GTK_TABLE (table), label_od, 0, 1, 5, 6);
	gtk_widget_show (label_od);
	
	GtkWidget *od_keys = gtk_entry_new ();
	gtk_entry_set_text(GTK_ENTRY(od_keys),dw_plugin->priv->conf->od_keys);
	gtk_table_attach_defaults (GTK_TABLE (table), od_keys, 1, 2, 5, 6);
	gtk_widget_show (od_keys);
	
	g_signal_connect (G_OBJECT (od_keys), "key-press-event",
			G_CALLBACK (od_keys_key_release_cb), dw_plugin);
									
	GtkWidget *label_si = gtk_label_new("Show Info keys: ");
	gtk_table_attach_defaults (GTK_TABLE (table), label_si, 0, 1, 6, 7);
	gtk_widget_show (label_si);
	
	GtkWidget *si_keys = gtk_entry_new ();
	gtk_entry_set_text(GTK_ENTRY(si_keys),dw_plugin->priv->conf->si_keys);
	gtk_table_attach_defaults (GTK_TABLE (table), si_keys, 1, 2, 6, 7);
	gtk_widget_show (si_keys);
	
	g_signal_connect (G_OBJECT (si_keys), "key-press-event",
			G_CALLBACK (si_keys_key_release_cb), dw_plugin);

	GtkWidget *check_autoselect = gtk_check_button_new_with_label("Enable autoselect proposal");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_autoselect),dw_plugin->priv->conf->autoselect_enabled);
	gtk_table_attach_defaults (GTK_TABLE (table), check_autoselect, 0, 2, 7, 8);
	gtk_widget_show (check_autoselect);
	g_signal_connect (G_OBJECT (check_autoselect), "toggled",
			G_CALLBACK (autoselect_enabled_toggled), dw_plugin);
	
	
	GtkWidget *close = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_table_attach_defaults (GTK_TABLE (table), close, 1, 2, 8, 9);
	gtk_widget_show (close);
	g_signal_connect (G_OBJECT (close), "clicked",
			G_CALLBACK (apply_configuration), dw_plugin);
	
	gtk_widget_show (table);

	return dw_plugin->priv->window;
}

static void
docwordscompletion_plugin_class_init (DocwordscompletionPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = docwordscompletion_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
	plugin_class->create_configure_dialog = create_configure_dialog;

	g_type_class_add_private (object_class, 
				  sizeof (DocwordscompletionPluginPrivate));
}
