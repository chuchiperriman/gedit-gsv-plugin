/* Copyright (C) 2007 - Jesús Barbero <chuchiperriman@gmail.com>
 *  
 *  This file is part of gtksourcecompletion.
 *
 *  gtksourcecompletion is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  gtksourcecompletion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __PROVIDER_WORDS_H__
#define __PROVIDER_WORDS_H__

#include <glib.h>
#include <glib-object.h>
#include <gtksourceview/gtksourcecompletionprovider.h>

G_BEGIN_DECLS

#define GWP_TYPE_PROVIDER_WORDS (gwp_provider_words_get_type ())
#define GWP_PROVIDER_WORDS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GWP_TYPE_PROVIDER_WORDS, GwpProviderWords))
#define GWP_PROVIDER_WORDS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GWP_TYPE_PROVIDER_WORDS, GwpProviderWordsClass))
#define GWP_IS_PROVIDER_WORDS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GWP_TYPE_PROVIDER_WORDS))
#define GWP_IS_PROVIDER_WORDS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GWP_TYPE_PROVIDER_WORDS))
#define GWP_PROVIDER_WORDS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GWP_TYPE_PROVIDER_WORDS, GwpProviderWordsClass))

#define GWP_PROVIDER_WORDS_NAME "GwpProviderWords"

typedef struct _GwpProviderWords GwpProviderWords;
typedef struct _GwpProviderWordsClass GwpProviderWordsClass;
typedef struct _GwpProviderWordsPrivate GwpProviderWordsPrivate;

struct _GwpProviderWords {
	GObject parent;
	GwpProviderWordsPrivate *priv;	
};

struct _GwpProviderWordsClass {
	GObjectClass parent;
};

/**
 * GwpProviderWordsSortType:
 * @GWP_PROVIDER_WORDS_SORT_NONE: Does not sort the proposals
 * @GWP_PROVIDER_WORDS_SORT_BY_LENGTH: Sort the proposals by label 
 * lenght. Sets the small words first an large words last.
 **/
typedef enum{
	GWP_PROVIDER_WORDS_SORT_NONE,
	GWP_PROVIDER_WORDS_SORT_BY_LENGTH
} GwpProviderWordsSortType;

/**
 * gwp_provider_words_new:
 * @view: #GtkTextView where the provider must search for words
 * Returns The new #GwpProviderWords
 *
 */
GwpProviderWords* 
gwp_provider_words_new(GtkSourceView *view);

/*TODO Change to a sort_function*/

/**
 * gwp_provider_words_set_sort_type:
 * @prov: The #GwpProviderWords
 * @sort_type: The #GwpProviderWordsSortType for the completion proposal.
 *
 * This method sets the sort type for the completion proposals list.
 */
void
gwp_provider_words_set_sort_type(GwpProviderWords *prov,
					 GwpProviderWordsSortType sort_type);

/**
 * gwp_provider_words_get_sort_type:
 * @prov: The #GwpProviderWords
 *
 * Returns The current sort type.
 */
GwpProviderWordsSortType
gwp_provider_words_get_sort_type(GwpProviderWords *prov);

GType 
gwp_provider_words_get_type ();

G_END_DECLS

#endif

