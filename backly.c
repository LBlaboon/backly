/*
 * backly - A simple directory cloner
 * Copyright (C) 2015  L. Bradley LaBoon <me@bradleylaboon.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void printUsage(FILE *stream)
{
	fprintf(stream, "Usage: backly [options] <srcdir> <destdir>\n");
}

void printHelp()
{
	printUsage(stdout);
	printf("Turn destdir into a clone of srcdir.\n\n");
	
	printf("Options:\n");
	printf(" --test\tRun in test mode.\n");
	printf(" --help\tPrint this help and quit.\n\n");
	
	printf("Report bugs to L. Bradley LaBoon <me@bradleylaboon.com>\n");
	printf("backly home page: <http://git.bradleylaboon.com/backly.git>\n");
}

void removeMissing(char *src, int srcPrefix, char *dest, int destPrefix, int testMode)
{
	int srcLen = 0, destLen = 0;
	while (src[srcLen])
		srcLen++;
	while (dest[destLen])
		destLen++;
	
	DIR *destDir = opendir(dest);
	if (destDir == NULL) {
		fprintf(stderr, "Could not open %s: %s\n", dest, strerror(errno));
		return;
	}
	
	// If the entire directory doesn't exist in src, remove it from dest
	DIR *srcDir = opendir(src);
	if (srcDir == NULL) {
		if (errno == ENOENT) {
			printf("- %s", dest + destPrefix);
			fflush(stdout);
			if (testMode == 0) {
				int pid = fork();
				if (pid == -1) {
					fprintf(stderr, "Could not fork: %s\n", strerror(errno));
					closedir(destDir);
					return;
				} else if (pid == 0) {
					execlp("rm", "rm", "-rf", dest, (char *) NULL);
				} else {
					wait(NULL);
				}
			}
			printf("\n");
			closedir(destDir);
			return;
		} else {
			fprintf(stderr, "Could not open %s: %s\n", src, strerror(errno));
			closedir(destDir);
			return;
		}
	}
	closedir(srcDir);
	
	// Look at each item in the folder
	struct dirent *item;
	while ((item = readdir(destDir)) != NULL) {
		// Ignore . and .. references
		if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
			continue;
		
		int itemLen = 0;
		while (item->d_name[itemLen])
			itemLen++;
		
		// +2 because a trailing slash might be added
		char itemSrc[srcLen + itemLen + 2];
		char itemDest[destLen + itemLen + 2];
		sprintf(itemSrc, "%s%s", src, item->d_name);
		sprintf(itemDest, "%s%s", dest, item->d_name);
		
		// Test if item is a directory
		int itemIsDir = 1;
		DIR *testDir = opendir(itemDest);
		if (testDir == NULL && errno == ENOTDIR)
			itemIsDir = 0;
		else if (testDir != NULL)
			closedir(testDir);
		
		if (itemIsDir == 1) {
			// If it's a directory, append trailing slashes and recurse
			itemSrc[srcLen + itemLen] = '/';
			itemSrc[srcLen + itemLen + 1] = '\0';
			itemDest[destLen + itemLen] = '/';
			itemDest[destLen + itemLen + 1] = '\0';
			removeMissing(itemSrc, srcPrefix, itemDest, destPrefix, testMode);
		} else {
			// If the file doesn't exist in src, remove it from dest
			FILE *srcFile = fopen(itemSrc, "r");
			if (srcFile == NULL) {
				if (errno == ENOENT) {
					printf("- %s", itemDest + destPrefix);
					fflush(stdout);
					if (testMode == 0) {
						int pid = fork();
						if (pid == -1) {
							fprintf(stderr, "Could not fork: %s\n", strerror(errno));
							continue;
						} else if (pid == 0) {
							execlp("rm", "rm", "-f", itemDest, (char *) NULL);
						} else {
							wait(NULL);
						}
					}
					printf("\n");
					continue;
				} else {
					fprintf(stderr, "Could not open %s: %s\n", itemSrc, strerror(errno));
					continue;
				}
			} else {
				fclose(srcFile);
			}
		}
	}
	
	closedir(destDir);
	return;
}

void copyNew(char *src, int srcPrefix, char *dest, int destPrefix, int testMode)
{
	int srcLen = 0, destLen = 0;
	while (src[srcLen])
		srcLen++;
	while(dest[destLen])
		destLen++;
	
	DIR *srcDir = opendir(src);
	if (srcDir == NULL) {
		fprintf(stderr, "Could not open %s: %s\n", src, strerror(errno));
		return;
	}
	
	// If the dest directory doesn't exist, create it
	DIR *destDir = opendir(dest);
	if (destDir == NULL) {
		if (errno == ENOENT) {
			printf("+ %s", dest + destPrefix);
			fflush(stdout);
			if (testMode == 0) {
				int pid = fork();
				if (pid == -1) {
					fprintf(stderr, "Could not fork: %s:\n", strerror(errno));
					closedir(srcDir);
					return;
				} else if (pid == 0) {
					execlp("mkdir", "mkdir", dest, (char *) NULL);
				} else {
					wait(NULL);
				}
			}
			printf("\n");
		} else {
			fprintf(stderr, "Could not open %s: %s\n", dest, strerror(errno));
			closedir(srcDir);
			return;
		}
	}
	closedir(destDir);
	
	// Look at each item in the folder
	struct dirent *item;
	while ((item = readdir(srcDir)) != NULL) {
		if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
			continue;
		
		int itemLen = 0;
		while (item->d_name[itemLen])
			itemLen++;
		
		char itemSrc[srcLen + itemLen + 2];
		char itemDest[destLen + itemLen + 2];
		sprintf(itemSrc, "%s%s", src, item->d_name);
		sprintf(itemDest, "%s%s", dest, item->d_name);
		
		// Test if item is a directory
		int itemIsDir = 1;
		DIR *testDir = opendir(itemSrc);
		if (testDir == NULL && errno == ENOTDIR)
			itemIsDir = 0;
		else if (testDir != NULL)
			closedir(testDir);
		
		if (itemIsDir == 1) {
			// If it's a directory, recurse
			itemSrc[srcLen + itemLen] = '/';
			itemSrc[srcLen + itemLen + 1] = '\0';
			itemDest[destLen + itemLen] = '/';
			itemDest[destLen + itemLen + 1] = '\0';
			copyNew(itemSrc, srcPrefix, itemDest, destPrefix, testMode);
		} else {
			// Check if file exists in dest
			int needToCopy = 0;
			FILE *destFile = fopen(itemDest, "r");
			if (destFile == NULL) {
				if (errno == ENOENT) {
					needToCopy = 1;
				} else {
					fprintf(stderr, "Could not open %s: %s\n", itemDest, strerror(errno));
					continue;
				}
			} else {
				fclose(destFile);
				
				// Check if file size or modified time is different
				struct stat srcStat, destStat;
				if (stat(itemSrc, &srcStat) == -1) {
					fprintf(stderr, "Could not stat %s\n", itemSrc);
					continue;
				}
				if (stat(itemDest, &destStat) == -1) {
					fprintf(stderr, "Could not stat %s\n", itemDest);
					continue;
				}
				
				if (srcStat.st_size != destStat.st_size)
					needToCopy = 1;
				else if (srcStat.st_mtime > destStat.st_mtime)
					needToCopy = 1;
			}
			
			// Only copy file if it doesn't exist or has changed
			if (needToCopy == 1) {
				printf("+ %s ", itemSrc + srcPrefix);
				fflush(stdout);
				if (testMode == 0) {
					int pid = fork();
					if (pid == -1) {
						fprintf(stderr, "Could not fork: %s\n", strerror(errno));
						continue;
					} else if (pid == 0) {
						execlp("cp", "cp", "-fp", itemSrc, itemDest, (char *) NULL);
					} else {
						struct stat srcStat, destStat;
						double pDone = 0;
						int numPrinted = 0;
						if (stat(itemSrc, &srcStat) != -1) {
							while (waitpid(pid, NULL, WNOHANG) == 0) {
								if (stat(itemDest, &destStat) == -1)
									continue;
								
								pDone = (double)destStat.st_size / (double)srcStat.st_size;
								
								for (int i = 0; i < numPrinted; i++)
									printf("\b \b");
								numPrinted = printf("%.2lf%%", pDone * 100);
								fflush(stdout);
							}
						}
					}
				}
				printf("\n");
			}
		}
	}
	
	closedir(srcDir);
	return;
}

int main(int argc, char **argv)
{
	int srcArg = 0, destArg = 0;
	int testMode = 0;
	
	// Parse arguments
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			printHelp();
			exit(0);
		} else if (strcmp(argv[i], "--test") == 0) {
			testMode = 1;
			printf("***Operating in test mode.***\n");
		} else if (srcArg == 0) {
			srcArg = i;
		} else {
			destArg = i;
		}
	}
	
	if (srcArg == 0 || destArg == 0) {
		printUsage(stderr);
		fprintf(stderr, "Run 'backly --help' for more information.\n");
		exit(1);
	}
	
	// Determine argument lengths
	int srcLen = 0, destLen = 0;
	while (argv[srcArg][srcLen])
		srcLen++;
	while (argv[destArg][destLen])
		destLen++;
	
	// Add trailing slashes to directory arguments if necessary
	char srcDir[srcLen + 2], destDir[destLen + 2];
	
	if (argv[srcArg][srcLen - 1] == '/')
		sprintf(srcDir, "%s", argv[srcArg]);
	else
		sprintf(srcDir, "%s/", argv[srcArg]);
	
	if (argv[destArg][destLen - 1] == '/')
		sprintf(destDir, "%s", argv[destArg]);
	else
		sprintf(destDir, "%s/", argv[destArg]);
	
	// Check if directories exist and can be accessed
	DIR *dir = opendir(srcDir);
	if (dir == NULL) {
		fprintf(stderr, "Could not open source: %s\n", strerror(errno));
		exit(1);
	}
	closedir(dir);
	
	dir = opendir(destDir);
	if (dir == NULL) {
		fprintf(stderr, "Could not open destination: %s\n", strerror(errno));
		exit(1);
	}
	closedir(dir);
	
	// Remove files from the destination that don't exist in the source
	removeMissing(srcDir, strlen(srcDir), destDir, strlen(destDir), testMode);
	
	// Copy new files and overwrite existing files if different
	copyNew(srcDir, strlen(srcDir), destDir, strlen(destDir), testMode);
	
	return 0;
}
