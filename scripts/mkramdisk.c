/*
 * Ramdisk image builder tool
 * Builds a ramdisk image from a directory of files
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define RAMDISK_MAGIC 0x444B4D52 // 'RMDK'

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint32_t file_count;
    uint64_t total_size;
} ramdisk_header_t;

typedef struct {
    char name[64];
    uint64_t offset;
    uint64_t size;
    uint32_t flags;
} ramdisk_entry_t;
#pragma pack(pop)

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_dir> <output_ramdisk>\n", argv[0]);
        return 1;
    }

    const char *input_dir = argv[1];
    const char *output_file = argv[2];

    // Count files in directory
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "find %s -type f | wc -l", input_dir);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        return 1;
    }
    int file_cnt = 0;
    fscanf(fp, "%d", &file_cnt);
    pclose(fp);

    if (file_cnt == 0) {
        fprintf(stderr, "No files found in %s\n", input_dir);
        return 1;
    }

    // Collect file info
    struct {
        char name[64];
        char path[512];
        uint64_t size;
    } *files = malloc(file_cnt * sizeof(*files));

    int idx = 0;
    snprintf(cmd, sizeof(cmd), "find %s -type f", input_dir);
    fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        return 1;
    }

    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < file_cnt) {
        line[strcspn(line, "\n")] = 0;
        strcpy(files[count].path, line);
        const char *base = strrchr(line, '/');
        strncpy(files[count].name, base ? base + 1 : line, 63);
        files[count].name[63] = 0;

        struct stat st;
        stat(line, &st);
        files[count].size = st.st_size;
        count++;
    }
    pclose(fp);

    // Calculate total size
    uint64_t total_data_size = 0;
    for (int i = 0; i < count; i++) {
        total_data_size += files[i].size;
    }

    // Calculate offsets
    uint64_t header_size = sizeof(uint32_t) * 3; // magic, file_count, total_size
    uint64_t entries_size = count * (64 + 8 + 8 + 4); // name + offset + size + flags
    uint64_t data_offset = header_size + entries_size;
    data_offset = (data_offset + 7) & ~7; // Align to 8 bytes

    uint64_t total_data_sz = 0;
    for (int i = 0; i < count; i++) {
        total_data_sz += files[i].size;
    }

    // Write ramdisk
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        perror("fopen output");
        return 1;
    }

    // Write header
    uint32_t magic = 0x444B4D52; // 'RMDK'
    uint32_t fc = count;
    uint64_t total_size = sizeof(uint32_t) * 3 + count * (64 + 8 + 8 + 4);
    total_size = (total_size + 7) & ~7;
    total_size += 0;
    for (int i = 0; i < count; i++) {
        total_size += files[i].size;
    }

fwrite(&magic, 1, 4, out);
        fwrite(&fc, 4, 1, out);
        fwrite(&total_size, 8, 1, out);

    // Write entries
    uint64_t current_offset = sizeof(uint32_t) * 3 + count * (64 + 8 + 8 + 4);
    current_offset = (current_offset + 7) & ~7;

    for (int i = 0; i < count; i++) {
        ramdisk_entry_t entry = {0};
        strncpy(entry.name, files[i].name, 63);
        entry.size = files[i].size;
        entry.offset = current_offset;
        entry.flags = 0;
        fwrite(&entry, 1, sizeof(ramdisk_entry_t), out);
        current_offset += files[i].size;
    }

    // Align data start
    uint64_t current_pos = ftell(out);
    uint64_t data_start = (current_offset + 7) & ~7;
    if (data_start > current_pos) {
        uint8_t padding[8] = {0};
        fwrite(padding, 1, data_start - current_pos, out);
    }

    // Write file data
    for (int i = 0; i < count; i++) {
        FILE *in = fopen(files[i].path, "rb");
        if (!in) {
            fprintf(stderr, "Failed to open %s\n", files[i].path);
            fclose(out);
            return 1;
        }

        uint8_t buffer[4096];
        size_t read_bytes;
        while ((read_bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
            fwrite(buffer, 1, read_bytes, out);
        }
        fclose(in);
    }

    fclose(out);

    printf("Created ramdisk: %s (%d files, %lu bytes)\n", output_file, count, (unsigned long)ftell(out));

    free(files);
    return 0;
}