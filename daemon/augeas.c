/* libguestfs - the guestfsd daemon
 * Copyright (C) 2009 Red Hat Inc. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <augeas.h>

#include "daemon.h"
#include "actions.h"

/* The Augeas handle.  We maintain a single handle per daemon, which
 * is all that is necessary and reduces the complexity of the API
 * considerably.
 */
static augeas *aug = NULL;

#define NEED_AUG(errcode)						\
  do {									\
    if (!aug) {								\
      reply_with_error ("%s: you must call 'aug-init' first to initialize Augeas", __func__); \
      return (errcode);							\
    }									\
  }									\
  while (0)

/* We need to rewrite the root path so it is based at /sysroot. */
int
do_aug_init (const char *root, int flags)
{
  char *buf;
  int len;

  NEED_ROOT (-1);
  ABS_PATH (root, -1);

  if (aug) {
    aug_close (aug);
    aug = NULL;
  }

  len = strlen (root) + 9;
  buf = malloc (len);
  if (!buf) {
    reply_with_perror ("malloc");
    return -1;
  }
  snprintf (buf, len, "/sysroot%s", root);

  aug = aug_init (buf, NULL, flags);
  free (buf);

  if (!aug) {
    reply_with_error ("Augeas initialization failed");
    return -1;
  }

  return 0;
}

int
do_aug_close (void)
{
  NEED_AUG(-1);

  aug_close (aug);
  aug = NULL;

  return 0;
}

int
do_aug_defvar (const char *name, const char *expr)
{
#ifdef HAVE_AUG_DEFVAR
  int r;

  NEED_AUG (-1);

  r = aug_defvar (aug, name, expr);
  if (r == -1) {
    reply_with_error ("Augeas defvar failed");
    return -1;
  }
  return r;
#else
  reply_with_error ("aug_defvar is not available");
  return -1;
#endif
}

guestfs_aug_defnode_ret *
do_aug_defnode (const char *name, const char *expr, const char *val)
{
#ifdef HAVE_AUG_DEFNODE
  static guestfs_aug_defnode_ret r;
  int created;

  NEED_AUG (NULL);

  r.nrnodes = aug_defnode (aug, name, expr, val, &created);
  if (r.nrnodes == -1) {
    reply_with_error ("Augeas defnode failed");
    return NULL;
  }
  r.created = created;
  return &r;
#else
  reply_with_error ("aug_defvar is not available");
  return NULL;
#endif
}

char *
do_aug_get (const char *path)
{
  const char *value = NULL;
  char *v;
  int r;

  NEED_AUG (NULL);

  r = aug_get (aug, path, &value);
  if (r == 0) {
    reply_with_error ("no matching node");
    return NULL;
  }
  if (r != 1) {
    reply_with_error ("Augeas get failed");
    return NULL;
  }

  /* value can still be NULL here, eg. try with path == "/augeas".
   * I don't understand this case, and it seems to contradict the
   * documentation.
   */
  if (value == NULL) {
    reply_with_error ("Augeas returned NULL match");
    return NULL;
  }

  /* The value is an internal Augeas string, so we must copy it. GC FTW. */
  v = strdup (value);
  if (v == NULL) {
    reply_with_perror ("strdup");
    return NULL;
  }

  return v;			/* Caller frees. */
}

int
do_aug_set (const char *path, const char *val)
{
  int r;

  NEED_AUG (-1);

  r = aug_set (aug, path, val);
  if (r == -1) {
    reply_with_error ("Augeas set failed");
    return -1;
  }

  return 0;
}

int
do_aug_insert (const char *path, const char *label, int before)
{
  int r;

  NEED_AUG (-1);

  r = aug_insert (aug, path, label, before);
  if (r == -1) {
    reply_with_error ("Augeas insert failed");
    return -1;
  }

  return 0;
}

int
do_aug_rm (const char *path)
{
  int r;

  NEED_AUG (-1);

  r = aug_rm (aug, path);
  if (r == -1) {
    reply_with_error ("Augeas rm failed");
    return -1;
  }

  return r;
}

int
do_aug_mv (const char *src, const char *dest)
{
  int r;

  NEED_AUG (-1);

  r = aug_mv (aug, src, dest);
  if (r == -1) {
    reply_with_error ("Augeas mv failed");
    return -1;
  }

  return 0;
}

char **
do_aug_match (const char *path)
{
  char **matches = NULL;
  void *vp;
  int r;

  NEED_AUG (NULL);

  r = aug_match (aug, path, &matches);
  if (r == -1) {
    reply_with_error ("Augeas match failed");
    return NULL;
  }

  /* This returns an array of length r, which we must extend
   * and add a terminating NULL.
   */
  vp = realloc (matches, sizeof (char *) * (r+1));
  if (vp == NULL) {
    reply_with_perror ("realloc");
    free (vp);
    return NULL;
  }
  matches = vp;
  matches[r] = NULL;

  return matches;		/* Caller frees. */
}

int
do_aug_save (void)
{
  NEED_AUG (-1);

  if (aug_save (aug) == -1) {
    reply_with_error ("Augeas save failed");
    return -1;
  }

  return 0;
}

int
do_aug_load (void)
{
#ifdef HAVE_AUG_LOAD
  NEED_AUG (-1);

  if (aug_load (aug) == -1) {
    reply_with_error ("Augeas load failed");
    return -1;
  }

  return 0;
#else
  reply_with_error ("aug_load is not available");
  return -1;
#endif
}

/* Simpler version of aug-match, which also sorts the output. */
char **
do_aug_ls (const char *path)
{
  char **matches;
  char *buf;
  int len;

  NEED_AUG (NULL);

  ABS_PATH (path, NULL);

  len = strlen (path);

  if (len > 1 &&
      (path[len-1] == '/' || path[len-1] == ']' || path[len-1] == '*')) {
    reply_with_error ("don't use aug-ls with a path that ends with / ] *");
    return NULL;
  }

  if (len == 1)
    /* we know path must be "/" because of ABS_PATH above */
    matches = do_aug_match ("/");
  else {
    len += 3;			/* / * + terminating \0 */
    buf = malloc (len);
    if (buf == NULL) {
      reply_with_perror ("malloc");
      return NULL;
    }

    snprintf (buf, len, "%s/*", path);
    matches = do_aug_match (buf);
    free (buf);
  }

  if (matches == NULL)
    return NULL;		/* do_aug_match has already sent the error */

  sort_strings (matches, count_strings (matches));
  return matches;		/* Caller frees. */
}
