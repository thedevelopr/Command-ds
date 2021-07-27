#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>
#include "common.h"
#include "lf.h"
#include "format_column.h"
#include "format_long.h"
#include "format_tree.h"
#include "list.h"

void run(LIST l)
{
    char *tmp;
    int i = 0;
    struct stat s;
    format_tree_t tree;
    lftype t;
    LIST l2;
    ITERATOR it;

    it = LAT(l, LFIRST);
    l2 = LOPEN();
    while (it)
    {
        // initialise 'path' and 'path_len'
        path[0] = 0;
        pathsiz = 0;
        tmp = (char *)it->data;
        lf_stat(tmp, &s);
        /**
         * 
         *  case where link is dir...
         * 
         */
        if (S_ISDIR(s.st_mode))
        {
            // init global variable "path" and "pathsiz"
            strcpy(path, tmp);
            pathsiz = strlen(tmp);
            // if tree
            if (opt.t)
            { // if first time then allocate memory for "parent_has_next".
                tree.level = 0;
                if (!tree.parent_has_next)
                {
                    tree.parent_has_next = (char *)lfalloc(sizeof(char) * PATH_MAX);
                }
                if (path[pathsiz - 1] == '/')
                {
                    pathsiz--;
                    path[pathsiz] = 0;
                }
                lf_show(path, &s.st_mode, 1);
            }
            // add slash if doesn't have it.
            if (path[pathsiz - 1] != '/')
            {
                path[pathsiz] = '/';
                pathsiz++;
                path[pathsiz] = 0;
            }
            // if multiple arguments then print name of each argument.
            if (l->count != 1)
            {
                printf("%s:\n", path);
            }
            // engine :)
            core(&tree);
        }
        else
        {
            LRESET(l2);
            t = (lftype)lfalloc(LFSIZ);
            t->st = s;
            t->name = tmp;
            LADD(l2, LFIRST, t);
            if (l->count != 1)
            {
                printf("%s:\n", tmp);
            }
            if (opt.l || opt.p || opt.s || opt.u || opt.g || opt.m)
            {
                long_main(l2);
            }
            else
            {
                lf_show(tmp, &s.st_mode, 1);
                printf("\n");
            }
        }
        if (i != l->count - 1)
        {
            printf("\n");
        }
        LINC(&it);
        ++i;
    }
    LCLOSE(l2);
}

int lf_count_dir_items(DIR *d)
{
    int i = 0;
    while (readdir(d))
    {
        i++;
    }
    rewinddir(d);
    return i;
}

// compare no case sensative
int cmp(lftype t1, lftype t2)
{
    char *s1, *s2;
    int n, l1, l2, ok;
    char c1, c2;

    if (!t1 || !t2)
    {
        return 0;
    }
    s1 = t1->name;
    s2 = t2->name;
    if (!s1 && !s2)
    {
        return 0;
    }
    if (!s1)
    {
        return -1;
    }
    if (!s2)
    {
        return 1;
    }
    l1 = strlen(s1);
    l2 = strlen(s2);
    if (l1 > l2)
    {
        n = l2;
    }
    else
    {
        n = l1;
    }
    ok = 0;
    for (int i = 0; !ok && i < n; i++)
    {
        // convert s1 to lower case
        if (s1[i] >= 'A' && s1[i] <= 'Z')
        {
            c1 = s1[i] + 32;
        }
        else
        {
            c1 = s1[i];
        }
        // convert s2 to lower case
        if (s2[i] >= 'A' && s2[i] <= 'Z')
        {
            c2 = s2[i] + 32;
        }
        else
        {
            c2 = s2[i];
        }
        if (c1 < c2)
        {
            ok = -1;
        }
        else if (c1 > c2)
        {
            ok = 1;
        }
    }
    if (!ok)
    {
        if (l1 > l2)
        {
            ok = 1;
        }
        else if (l1 < l2)
        {
            ok = -1;
        }
    }
    return ok;
}

void core(format_tree_t *tree)
{
    DIR *d;
    struct dirent *f;
    struct stat s;
    LIST l;
    lftype t;
    int index;

    if (tree->level == 0)
    {
        int ok = 0;
        ++ok;
    }

    // keep value of "path" and "path_z"
    d = opendir(path);
    if (!d)
    {
        lf_path_error(errno);
        if (opt.t)
        {
            tree_display(tree, 1);
            /**
             * 
             * Modify the error inst.
             * 
             * */
            printf("%saccess denied: %s%s\n", RED, buf, RST);
            buf[0] = 0;
        }
        else
        {
            printf("%s: access denied to \"%s\"\n", PROGRAM, path);
        }
        return;
    }
    l = LOPEN();
    index = 0;
    while ((f = readdir(d)))
    {
        if (opt.a || (f->d_name[0] != '.'))
        {
            strcpy(&path[pathsiz], f->d_name);
            lf_stat(path, &s);
            /**
             * 
             *  #case where link is dir
             *  # allocate strlen(f->d_name) + 2 // +2 is for the "/"
             * 
             */
            t = (lftype)lfalloc(LFSIZ);
            t->st = s;
            t->name = (char *)lfalloc(sizeof(char) * (strlen(f->d_name) + 1));
            strcpy(t->name, f->d_name);
            if (S_ISDIR(s.st_mode))
            {
                if (opt.d)
                {
                    LADD(l, LLAST, t);
                }
            }
            else
            {
                if (opt.f)
                {
                    LADD(l, LFIRST, t);
                    ++index;
                }
            }
        }
    }
    closedir(d);
    if (LEMPTY(l))
    {
        return;
    }
    if (index > 0)
    { // if there's files
        LSORT(l, LFIRST, index - 1, (int (*)(void *, void *))cmp);
    }
    if (index != l->count)
    { // if there's folders
        LSORT(l, index, LLAST, (int (*)(void *, void *))cmp);
    }
    if (opt.t)
    { // format tree
        tree_main(l, index, tree);
    }
    else if (opt.l || opt.p || opt.s || opt.u || opt.g || opt.m)
    { // format long
        long_main(l);
    }
    else
    { // format column
        column_main(l, NULL);
    }
    if (!LEMPTY(l))
    {
        ITERATOR i = LAT(l, LFIRST);
        while (i)
        {
            t = (lftype)i->data;
            free(t->name);
            free(t);
            LINC(&i);
        }
    }

    LCLOSE(l);
}