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
#include <glib/gprintf.h>
#include <string.h>
#include <ctype.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcecompletionitem.h>
#include "gwp-provider-words.h"

#define GWP_PROVIDER_WORDS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GWP_TYPE_PROVIDER_WORDS, GwpProviderWordsPrivate))

static void	 gwp_provider_words_iface_init	(GtkSourceCompletionProviderIface *iface);

struct _GwpProviderWordsPrivate {
	GHashTable *current_words;
	GList *data_list;
	gchar *cleaned_word;
	GdkPixbuf *provider_icon;
	GdkPixbuf *proposal_icon;
	gint count;
	GwpProviderWordsSortType sort_type;
	GtkTextIter start_iter;
	GtkSourceView *view;
};

G_DEFINE_TYPE_WITH_CODE (GwpProviderWords,
			 gwp_provider_words,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_SOURCE_COMPLETION_PROVIDER,
				 		gwp_provider_words_iface_init))

static gboolean
is_separator(const gunichar ch)
{
	if (g_unichar_isprint(ch) && 
	    (g_unichar_isalnum(ch) || ch == g_utf8_get_char("_")))
	{
		return FALSE;
	}
	
	return TRUE;
}

gchar *
get_word_iter (GtkSourceBuffer *source_buffer, 
	       GtkTextIter     *current,
	       GtkTextIter     *start_word, 
	       GtkTextIter     *end_word)
{
	GtkTextBuffer *text_buffer;
	gunichar ch;
	gboolean no_doc_start;
	
	text_buffer = GTK_TEXT_BUFFER (source_buffer);
	
	if (current == NULL)
	{
		gtk_text_buffer_get_iter_at_mark (text_buffer,
		                                  start_word,
		                                  gtk_text_buffer_get_insert (text_buffer));
	}
	else
	{
		*start_word = *current;
	}
	
	*end_word = *start_word;

	while ((no_doc_start = gtk_text_iter_backward_char (start_word)) == TRUE)
	{
		ch = gtk_text_iter_get_char (start_word);

		if (is_separator (ch))
		{
			break;
		}
	}
	
	if (!no_doc_start)
	{
		gtk_text_buffer_get_start_iter (text_buffer, start_word);
		return gtk_text_iter_get_text (start_word, end_word);
	}
	else
	{
		gtk_text_iter_forward_char (start_word);
		return gtk_text_iter_get_text (start_word, end_word);
	}
}

static GdkPixbuf *
get_icon_from_theme (const gchar *name)
{
	GtkIconTheme *theme;
	gint width;
	
	theme = gtk_icon_theme_get_default ();

	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, NULL);
	return gtk_icon_theme_load_icon (theme,
	                                 name,
	                                 width,
	                                 GTK_ICON_LOOKUP_USE_BUILTIN,
	                                 NULL);
}

static void
clean_current_words(GwpProviderWords* self)
{
	/*Clean the previous data*/
	if (self->priv->current_words!=NULL)
	{	
		g_hash_table_destroy(self->priv->current_words);
		self->priv->current_words = NULL;
	}
}

static gboolean
pred_is_separator(gunichar ch, gpointer user_data)
{
	return is_separator(ch);
}

static gint
utf8_len_compare(gconstpointer a, gconstpointer b)
{
    glong lena,lenb;
    lena = g_utf8_strlen(gtk_source_completion_proposal_get_label((GtkSourceCompletionProposal*)a),-1);
    lenb = g_utf8_strlen(gtk_source_completion_proposal_get_label((GtkSourceCompletionProposal*)b),-1);
    if (lena==lenb)
        return 0;
    else if (lena<lenb)
        return -1;
    else
        return 1;
}

static gchar*
clear_word(const gchar* word)
{
  int len = g_utf8_strlen(word,-1);
  int i;
  const gchar *temp = word;
  
  for (i=0;i<len;i++)
  {
    if (is_separator(g_utf8_get_char(temp)))
      temp = g_utf8_next_char(temp);
    else
      return g_strdup(temp);
    
  }
  return NULL;
}

static GHashTable*
get_all_words(GwpProviderWords* self, GtkTextBuffer *buffer )
{
	GtkTextIter start_iter;
	GtkTextIter prev_iter;
	gchar *word;
	GHashTable *result = g_hash_table_new_full(g_str_hash,
						   g_str_equal,
						   g_free,
						   NULL);
	
	
	gtk_text_buffer_get_start_iter(buffer,&start_iter);
	prev_iter = start_iter;
	while (gtk_text_iter_forward_find_char(
			&start_iter,
			(GtkTextCharPredicate)pred_is_separator,
			NULL,
			NULL))
	{
		if (gtk_text_iter_compare(&self->priv->start_iter,&prev_iter)!=0)
		{
			word = gtk_text_iter_get_text(&prev_iter,&start_iter);
        
			if (strlen(word)>0)
				g_hash_table_insert(result,word,NULL);
			else
				g_free(word);
		}
		prev_iter = start_iter;
		gtk_text_iter_forward_char(&prev_iter);
	}

	if (!gtk_text_iter_is_end(&prev_iter))
	{
		gtk_text_buffer_get_end_iter(buffer,&start_iter);
		if (gtk_text_iter_compare(&self->priv->start_iter,&prev_iter)!=0)
		{
			word = gtk_text_iter_get_text(&prev_iter,&start_iter);
			if (strlen(word)>0)
				g_hash_table_insert(result,word,NULL);
			else
				g_free(word);
		}
		prev_iter = start_iter;
	        gtk_text_iter_forward_char(&prev_iter);
	}
	return result;
}

static gboolean
is_valid_word(gchar *current_word, gchar *completion_word)
{
	if (g_utf8_strlen(completion_word, -1) < 3)
		return FALSE;
	
	if (current_word==NULL)
		return TRUE;
		
	gint len_cur = strlen (current_word);
	if (g_utf8_collate(current_word,completion_word) == 0)
			return FALSE;

	if (len_cur!=0 && strncmp(current_word,completion_word,len_cur)==0)
	{
		return TRUE;
	}
	return FALSE;
}

/*
 * Check the proposals hash and inserts the completion proposal into the final list
 */
static void
gh_add_key_to_list(gpointer key,
		   gpointer value,
		   gpointer user_data)
{

	GwpProviderWords *self = GWP_PROVIDER_WORDS(user_data);
	if (self->priv->count>=500)
	{
		return;
	}
	GtkSourceCompletionProposal *data;
	if (is_valid_word(self->priv->cleaned_word,(gchar*)key))
	{
		self->priv->count++;
		data = GTK_SOURCE_COMPLETION_PROPOSAL (gtk_source_completion_item_new((gchar*)key,
						      self->priv->proposal_icon,
						      NULL));
		self->priv->data_list = g_list_append(self->priv->data_list,data);
	}
}

static GList*
_sort_completion_list(GwpProviderWords *self, GList *data_list)
{
	switch(self->priv->sort_type)
	{
		case GWP_PROVIDER_WORDS_SORT_BY_LENGTH:
		{
			data_list = g_list_sort(data_list,
						(GCompareFunc)utf8_len_compare );
			break;
		}
		default: 
			break;
	}
	
	return data_list;
}


static const gchar * 
gwp_provider_words_get_name (GtkSourceCompletionProvider *self)
{
	return GWP_PROVIDER_WORDS_NAME;
}


GdkPixbuf * 
gwp_provider_words_get_icon (GtkSourceCompletionProvider *self)
{
	return  GWP_PROVIDER_WORDS(self)->priv->provider_icon;
}

static GList *
gwp_provider_words_get_proposals (GtkSourceCompletionProvider 	*base,
				  GtkTextIter			*place)
{
	GwpProviderWords *self = GWP_PROVIDER_WORDS(base);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (self->priv->view));
	GtkTextIter end_iter;
	clean_current_words(self);

	gchar* current_word = get_word_iter(GTK_SOURCE_BUFFER (text_buffer),
					    place,
					    &self->priv->start_iter,
					    &end_iter);
	self->priv->cleaned_word = clear_word(current_word);
	g_free(current_word);
	
	self->priv->current_words = get_all_words(self,text_buffer);
	
	self->priv->data_list = NULL;
	self->priv->count = 0;
	g_hash_table_foreach(self->priv->current_words,gh_add_key_to_list,self);
	g_free(self->priv->cleaned_word);
	self->priv->cleaned_word = NULL;
	
	if (self->priv->data_list!=NULL)
	{
		self->priv->data_list = _sort_completion_list(self,
							      self->priv->data_list);
	}

	return self->priv->data_list;
}

static gboolean
gwp_provider_words_filter_proposal (GtkSourceCompletionProvider *provider,
                                    GtkSourceCompletionProposal *proposal,
				    GtkTextIter			*iter,
                                    const gchar			*criteria)
{
	const gchar *label;
	
	label = gtk_source_completion_proposal_get_label (proposal);
	return g_str_has_prefix (label, criteria);
}

static gboolean
gwp_provider_words_get_interactive (GtkSourceCompletionProvider *provider)
{
	return TRUE;
}

static void 
gwp_provider_words_finalize(GObject *object)
{
	GwpProviderWords *self;
	self = GWP_PROVIDER_WORDS(object);
	
	clean_current_words(self);
	g_free(self->priv->cleaned_word);
	self->priv->cleaned_word = NULL;
	self->priv->data_list = NULL;
	self->priv->count= 0;
	self->priv->view = NULL;
	gdk_pixbuf_unref (self->priv->provider_icon);
	gdk_pixbuf_unref (self->priv->proposal_icon);
	G_OBJECT_CLASS(gwp_provider_words_parent_class)->finalize(object);
}


static void 
gwp_provider_words_class_init (GwpProviderWordsClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	G_OBJECT_CLASS (klass)->finalize = gwp_provider_words_finalize;
	
	g_type_class_add_private (object_class, sizeof(GwpProviderWordsPrivate));
}


static void 
gwp_provider_words_iface_init (GtkSourceCompletionProviderIface * iface)
{
	iface->get_name = gwp_provider_words_get_name;
	iface->get_icon = gwp_provider_words_get_icon;

	iface->get_proposals = gwp_provider_words_get_proposals;
	iface->filter_proposal = gwp_provider_words_filter_proposal;
	iface->get_interactive = gwp_provider_words_get_interactive;
}


static void gwp_provider_words_init (GwpProviderWords * self)
{
	self->priv = GWP_PROVIDER_WORDS_GET_PRIVATE (self);
	self->priv->current_words = NULL;
	self->priv->count=0;
	self->priv->view = NULL;
	self->priv->cleaned_word=NULL;
	self->priv->provider_icon = get_icon_from_theme (GTK_STOCK_COPY);
	self->priv->proposal_icon = get_icon_from_theme (GTK_STOCK_NEW);
	self->priv->sort_type = GWP_PROVIDER_WORDS_SORT_BY_LENGTH;
}

GwpProviderWords*
gwp_provider_words_new(GtkSourceView *view)
{
	GwpProviderWords *self = g_object_new (GWP_TYPE_PROVIDER_WORDS, NULL);
	self->priv->view = view;
	return self;
}

void
gwp_provider_words_set_sort_type(GwpProviderWords *prov,
					 GwpProviderWordsSortType sort_type)
{
	prov->priv->sort_type = sort_type;
}

GwpProviderWordsSortType
gwp_provider_words_get_sort_type(GwpProviderWords *prov)
{
	return prov->priv->sort_type;
}
