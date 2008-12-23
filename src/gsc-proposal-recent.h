/*
 * gsc-proposal-recent.h
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

#ifndef __GSC_PROPOSAL_RECENT_H__
#define __GSC_PROPOSAL_RECENT_H__

#include <glib.h>
#include <glib-object.h>
#include <gedit/gedit-plugin.h>
#include <gtksourcecompletion/gsc-proposal.h>

G_BEGIN_DECLS

typedef struct _GscProposalRecentPrivate GscProposalRecentPrivate;
typedef struct _GscProposalRecent GscProposalRecent;
typedef struct _GscProposalRecentClass GscProposalRecentClass;

struct _GscProposalRecent
{
	GscProposal parent;
	GscProposalRecentPrivate *priv;
};

struct _GscProposalRecentClass
{
	GscProposalClass parent_class;
};

#define GSC_TYPE_PROPOSAL_RECENT (gsc_proposal_recent_get_type ())
#define GSC_PROPOSAL_RECENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSC_TYPE_PROPOSAL_RECENT, GscProposalRecent))
#define GSC_PROPOSAL_RECENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GSC_TYPE_PROPOSAL_RECENT, GscProposalRecentClass))
#define GSC_IS_PROPOSAL_RECENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSC_TYPE_PROPOSAL_RECENT))
#define GSC_IS_PROPOSAL_RECENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSC_TYPE_PROPOSAL_RECENT))
#define GSC_PROPOSAL_RECENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSC_TYPE_PROPOSAL_RECENT, GscProposalRecentClass))

GType		 gsc_proposal_recent_get_type	(void) G_GNUC_CONST;

GscProposal	*gsc_proposal_recent_new	(GeditWindow *window,
						 GtkRecentInfo *info,
						 GdkPixbuf *icon);



G_END_DECLS

#endif
