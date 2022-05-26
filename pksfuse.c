/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall pksfuse.c rbtree.c `pkg-config fuse --cflags --libs` -o main
 *
 * ## Source code ##
 * \include hello.c
 */

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "rbtree.h"

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct PKSFileInfo
{
	struct rb_node node;
	char *Path;
	char *Content;
	int dir;
	int len;
} pks;
static struct rb_root ROOT = RB_ROOT;

struct PKSFileInfo *my_search(struct rb_root *root, const char *string)
{
	struct rb_node *Nownode = root->rb_node;
	while (Nownode != NULL)
	{
		struct PKSFileInfo *data = container_of(Nownode, struct PKSFileInfo, node);
		int result;
		result = strcmp(string, data->Path);
		if (result < 0)
			Nownode = Nownode->rb_left;
		else if (result > 0)
			Nownode = Nownode->rb_right;
		else
			return data;
	}
	return NULL;
}
int my_insert(struct rb_root *root, struct PKSFileInfo *data)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	/* Figure out where to put new node */
	while (*new)
	{
		struct PKSFileInfo *this = container_of(*new, struct PKSFileInfo, node);
		int result = strcmp(data->Path, this->Path);
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return -1;
	}
	/* Add new node and rebalance tree. */
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);
	return 0;
}
void my_free(struct PKSFileInfo *data)
{
	if (data->Path != NULL)
		free(data->Path);
	if (data->Content != NULL)
		free(data->Content);
	free(data);
}
int my_erase(struct rb_root *root, const char *path)
{
	struct PKSFileInfo *data = my_search(root, path);
	if (data)
	{
		rb_erase(&data->node, root);
		my_free(data);
	}
	return 0;
}
char *PKSCOPY(const char *X)
{
	char *Y = (char *)malloc(strlen(X) + 1);
	strcpy(Y, X);
	return Y;
}
struct PKSFileInfo *my_newnode(const char *path, char *Content, int dir)
{
	struct PKSFileInfo *Newnode = (struct PKSFileInfo *)malloc(sizeof(struct PKSFileInfo));
	Newnode->Path = PKSCOPY(path);
	// if (Content!=NULL) Newnode->Content=PKSCOPY(Content);
	Newnode->Content = NULL;
	Newnode->dir = dir;
	Newnode->len = 0;
	return Newnode;
}
// https://blog.csdn.net/star871016/article/details/109311872

static struct options
{
	const char *filename;
	const char *contents;
	int show_help;
} options;

#define OPTION(t, p)                      \
	{                                     \
		t, offsetof(struct options, p), 1 \
	}
static const struct fuse_opt option_spec[] = {
	OPTION("--name=%s", filename),
	OPTION("--contents=%s", contents),
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END};

static void *hello_init(struct fuse_conn_info *conn)
{
	// printf("init");
	// fflush(stdout);
	(void)conn;
	return NULL;
}

static int hello_getattr(const char *path, struct stat *stbuf)
{
	// printf("getattr %s\n",path);
	// fflush(stdout);
	int res = 0;
	struct PKSFileInfo *Data = my_search(&ROOT, path);

	if (Data == NULL)
	{
		res = -ENOENT;
		printf("RE\n");
		return res;
	}
	memset(stbuf, 0, sizeof(struct stat));
	if (Data->dir == 1)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = Data->len;
	}
	return res;
}
char *checkSon(char *Father, char *Son)
{
	// printf("CheckSon Father:%s  Son:%s", Father, Son);
	// fflush(stdout);
	while (Father[0] != '\0' && Son[0] != '\0' && Father[0] == Son[0])
		Father++, Son++;
	if (Father[0] != '\0')
	{
		// printf("Finish CheckSon Fail");
		return NULL;
	}
	if (Son[0] == '/')
		Son++;
	char *CheckPoint = Son;
	while (CheckPoint[0] != '\0')
	{
		if (CheckPoint[0] == '/')
		{
			// printf("Finish CheckSon Fail");
			return NULL;
		}
		CheckPoint++;
	}
	// printf("Finish CheckSon Success%s", Son);
	return Son;
}
static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
						 off_t offset, struct fuse_file_info *fi)
{
	(void)offset;
	(void)fi;

	// printf("readdir %s\n", path);
	// fflush(stdout);

	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data == NULL)
		return -ENOENT;
	if (Data->dir == 0)
		return -ENOTDIR;
	filler(buf, ".", NULL, 0);

	if (strcmp(Data->Path, "/") != 0)
		filler(buf, "..", NULL, 0);
	struct rb_node *Nownode;
	for (Nownode = rb_next(&Data->node); Nownode != NULL; Nownode = rb_next(Nownode))
	{
		char *rev;
		struct PKSFileInfo *SonData = container_of(Nownode, struct PKSFileInfo, node);
		if ((rev = checkSon(Data->Path, SonData->Path)) != NULL)
			filler(buf, rev, NULL, 0);
	}
	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{

	// printf("open%s", path);
	// fflush(stdout);
	// printf("%d %d %d\n",fi->flags,O_ACCMODE,O_RDONLY);
	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data->dir == 1)
		return -EISDIR;
	if (Data == NULL)
	{
		// if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
		struct PKSFileInfo *NewNode = my_newnode(path, NULL, 0);
		my_insert(&ROOT, NewNode);
	}
	// if ((fi->flags & O_ACCMODE) != O_RDONLY)
	// 	return -EACCES;
	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
					  struct fuse_file_info *fi)
{
	size_t len;
	(void)fi;

	// printf("read%s %d %d ", path, offset, size);
	// fflush(stdout);
	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data == NULL)
		return -ENOENT;
	if (Data->dir == 1)
		return -EISDIR;

	len = Data->len;
	printf("%d\n", len);
	if (offset < len)
	{
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, Data->Content + offset, size);
	}
	else
		size = 0;
	// printf("read size%d\n", size);
	return size;
}

char *CheckChange(const char *OldPath)
{
	// printf("CheckChange%s\n", OldPath);
	int cnt = 0, id1, id2;
	int len = strlen(OldPath);
	for (int i = 1; OldPath[i] != '\0'; i++)
	{
		printf("%d ", i);
		fflush(stdout);
		if (OldPath[i] == '/')
		{
			cnt++;
			if (cnt == 1)
				id1 = i;
			if (cnt == 2)
				id2 = i;
		}
	}
	if (cnt != 2)
	{
		// printf("Finish Fail\n");
		// fflush(stdout);
		return NULL;
	}
	char *Y = PKSCOPY(OldPath);
	memcpy(Y + 1, OldPath + (id1 + 1), id2 - id1 - 1);
	Y[id2 - id1] = '/';
	memcpy(Y + id2 - id1 + 1, OldPath + 1, id1 - 1);
	// printf("Finish%s", Y);
	// fflush(stdout);
	return Y;
}
static int hello_write(const char *path, const char *buf, size_t size,
					   off_t offset, struct fuse_file_info *fi)
{
	(void)fi;

	// printf("write");
	// fflush(stdout);

	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data == NULL)
		return -ENOENT;
	if (Data->dir == 1)
		return -EISDIR;
	int len = Data->len;
	if (offset + size > len)
	{
		Data->Content = realloc(Data->Content, offset + size);
		Data->len = offset + size;
		len = Data->len;
	}
	memcpy(Data->Content + offset, buf, size);
	char *rev;
	if ((rev = CheckChange(path)) != NULL)
	{
		struct PKSFileInfo *RevData = my_search(&ROOT, rev);
		if (RevData != NULL)
		{
			int Revlen = RevData->len;
			if (offset + size > Revlen)
			{
				RevData->Content = realloc(RevData->Content, offset + size);
				RevData->len = offset + size;
				Revlen = RevData->len;
			}
			memcpy(RevData->Content + offset, buf, size);
		}
	}
	return size;
}
static int hello_mkdir(const char *path, mode_t mode)
{
	(void)mode;
	// printf("mkdir");
	// fflush(stdout);
	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data != NULL)
		return -EEXIST;
	return my_insert(&ROOT, my_newnode(path, NULL, 1));
}

static int hello_rmdir(const char *path)
{
	// printf("rmdir");
	// fflush(stdout);
	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data == NULL)
		return -ENOENT;
	my_erase(&ROOT, path);
	return 0;
}

// int hello_rename(const char *path, const char *pathnew)
// {
// 	printf("rename\n");
// 	fflush(stdout);
// 	return 0;
// }
// int hello_truncate(const char *path, off_t size)
// {
// 	printf("truncate\n");
// 	fflush(stdout);
// 	return 0;
// }
static int hello_mknod(const char *path, mode_t mode, dev_t rdev)
{
	(void)mode, (void)rdev;
	// printf("mknod%s\n", path);
	// fflush(stdout);
	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data != NULL)
		return -EEXIST;
	int err = my_insert(&ROOT, my_newnode(path, NULL, 0));
	// printf("mknod Finish%d\n",err);
	return err;
}
int hello_utimens(const char *path, const struct timespec tv[2])
{
	// printf("utimens%s\n", path);
	// fflush(stdout);
	return 0;
}
// int hello_readlink(const char *path, char *X, size_t Y)
// {
// 	printf("readlink%s\n", path);
// 	fflush(stdout);
// 	return 0;
// }

int hello_unlink(const char *path)
{
	// printf("unlink%s\n", path);
	// fflush(stdout);
	struct PKSFileInfo *Data = my_search(&ROOT, path);
	if (Data == NULL)
		return -ENOENT;
	my_erase(&ROOT, path);
	return 0;
}
static const struct fuse_operations hello_oper = {
	.init = hello_init,
	.getattr = hello_getattr,
	.readdir = hello_readdir,
	.open = hello_open,
	.read = hello_read,
	.mknod = hello_mknod,
	.mkdir = hello_mkdir,
	.rmdir = hello_rmdir,
	.write = hello_write,
	// .rename = hello_rename,
	// .truncate = hello_truncate,
	.utimens = hello_utimens,
	// .readlink = hello_readlink,
	.unlink = hello_unlink,
};

static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
		   "    --name=<s>          Name of the \"hello\" file\n"
		   "                        (default: \"hello\")\n"
		   "    --contents=<s>      Contents \"hello\" file\n"
		   "                        (default \"Hello, World!\\n\")\n"
		   "\n");
}

int main(int argc, char *argv[])
{
	// printf("%s",(int)&pks);
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	// options.filename = strdup("hello");
	// options.contents = strdup("Hello World!\n");s

	/* Parse options */
	/*if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;*/

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help)
	{
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}
	printf("fusebegin\n");
	fflush(stdout);
	my_insert(&ROOT, my_newnode("/", NULL, 1));
	ret = fuse_main(args.argc, args.argv, &hello_oper, NULL);
	fuse_opt_free_args(&args);
	return ret;
}
