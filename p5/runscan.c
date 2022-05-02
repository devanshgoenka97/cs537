#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "ext2_fs.h"
#include "read_ext2.h"

// Utility to check if image is jpg
int isjpeg(char* buffer) {
	int is_jpg = 0;

	// Check if the JPEG magic numbers match
	if (buffer[0] == (char)0xff &&
    	buffer[1] == (char)0xd8 &&
    	buffer[2] == (char)0xff &&
    	(buffer[3] == (char)0xe0 ||
     	buffer[3] == (char)0xe1 ||
     	buffer[3] == (char)0xe8)) {
		// Found JPEG image
		is_jpg = 1;
	}
	return is_jpg;
}

int isinodejpeg(int fd, off_t start_inode_table, int ino) {
	struct ext2_inode inode;

	// Read the inode from the disk
	read_inode(fd, start_inode_table, ino, &inode);

	char buffer[block_size];

	int read = read_data(fd, inode.i_block[0], buffer, block_size);

	if (read < 0 || !isjpeg(buffer)) {
		// Not a JPEG file
		return 0;
	}

	return 1;
}

//Utiliy to copy data from inode ino to outdir
int copydata(int fd, char* filename, off_t start_inode_table, int ino) {
	struct ext2_inode inode;

	// Read the inode from the disk
	read_inode(fd, start_inode_table, ino, &inode);

	// If not a regular file, skip
	if (!S_ISREG(inode.i_mode))
		return -1;

	// Nothing to copy
	if (inode.i_blocks == 0)
		return 0;

	// Create new file in out directory
	int fd2write = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	// Copy image to out directory
	int to_read = inode.i_size;

	// The current data block pointed to by the inode
	int j = 0;

	// Use direct pointers first
	while (j < EXT2_NDIR_BLOCKS && to_read > 0) {
		char buffer[block_size];

		// Copy the max(to_read, block_size) from the data blocks
		int read = read_data(fd, inode.i_block[j++], buffer, 
			to_read > (int)block_size ? (int)block_size : to_read);

		// Write to output file buffer
		write(fd2write, buffer, read);
		to_read -= read;
	}

	// Use the single indirect block to read
	if(to_read > 0) {
		unsigned int blocks[block_size/sizeof(unsigned int)];

		unsigned int* arr = blocks;

		// Read one block of indirect blocks
		read_data(fd, inode.i_block[EXT2_IND_BLOCK], (char *)blocks, block_size);

		int iterations = block_size/sizeof(unsigned int);

		while(iterations-- > 0 && to_read > 0) {
			char buffer[block_size];

			// Copy the max(to_read, block_size) from the data blocks
			int read = read_data(fd, *arr, buffer, 
				to_read > (int)block_size ? (int)block_size : to_read);

			// Write to output file buffer
			write(fd2write, buffer, read);
			to_read -= read;
			arr++;
		}

		// Need double indirect pointers
		if (to_read > 0) {
			unsigned int ind_blocks[block_size/sizeof(unsigned int)];
			unsigned int* ind_arr = ind_blocks;

			// Read one block of indirect blocks
			read_data(fd, inode.i_block[EXT2_DIND_BLOCK], (char *)ind_blocks, block_size);

			int out_iterations = block_size/sizeof(unsigned int);

			// Iterate over block of indirect pointers
			while(out_iterations-- > 0 && to_read > 0) {
				unsigned int blocks[block_size/sizeof(unsigned int)];
				unsigned int* arr = blocks;

				// Read the pointers in this block
				read_data(fd, *ind_arr, (char *)blocks, block_size);

				int iterations = block_size/sizeof(unsigned int);

				// Iterate over block of direct pointers and copy data
				while(iterations-- > 0 && to_read > 0) {
					char buffer[block_size];

					// Copy the max(to_read, block_size) from the data blocks
					int read = read_data(fd, *arr, buffer, 
						to_read > (int)block_size ? (int)block_size : to_read);

					// Write to output file buffer
					write(fd2write, buffer, read);
					to_read -= read;
					arr++;
				}

				ind_arr++;
			}
		}
	}

	close(fd2write);

	return 0;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}
	
	int fd;

	// Open disk image
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		printf("runScan: could not open disk image\n");
		exit(0);
	}

	// Fail if the output directory already exists
	if (opendir(argv[2]) != NULL) {
		printf("runScan: output directory already exists\n");
		exit(0);
	}

	// Create out directory
	int dir = mkdir(argv[2], S_IRWXU);
	if (dir < 0) {
		printf("runScan: could not create output directory\n");
		exit(0);
	}

	// Initialize the ext2 reader
	ext2_read_init(fd);

	struct ext2_super_block super;
	// Just read the first superblock
	read_super_block(fd, 0, &super);

	/* PART 1 */

	int global_ino = 0;

	// Iterate over all groups to find jpg files
	for (unsigned int ngroup = 0; ngroup < 1; ngroup++) {
		struct ext2_group_desc* group = (struct ext2_group_desc *) malloc(sizeof(struct ext2_group_desc));

		// Read the group descriptors
		read_group_desc(fd, ngroup, group);

		// Get the first inode table block in the group
		off_t start_inode_table = locate_inode_table(ngroup, group);

		// Iterate over all inodes to check for JPEGs
    	for (unsigned int ino = 1; ino < inodes_per_group; ino++) {
			// ino is relative to the block
			struct ext2_inode inode;

			// Read the inode from the disk
            read_inode(fd, start_inode_table, ino, &inode);

			if (inode.i_blocks == 0)
				continue;

			// If not a regular file, skip
			if (!S_ISREG(inode.i_mode))
				continue;

	        /* The maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)
			 * or once simplified, i_blocks/(2<<s_log_block_size)
			 * https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
			 */

			//unsigned int i_blocks = inode.i_blocks/(2<<super.s_log_block_size);

			char buffer[block_size];

			int read = read_data(fd, inode.i_block[0], buffer, block_size);

			if (read < 0 || !isjpeg(buffer)) {
				// Not a JPEG file
				continue;
			}

			//printf("runscan: found jpeg file at inode %u\n", ino + global_ino);

			char filename[255];
			sprintf(filename, "%s/file-%d.jpg", argv[2], ino + global_ino);

			// Copy data of this inode to the outfile
			copydata(fd, filename, start_inode_table, ino);
        }

		// Increment global count after iterating over group
		global_ino += inodes_per_group;
	}

	/* PART 2 */

	// Iterate over all directories to find jpg file names
	for (unsigned int ngroup = 0; ngroup < 1; ngroup++) {
		struct ext2_group_desc* group = (struct ext2_group_desc *) malloc(sizeof(struct ext2_group_desc));

		// Read the group descriptors
		read_group_desc(fd, ngroup, group);

		// Get the first inode table block in the group
		off_t start_inode_table = locate_inode_table(ngroup, group);

		// Iterate over all inodes in the group
    	for (unsigned int ino = 1; ino < inodes_per_group; ino++) {
			// ino is relative to the block
			struct ext2_inode inode;

			// Read the inode from the disk
            read_inode(fd, start_inode_table, ino, &inode);

			if (inode.i_blocks == 0)
				continue;

			// If not a directory, skip
			if (!S_ISDIR(inode.i_mode))
				continue;

			char buffer[block_size];

			// Read the first data block of directory, guaranteed to be in only one data block
			read_data(fd, inode.i_block[0], buffer, block_size);
			unsigned int offset = 0;

			while (offset < block_size) {

				// Read the first dir ent
				struct ext2_dir_entry* dentry = (struct ext2_dir_entry*) &(buffer[offset]);

				int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly

				if (name_len <= 0)
					break;

				char name[EXT2_NAME_LEN];
				strncpy(name, dentry->name, name_len);
				name[name_len] = '\0';

				// If inode is JPEG, copy to out directory

				// This ino is global ino, subtract from groups per block * ngroup to get relative
				int ino = dentry->inode;

				int rel_ino = ino - (inodes_per_group * ngroup);

				if (isinodejpeg(fd, start_inode_table, rel_ino)) {
					char filename[255 + EXT2_NAME_LEN];
					sprintf(filename, "%s/%s", argv[2], name);

					// Copy the  data contained in the inode to the outdir
					copydata(fd, filename, start_inode_table, rel_ino);
				}

				// Rounding up name_len to powers of 4
				name_len = ((name_len + 3)/4) * 4;

				// Finding hidden entries by offsetting through name_len instead of rec_len
				offset += name_len + 8;
			}
        }
	}
	
	close(fd);

	printf("runScan: done recovering jpeg images\n");
	return 0;
}
