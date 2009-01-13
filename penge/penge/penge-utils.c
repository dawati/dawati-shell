#include "penge-utils.h"

#include <string.h>
#include <glib/gi18n.h>

#include <nbtk/nbtk.h>

/* Code here is stolen from Tweet (C) Emmanuele Bassi */
gchar *
penge_utils_format_time (GTimeVal *time_)
{
  GTimeVal now;
  struct tm tm_mtime;
  const gchar *format = NULL;
  gchar *locale_format = NULL;
  gchar buf[256];
  gchar *retval = NULL;
  gint secs_diff;
 
  g_return_val_if_fail (time_->tv_usec >= 0 && time_->tv_usec < G_USEC_PER_SEC, NULL);
 
  g_get_current_time (&now);
 
#ifdef HAVE_LOCALTIME_R
  localtime_r ((time_t *) &(time_->tv_sec), &tm_mtime);
#else
  {
    struct tm *ptm = localtime ((time_t *) &(time_->tv_usec));
 
    if (!ptm)
      {
        g_warning ("ptm != NULL failed");
        return NULL;
      }
    else
      memcpy ((void *) &tm_mtime, (void *) ptm, sizeof (struct tm));
  }
#endif /* HAVE_LOCALTIME_R */
 
  secs_diff = now.tv_sec - time_->tv_sec;
 
  /* within the hour */
  if (secs_diff < 60)
    retval = g_strdup (_("Less than a minute ago"));
  else
    {
      gint mins_diff = secs_diff / 60;
 
      if (mins_diff < 60)
        retval = g_strdup_printf (ngettext ("About a minute ago",
                                            "About %d minutes ago",
                                            mins_diff),
                                  mins_diff);
      else if (mins_diff < 360)
        {
          gint hours_diff = mins_diff / 60;
 
          retval = g_strdup_printf (ngettext ("About an hour ago",
                                              "About %d hours ago",
                                              hours_diff),
                                    hours_diff);
        }
    }
 
  if (retval)
    return retval;
  else
    {
      GDate d1, d2;
      gint days_diff;
 
      g_date_set_time_t (&d1, now.tv_sec);
      g_date_set_time_t (&d2, time_->tv_sec);
 
      days_diff = g_date_get_julian (&d1) - g_date_get_julian (&d2);
 
      if (days_diff == 0)
        format = _("Today at %H:%M");
      else if (days_diff == 1)
        format = _("Yesterday at %H:%M");
      else
        {
          if (days_diff > 1 && days_diff < 7)
            format = _("Last %A at %H:%M"); /* day of the week */
          else
            format = _("%x at %H:%M"); /* any other date */
        }
    }
 
  locale_format = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
 
  if (strftime (buf, sizeof (buf), locale_format, &tm_mtime) != 0)
    retval = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  else
    retval = g_strdup (_("Unknown"));
 
  g_free (locale_format);
 
  return retval;
}

gchar *
penge_utils_get_thumbnail_path (const gchar *uri)
{
  gchar *thumbnail_path;
  gchar *thumbnail_filename;
  gchar *csum;

  csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
  thumbnail_filename = g_strconcat (csum, ".png", NULL);
  thumbnail_path = g_build_filename (g_get_home_dir (),
                                     ".thumbnails",
                                     "normal",
                                     thumbnail_filename,
                                     NULL);

  g_free (csum);
  g_free (thumbnail_filename);

  if (g_file_test (thumbnail_path, G_FILE_TEST_EXISTS))
  {
    return thumbnail_path;
  } else {
    g_free (thumbnail_path);
    return NULL;
  }
}

void
penge_utils_load_stylesheet ()
{
  NbtkStyle *style;
  GError *error = NULL;
  gchar *path;

  path = g_build_filename (PKG_DATADIR,
                           "theme",
                           "penge.css",
                           NULL);

  /* register the styling */
  style = nbtk_style_get_default ();

  if (!nbtk_style_load_from_file (style,
                             path,
                             &error))
  {
    g_warning (G_STRLOC ": Error opening style: %s",
               error->message);
    g_clear_error (&error);
  }

  g_free (path);
}

