#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include "useTree.h"
#include "common.h"
#include "ds.h"
#include "linkedList.h"
#include "display.h"

void printBranch(TreeInfo *tree, Bool islast)
{
    for (int j = 0; j < tree->level; j++)
    {
        if (tree->has_next[j] == '1')
        {
            printf("│  ");
        }
        else
        {
            printf("   ");
        }
    }
    if (islast)
    {
        printf("└─ ");
    }
    else
    {
        printf("├─ ");
    }
}

void treeMain(LinkedList l, TreeInfo *tree)
{
    Bool islast;
    File file;
    int i = 0, tmpLen = PthLen;
    int n = l->count - ((Mparams & MH) ? 2 : 0);

    while (i < n)
    {
        islast = (i == n - 1) ? True : False;
        PthLen = tmpLen;
        file = (File)(lAt(l, LFIRST))->data;
        if (S_ISDIR(file->st.st_mode))
        {
            if (strcmp(file->name, ".") && strcmp(file->name, ".."))
            {
                ++i;
                if (islast)
                {
                    tree->has_next[tree->level] = '0';
                }
                else
                {
                    tree->has_next[tree->level] = '1';
                }
                printBranch(tree, islast);
                display(file->name, &file->st.st_mode, True);
                if (tree->level + 1 < Tparam)
                {
                    tree->level++;
                    strcpy(&Pth[PthLen], file->name);
                    PthLen = strlen(Pth);
                    Pth[PthLen] = '/'; // add slash to path
                    PthLen++;
                    Pth[PthLen] = 0;
                    core(tree);
                    tree->level--;
                }
            }
        }
        else
        {
            ++i;
            printBranch(tree, islast);
            display(file->name, &file->st.st_mode, True);
        }
        free(file->name);
        free(file);
        lDelete(l, LFIRST);
    }
}