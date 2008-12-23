 /* gsc-proposal-open.c
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

#include "gsc-proposal-open.h"

struct _GscProposalOpenPrivate
{
	GeditDocument *doc;
	GeditWindow *window;
};

G_DEFINE_TYPE(GscProposalOpen, gsc_proposal_open, GSC_TYPE_PROPOSAL);

#define GSC_PROPOSAL_OPEN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSC_TYPE_PROPOSAL_OPEN, GscProposalOpenPrivate))
 
static gboolean
gsc_proposal_open_apply(GscProposal* proposal, GtkTextView *view)
{
	GscProposalOpen *self = GSC_PROPOSAL_OPEN (proposal);
	GeditTab *tab = gedit_tab_get_from_document(self->priv->doc);
	gedit_window_set_active_tab(self->priv->window,tab);
	return TRUE;
}

static void
gsc_proposal_open_finalize (GObject *object)
{
	G_OBJECT_CLASS (gsc_proposal_open_parent_class)->finalize (object);
}

static void
gsc_proposal_open_class_init (GscProposalOpenClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (object_class, sizeof(GscProposalOpenPrivate));
	
	GscProposalClass *proposal_class = GSC_PROPOSAL_CLASS (klass);
	proposal_class->apply = gsc_proposal_open_apply;
}

static void
gsc_proposal_open_init (GscProposalOpen *self)
{
	self->priv = GSC_PROPOSAL_OPEN_GET_PRIVATE (self);
}

GscProposal*
gsc_proposal_open_new (GeditWindow *window,
		       GeditDocument *doc,
		       GdkPixbuf *icon)
{
	/* FIXME Check objects */
	gchar *name = gedit_document_get_short_name_for_display (doc);
	gchar *info = gedit_document_get_uri_for_display (doc);
	GscProposalOpen *self = GSC_PROPOSAL_OPEN (g_object_new (GSC_TYPE_PROPOSAL_OPEN, 
								 "label", name,
								 "info", info,
								 "icon", icon,
								 NULL));
	self->priv->doc = doc;
	self->priv->window = window;
	g_free (name);
	g_free (info);
	return GSC_PROPOSAL (self);
}



