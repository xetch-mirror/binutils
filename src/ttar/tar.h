#ifndef TAR_H
#define TAR_H

#include "syscalls.h"

int my_strcmp(char *str1, char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

void print_str(char *message) {
    char *start = message;
    while (*message) {
        message++;
    }
    sys_write(1, start, (unsigned long)(message - start));
}

unsigned long oct2bin(char *octal_string, int string_length) {
    unsigned long result_value = 0;
    int index = 0;
    while (index < string_length && octal_string[index] >= '0' && octal_string[index] <= '7') {
        result_value = (result_value << 3) + (octal_string[index] - '0');
        index++;
    }
    return result_value;
}

void bin2oct(unsigned long binary_value, char *dest_octal_string, int dest_size) {
    int index = dest_size - 1;
    dest_octal_string[index] = '\0';
    index--;
    for (; index >= 0; index--) {
        dest_octal_string[index] = (char)('0' + (binary_value & 7));
        binary_value >>= 3;
    }
}

int copy_name(char *dest, char *src, int max_len) {
    int i = 0;

    while (src[i] != '\0' && i < (max_len - 1)) {
        dest[i] = src[i];
        i++;
    }

    /* Ставимо нуль-термінатор в кінець */
    dest[i] = '\0';

    return i;
}


struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

void set_checksum(struct tar_header *header) {
    unsigned int sum = 0;
    unsigned long i;

    for (i = 0; i < 8; i++) {
        header->chksum[i] = ' ';
    }

    for (i = 0; i < 512; i++) {
        sum += ((unsigned char *)header)[i];
    }

    bin2oct(sum, header->chksum, 7);
    header->chksum[6] = '\0';
    header->chksum[7] = ' ';
}


void force_directories(char *path) {
    char temp_path[512];
    int i = 0;
    while (path[i] != '\0' && i < 510) {
        temp_path[i] = path[i];
        if (temp_path[i] == '/') {
            temp_path[i] = '\0';
            sys_mkdir(temp_path, 0755);
            temp_path[i] = '/';
        }
        i++;
    }
}

void add_to_archive(int archive_fd, char *path) {
    struct stat_custom st;
    struct tar_header header;
    unsigned long byte_index;

    if (sys_stat(path, &st) < 0) {
        sys_exit(1);
    }

    {
        struct block64 {
            unsigned long data[8];
        };

        struct block64 *header_blocks = (struct block64 *)&header;
        int idx;

        /* Clean deployment to avoid implicit memset calls */
        struct block64 empty_block;
        empty_block.data[0] = 0;
        empty_block.data[1] = 0;
        empty_block.data[2] = 0;
        empty_block.data[3] = 0;
        empty_block.data[4] = 0;
        empty_block.data[5] = 0;
        empty_block.data[6] = 0;
        empty_block.data[7] = 0;

        /* we do a total of 8 loop steps*/
        for (idx = 0; idx < 8; idx++) {
            header_blocks[idx] = empty_block;
        }
    }



    copy_name(header.name, path, 100);
    bin2oct(STAT_MODE(&st) & 0777, header.mode, 8);
    bin2oct(STAT_UID(&st), header.uid, 8);
    bin2oct(STAT_GID(&st), header.gid, 8);
    bin2oct(STAT_MTIME(&st), header.mtime, 12);

    /* We record 'u', 's', 't', 'a', 'r', '\0', '0', '0' with one 64-bit record */
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        *(unsigned long *)&header.magic[0] = 0x3030007261747375;
    #else
        *(unsigned long *)&header.magic[0] = 0x7573746172003030;
    #endif

    /* It is possible to use this metadata for future functions */
    int len = copy_name(header.name, path, 100);

    if (S_ISDIR(STAT_MODE(&st))) {

        if (len < 99 && (len == 0 || header.name[len - 1] != '/')) {
            header.name[len] = '/';
            header.name[len + 1] = '\0';
        }


        header.typeflag = '5';

        *(unsigned long *)&header.size[0] = 0;
        *(unsigned int *)&header.size[8] = 0;

        set_checksum(&header);
        sys_write(archive_fd, &header, 512);

        {
            int dir_fd = sys_open(path, O_RDONLY, 0);
            if (dir_fd >= 0) {
                char d_buf[4096];
                long nread;
                while ((nread = sys_getdents64(dir_fd, d_buf, 4096)) > 0) {
                    long bpos = 0;
                    char sub_path[512];

                    while (bpos < nread) {
                        struct linux_dirent64 *d = (struct linux_dirent64 *)(d_buf + bpos);
                        if (d->d_name[0] != '.' || (d->d_name[1] != '\0' && (d->d_name[1] != '.' || d->d_name[2] != '\0'))) {
                            int p_idx = 0;

                            while (path[p_idx] != '\0' && p_idx < 250) {
                                sub_path[p_idx] = path[p_idx];
                                p_idx++;
                            }
                            if (p_idx > 0 && sub_path[p_idx - 1] != '/') {
                                sub_path[p_idx++] = '/';
                            }

                            {
                                int name_idx = 0;
                                while (d->d_name[name_idx] != '\0' && p_idx < 510) {
                                    sub_path[p_idx++] = d->d_name[name_idx++];
                                }
                            }
                            sub_path[p_idx] = '\0';

                            add_to_archive(archive_fd, sub_path);
                        }
                        bpos += d->d_reclen;
                    }
                }
                sys_close(dir_fd);
            }
        }
    } else {
        int file_fd;
        char buffer[512];
        long read_bytes;

        header.typeflag = '0';
        bin2oct(STAT_SIZE(&st), header.size, 12);
        set_checksum(&header);
        sys_write(archive_fd, &header, 512);

        file_fd = sys_open(path, O_RDONLY, 0);
        if (file_fd < 0) {
            sys_exit(1);
        }

        while ((read_bytes = sys_read(file_fd, buffer, 512)) > 0) {
            if (read_bytes < 512) {
                for (byte_index = (unsigned long)read_bytes; byte_index < 512; byte_index++) {
                    buffer[byte_index] = 0;
                }
            }
            sys_write(archive_fd, buffer, 512);
        }
        sys_close(file_fd);
    }
}

int create_archive(char *archive_name, int file_count, char **files) {
    int archive_fd;

    archive_fd = sys_open(archive_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (archive_fd < 0) return 1;

    while (file_count--) {
        add_to_archive(archive_fd, *files++);
    }

    {
        /* I originally thought that manually listing all zeros would prevent GCC from generating an implicit memset(), but in this build the opposite happened: the manually expanded zero initializer triggered a generated memset(), while {0} did not. */
        unsigned long zero_block[] = {0};
        sys_write(archive_fd, zero_block, 1024);
    }

    sys_close(archive_fd);
    print_str("Successfully.\n");
    return 0;
}

int extract_archive(char *archive_name) {
    int archive_fd;
    struct tar_header header;

    archive_fd = sys_open(archive_name, O_RDONLY, 0);
    if (archive_fd < 0) {
        return 1;
    }

    while (sys_read(archive_fd, &header, 512) == 512) {
        unsigned long file_size;
        unsigned long blocks_count;
        unsigned long j;
        unsigned int file_mode;

        if (header.name[0] == '\0') {
            break;
        }

        file_size = oct2bin(header.size, 12);
        blocks_count = (file_size + 511) / 512;
        file_mode = (unsigned int)oct2bin(header.mode, 8);

        force_directories(header.name);

        if (header.typeflag == '5') {
            sys_mkdir(header.name, file_mode ? file_mode : 0755);
        } else {
            int out_fd = sys_open(header.name, O_WRONLY | O_CREAT | O_TRUNC, file_mode ? file_mode : 0644);
            if (out_fd < 0) {
                return 1;
            }

            char data_block[512];
            for (j = 0; j < blocks_count; j++) {
                if (sys_read(archive_fd, data_block, 512) != 512) {
                    return 1;
                }

                if (j == blocks_count - 1) {
                    unsigned long remaining = file_size % 512;
                    if (remaining == 0) {
                        remaining = 512;
                    }
                    sys_write(out_fd, data_block, remaining);
                } else {
                    sys_write(out_fd, data_block, 512);
                }
            }
            sys_close(out_fd);
        }
    }

    sys_close(archive_fd);
    print_str("Successfully!\n");
    return 0;
}

#endif