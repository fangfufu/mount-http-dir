#include "network.h"

/* must be included before including <fuse.h> */
#define FUSE_USE_VERSION 26
#include <fuse.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

static int fs_getattr(const char *path, struct stat *stbuf);
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi);
static int fs_open(const char *path, struct fuse_file_info *fi);
static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi);

static struct fuse_operations fs_oper = {
    .getattr	= fs_getattr,
    .readdir	= fs_readdir,
    .open		= fs_open,
    .read		= fs_read,
};

int main(int argc, char **argv) {
    network_init(argv[1]);
    return fuse_main(argc - 1, argv + 1, &fs_oper, NULL);
}

/** \brief return the attributes for a single file indicated by path */
static int fs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if (!strcmp(path, "/")) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 1;
    } else {
        Link *link = path_to_Link(path);
        if (!link) {
            return -ENOENT;
        }
        switch (link->type) {
            case LINK_DIR:
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 1;
                break;
            case LINK_FILE:
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = link->content_length;
                break;
            default:
                return -ENOENT;
        }
    }

    return res;
}

/** \brief read the directory indicated by the path*/
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t dir_add,
                      off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    Link *link;
    LinkTable *linktbl;


    if (!strcmp(path, "/")) {
        linktbl = ROOT_LINK_TBL;
    } else {
        link = path_to_Link(path);
        if (!link) {
            return -ENOENT;
        }
        linktbl = link->next_table;
        if (!linktbl) {
            return -ENOENT;
        }
    }

    /* start adding the links */
    dir_add(buf, ".", NULL, 0);
    dir_add(buf, "..", NULL, 0);
    for (int i = 1; i < linktbl->num; i++) {
        link = linktbl->links[i];
        dir_add(buf, link->p_url, NULL, 0);
    }

    return 0;
}

/** \brief open a file indicated by the path */
static int fs_open(const char *path, struct fuse_file_info *fi)
{
    if (!path_to_Link(path)) {
        return -ENOENT;
    }

    if ((fi->flags & 3) != O_RDONLY) {
        return -EACCES;
    }

    return 0;
}

/** \brief read a file */
static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi)
{
    (void) fi;
    Link *link;
    link = path_to_Link(path);

    if (!link) {
        return -ENOENT;
    }

    int bytes_downloaded = Link_download(link, offset, size);
    memcpy(buf, link->body, bytes_downloaded);

    return bytes_downloaded;
}
