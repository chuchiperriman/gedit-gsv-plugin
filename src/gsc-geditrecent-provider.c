 /* gsc-geditrecent-provider.c - Type here a short description of your plugin
 *
 * Copyright (C) 2008 - perriman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <glib/gprintf.h>
#include <string.h>
#include "gsc-geditrecent-provider.h"

#define ICON_FILE ICON_DIR"/locals.png"

struct _GscGeditrecentProviderPrivate {
	GeditWindow *window;
	GdkPixbuf *icon;
};

typedef struct {
	GscGeditrecentProvider *provider;
	gchar* uri;
}RecentData;

#define GSC_GEDITRECENT_PROVIDER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_GSC_GEDITRECENT_PROVIDER, GscGeditrecentProviderPrivate))

enum  {
	GSC_GEDITRECENT_PROVIDER_DUMMY_PROPERTY,
};
static const gchar* gsc_geditrecent_provider_real_get_name (GscProvider* self);
static GList* gsc_geditrecent_provider_real_get_proposals (GscProvider* base, GscTrigger *trigger);
static void gsc_geditrecent_provider_real_end_completion (GscProvider* base);
static gpointer gsc_geditrecent_provider_parent_class = NULL;
static GscProviderIface* gsc_geditrecent_provider_gsc_manager_provider_parent_iface = NULL;


static const gchar* gsc_geditrecent_provider_real_get_name (GscProvider* self)
{
	return GSC_GEDITRECENT_PROVIDER_NAME;
}

static gboolean
_apply_cd(GscProposal* proposal, GtkTextView *view, gpointer user_data)
{
	RecentData *rd = (RecentData*)user_data;
	GscGeditrecentProvider *self = rd->provider;
	gedit_commands_load_uri(self->priv->window,
				rd->uri,
				NULL,
				1);
	return TRUE;
}

static void
_apply_closure_cb (gpointer data,GClosure *closure)
{
	RecentData *rd = (RecentData*)data;
	g_free(rd->uri);
	g_free(rd);
}

static gint
sort_recents_mru (GtkRecentInfo *a, GtkRecentInfo *b)
{
        return (gtk_recent_info_get_modified (b) - gtk_recent_info_get_modified (a));
}

static GList* gsc_geditrecent_provider_real_get_proposals (GscProvider* base, GscTrigger *trigger)
{
	GscGeditrecentProvider *self = GSC_GEDITRECENT_PROVIDER(base);
	GscProposal *item;
	GList *item_list = NULL;
	gint max_recent = 10;
	GtkRecentManager *recent_manager =  gtk_recent_manager_get_default ();
	GList *items = gtk_recent_manager_get_items (recent_manager);
	GList *filtered_items = NULL, *l;
	const gchar *display_name;
	/* filter */
        for (l = items; l != NULL; l = l->next)
        {
                GtkRecentInfo *info = l->data;
                if (!gtk_recent_info_has_group (info, "gedit"))
                        continue;
                filtered_items = g_list_prepend (filtered_items, info);
        }

        /* sort */
        filtered_items = g_list_sort (filtered_items,
                                      (GCompareFunc) sort_recents_mru);
	gint i = 0;
        for (l = filtered_items; l != NULL; l = l->next)
        {
        	GtkRecentInfo *info = l->data;
        	const gchar *uri;
        	display_name = gtk_recent_info_get_display_name (info);
        	uri = gtk_recent_info_get_uri (info);
        	item = gsc_proposal_new(display_name,
        						  uri,
							  self->priv->icon);
		gsc_proposal_set_page_name(item,"Recent Files");
		
		RecentData *data = g_new0(RecentData,1);
		data->provider = self;
		data->uri = g_strdup(uri);
		g_signal_connect_data(item, 
				 "apply",
				 G_CALLBACK(_apply_cd),
				 (gpointer)data,
				 _apply_closure_cb,
				 0);
		/*TODO How do I free this uri??????*/
		item_list = g_list_append(item_list,item);
        	++i;
        	if (i>=max_recent)
        		break;
        }
        g_list_free(filtered_items);
        g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
        g_list_free (items);
        
	return item_list;
}

static void gsc_geditrecent_provider_real_end_completion (GscProvider* base)
{
}

static void gsc_geditrecent_provider_real_data_free (GscProvider* self, GscProposal* data)
{
	
}

static void gsc_geditrecent_provider_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
}


static void gsc_geditrecent_provider_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
}

static void gsc_geditrecent_provider_finalize(GObject *object)
{
	GscGeditrecentProvider *self;
	
	self = GSC_GEDITRECENT_PROVIDER(object);
	
	g_object_unref(self->priv->icon);
	
	G_OBJECT_CLASS(gsc_geditrecent_provider_parent_class)->finalize(object);
}


static void gsc_geditrecent_provider_class_init (GscGeditrecentProviderClass * klass)
{
	gsc_geditrecent_provider_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->get_property = gsc_geditrecent_provider_get_property;
	G_OBJECT_CLASS (klass)->set_property = gsc_geditrecent_provider_set_property;
	G_OBJECT_CLASS (klass)->finalize = gsc_geditrecent_provider_finalize;
}


static void gsc_geditrecent_provider_gsc_manager_provider_interface_init (GscProviderIface * iface)
{
	gsc_geditrecent_provider_gsc_manager_provider_parent_iface = g_type_interface_peek_parent (iface);
	iface->get_proposals = gsc_geditrecent_provider_real_get_proposals;
	iface->get_name = gsc_geditrecent_provider_real_get_name;
	iface->finish = gsc_geditrecent_provider_real_end_completion;
}


static void gsc_geditrecent_provider_init (GscGeditrecentProvider * self)
{
	self->priv = g_new0(GscGeditrecentProviderPrivate, 1);
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	self->priv->icon = gtk_icon_theme_load_icon(theme,GTK_STOCK_FILE,16,0,NULL);
}

GType gsc_geditrecent_provider_get_type ()
{
	static GType g_define_type_id = 0;
	if (G_UNLIKELY (g_define_type_id == 0)) {
		static const GTypeInfo g_define_type_info = { sizeof (GscGeditrecentProviderClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gsc_geditrecent_provider_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GscGeditrecentProvider), 0, (GInstanceInitFunc) gsc_geditrecent_provider_init };
		g_define_type_id = g_type_register_static (G_TYPE_OBJECT, "GscGeditrecentProvider", &g_define_type_info, 0);
		static const GInterfaceInfo gsc_manager_provider_info = { (GInterfaceInitFunc) gsc_geditrecent_provider_gsc_manager_provider_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		g_type_add_interface_static (g_define_type_id, GSC_TYPE_PROVIDER, &gsc_manager_provider_info);
	}
	return g_define_type_id;
}


GscGeditrecentProvider*
gsc_geditrecent_provider_new(GeditWindow *window)
{
	GscGeditrecentProvider *self = GSC_GEDITRECENT_PROVIDER (g_object_new (TYPE_GSC_GEDITRECENT_PROVIDER, NULL));
	self->priv->window = window;
	return self;
}

