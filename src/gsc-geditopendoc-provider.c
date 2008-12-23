 /* gsc-geditopendoc-provider.c
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
#include <gtksourcecompletion/gsc-proposal.h>
#include "gsc-geditopendoc-provider.h"
#include "gsc-proposal-open.h"

struct _GscGeditopendocProviderPrivate {
	GeditWindow *window;
	GdkPixbuf *icon;
};

#define GSC_GEDITOPENDOC_PROVIDER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_GSC_GEDITOPENDOC_PROVIDER, GscGeditopendocProviderPrivate))

static const gchar* gsc_geditopendoc_provider_real_get_name (GscProvider* self);
static GList* gsc_geditopendoc_provider_real_get_proposals (GscProvider* base, GscTrigger *trigger);
static void gsc_geditopendoc_provider_real_end_completion (GscProvider* base);
static gchar* gsc_geditopendoc_provider_real_get_item_info_markup (GscProvider *self, GscProposal *item);
static gpointer gsc_geditopendoc_provider_parent_class = NULL;
static GscProviderIface* gsc_geditopendoc_provider_parent_iface = NULL;

static const gchar* gsc_geditopendoc_provider_real_get_name (GscProvider* self)
{
	return GSC_GEDITOPENDOC_PROVIDER_NAME;
}

static GList* gsc_geditopendoc_provider_real_get_proposals (GscProvider* base, GscTrigger *trigger)
{

	GList *item_list = NULL;
	GList *list,*wins,*temp;
	GeditDocument *doc, *current_doc;
	GscProposal *item;
	gchar* name;
	GscGeditopendocProvider *self = GSC_GEDITOPENDOC_PROVIDER (base);
	wins = gedit_window_get_documents(self->priv->window);
	current_doc = gedit_window_get_active_document(self->priv->window);
	temp = wins;
	while(temp!=NULL)
	{
		doc = GEDIT_DOCUMENT(temp->data);
		if (doc != current_doc)
		{
			item = gsc_proposal_open_new (self->priv->window,
						      doc,
						      self->priv->icon);
			item_list = g_list_append(item_list,item);
			
		}
		temp = g_list_next(temp);
	}

	g_list_free(wins);
	return item_list;
}

static void gsc_geditopendoc_provider_real_end_completion (GscProvider* base)
{

}

static void gsc_geditopendoc_provider_real_data_free (GscProvider* self, GscProposal* data)
{
	g_debug("gedit open data free");
}

static void gsc_geditopendoc_provider_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
}


static void gsc_geditopendoc_provider_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
}

static void gsc_geditopendoc_provider_finalize(GObject *object)
{
	GscGeditopendocProvider *self;
	
	self = GSC_GEDITOPENDOC_PROVIDER(object);
	
	g_object_unref(self->priv->icon);
	
	G_OBJECT_CLASS(gsc_geditopendoc_provider_parent_class)->finalize(object);
}


static void gsc_geditopendoc_provider_class_init (GscGeditopendocProviderClass * klass)
{
	gsc_geditopendoc_provider_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->get_property = gsc_geditopendoc_provider_get_property;
	G_OBJECT_CLASS (klass)->set_property = gsc_geditopendoc_provider_set_property;
	G_OBJECT_CLASS (klass)->finalize = gsc_geditopendoc_provider_finalize;
}


static void gsc_geditopendoc_provider_interface_init (GscProviderIface * iface)
{
	gsc_geditopendoc_provider_parent_iface = g_type_interface_peek_parent (iface);
	iface->get_proposals = gsc_geditopendoc_provider_real_get_proposals;
	iface->get_name = gsc_geditopendoc_provider_real_get_name;
	iface->finish = gsc_geditopendoc_provider_real_end_completion;
}


static void gsc_geditopendoc_provider_init (GscGeditopendocProvider * self)
{
	self->priv = g_new0(GscGeditopendocProviderPrivate, 1);
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	self->priv->icon = gtk_icon_theme_load_icon(theme,GTK_STOCK_FILE,16,0,NULL);
}

GType gsc_geditopendoc_provider_get_type ()
{
	static GType g_define_type_id = 0;
	if (G_UNLIKELY (g_define_type_id == 0)) {
		static const GTypeInfo g_define_type_info = { sizeof (GscGeditopendocProviderClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gsc_geditopendoc_provider_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GscGeditopendocProvider), 0, (GInstanceInitFunc) gsc_geditopendoc_provider_init };
		g_define_type_id = g_type_register_static (G_TYPE_OBJECT, "GscGeditopendocProvider", &g_define_type_info, 0);
		static const GInterfaceInfo gsc_manager_provider_info = { (GInterfaceInitFunc) gsc_geditopendoc_provider_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		g_type_add_interface_static (g_define_type_id, GSC_TYPE_PROVIDER, &gsc_manager_provider_info);
	}
	return g_define_type_id;
}


GscGeditopendocProvider*
gsc_geditopendoc_provider_new(GeditWindow *window)
{
	GscGeditopendocProvider *self = GSC_GEDITOPENDOC_PROVIDER (g_object_new (TYPE_GSC_GEDITOPENDOC_PROVIDER, NULL));
	self->priv->window = window;
	return self;
}

