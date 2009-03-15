 /* gsc-proposal-recent.c
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

#include "gsc-proposal-recent.h"

struct _GscProposalRecentPrivate
{
	GeditWindow *window;
	gchar *uri;
};

G_DEFINE_TYPE(GscProposalRecent, gsc_proposal_recent, GSC_TYPE_PROPOSAL);

#define GSC_PROPOSAL_RECENT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSC_TYPE_PROPOSAL_RECENT, GscProposalRecentPrivate))
 
static gboolean
gsc_proposal_recent_apply(GscProposal* proposal, GtkTextView *view)
{
	GscProposalRecent *self = GSC_PROPOSAL_RECENT (proposal);
	gedit_commands_load_uri(self->priv->window,
				self->priv->uri,
				NULL,
				1);
	return TRUE;
}

static void
gsc_proposal_recent_finalize (GObject *object)
{
	GscProposalRecent *self = GSC_PROPOSAL_RECENT (object);
	g_free (self->priv->uri);
	G_OBJECT_CLASS (gsc_proposal_recent_parent_class)->finalize (object);
}

static void
gsc_proposal_recent_class_init (GscProposalRecentClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (object_class, sizeof(GscProposalRecentPrivate));
	
	GscProposalClass *proposal_class = GSC_PROPOSAL_CLASS (klass);
	proposal_class->apply = gsc_proposal_recent_apply;
	object_class->finalize = gsc_proposal_recent_finalize;
}

static void
gsc_proposal_recent_init (GscProposalRecent *self)
{
	self->priv = GSC_PROPOSAL_RECENT_GET_PRIVATE (self);
}

GscProposal*
gsc_proposal_recent_new (GeditWindow *window,
			 GtkRecentInfo *info,
			 GdkPixbuf *icon)
{
	/* FIXME Check objects */
	const gchar *name = gtk_recent_info_get_display_name (info);
	gchar *display_info = gtk_recent_info_get_uri_display (info);
	const gchar *uri = gtk_recent_info_get_uri (info);
	GscProposalRecent *self = GSC_PROPOSAL_RECENT (g_object_new (GSC_TYPE_PROPOSAL_RECENT, 
								 "label", name,
								 "info", display_info,
								 "icon", icon,
								 "page-name", "Recent Files",
								 NULL));
	self->priv->window = window;
	self->priv->uri = g_strdup(uri);
	g_free (display_info);
	return GSC_PROPOSAL (self);
}



