#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

// --- 配置区 ---
// 定义固定结构的大小和格式 (小端序)
#pragma pack(push, 1)
struct header_fixed {
    char magic[4];
    uint32_t name_size;
};

struct meta_header_fixed {
    uint32_t hardcoded_bytes_per_file;
    uint32_t bytes_frag_num;
    uint32_t direct_code_num;
    uint32_t direct_data_num;
    uint32_t fall_through_num;
};

struct meta_fixed {
    uint64_t frag_address;
    uint32_t frag_index;
    uint32_t frag_size;
    uint8_t fr_flags[4];
    uint32_t symbol_size;
};
#pragma pack(pop)

const char MAGIC_SIG[4] = {'x', 'o', 'm', '\0'};

void parse_xom(const uint8_t *data, uint64_t data_len, uint64_t text_size, const char *log_file_path) {
    uint64_t offset = 0;
    uint32_t entry_count = 0;
    uint64_t embeded_data_size = 0;
    uint32_t data_block = 0;

    uint32_t total_hardcode_size = 0;
    uint32_t total_hardcode_block = 0;
    uint32_t total_direct_code_num = 0;
    uint32_t total_direct_data_num = 0;
    uint32_t total_fall_through_num = 0;

    FILE *log = fopen(log_file_path, "w");
    if (!log) {
        fprintf(stderr, "[!] 无法打开日志文件进行写入: %s\n", log_file_path);
        return;
    }

    fprintf(log, "=== 🚀 开始解析 .xom 段 (总大小: %lu 字节) ===\n\n", data_len);

    while (offset < data_len) {
        if (data_len - offset < 4) {
            break;
        }

        // 1. 处理 header
        if (memcmp(data + offset, MAGIC_SIG, 4) == 0) {
            if (data_len - offset < sizeof(struct header_fixed)) {
                fprintf(log, "[ERROR] 数据太短，无法解析 Header。\n");
                break;
            }

            struct header_fixed *hdr = (struct header_fixed *)(data + offset);
            offset += sizeof(struct header_fixed);

            // 提取文件名
            char file_name[256] = {0};
            if (hdr->name_size > 0) {
                if (offset + hdr->name_size > data_len) {
                    fprintf(log, "[ERROR] 文件名长度越界 (需要 %u 字节)。\n", hdr->name_size);
                    break;
                }
                uint32_t copy_size = hdr->name_size < 255 ? hdr->name_size : 255;
                memcpy(file_name, data + offset, copy_size);
                offset += hdr->name_size;
            }

            if (data_len - offset < sizeof(struct meta_header_fixed)) {
                fprintf(log, "[ERROR] 数据太短，无法解析 Meta Header。\n");
                break;
            }

            struct meta_header_fixed *meta_hdr = (struct meta_header_fixed *)(data + offset);
            offset += sizeof(struct meta_header_fixed);

            fprintf(log, "--- [ Header ] ---\n");
            fprintf(log, "Magic      : %s\n", hdr->magic);
            fprintf(log, "Name Size  : %u bytes\n", hdr->name_size);
            fprintf(log, "File Name  : %s\n\n", strlen(file_name) > 0 ? file_name : "<empty>");
            fprintf(log, "hardcoded_bytes_per_file  : %u  bytes\n", meta_hdr->hardcoded_bytes_per_file);
            fprintf(log, "bytes_frag_num  : %u\n", meta_hdr->bytes_frag_num);
            fprintf(log, "direct_code_num  : %u\n", meta_hdr->direct_code_num);
            fprintf(log, "direct_data_num  : %u\n", meta_hdr->direct_data_num);
            fprintf(log, "fall_through_num : %u\n\n", meta_hdr->fall_through_num);

            total_hardcode_size += meta_hdr->hardcoded_bytes_per_file;
            total_hardcode_block += meta_hdr->bytes_frag_num;
            total_direct_code_num += meta_hdr->direct_code_num;
            total_direct_data_num += meta_hdr->direct_data_num;
            total_fall_through_num += meta_hdr->fall_through_num;
        } 
        // 2. 解析 Metadata 结构体
        else {
            fprintf(log, "--- [ Metadata Entries ] ---\n");

            if (offset + sizeof(struct meta_fixed) > data_len) {
                fprintf(log, "\n[WARNING] 剩余数据 (%lu 字节) 不足以解析下一个完整的 Metadata 固定头。\n", data_len - offset);
                break;
            }

            struct meta_fixed *meta = (struct meta_fixed *)(data + offset);
            offset += sizeof(struct meta_fixed);

            // 提取 Symbol Name
            char symbol_name[256] = {0};
            if (meta->symbol_size > 0) {
                if (offset + meta->symbol_size > data_len) {
                    fprintf(log, "\n[ERROR] Entry #%u: Symbol 长度越界 (需要 %u 字节)。\n", entry_count, meta->symbol_size);
                    break;
                }
                uint32_t copy_size = meta->symbol_size < 255 ? meta->symbol_size : 255;
                memcpy(symbol_name, data + offset, copy_size);
                offset += meta->symbol_size;
            }

            fprintf(log, "[Entry #%u]\n", entry_count);
            fprintf(log, "  Frag Address : 0x%016lx\n", meta->frag_address);
            fprintf(log, "  Frag Index   : %u\n", meta->frag_index);
            fprintf(log, "  Frag Size    : %u (0x%x) bytes\n", meta->frag_size, meta->frag_size);
            fprintf(log, "  Frag Flags   : 0x%02x%02x%02x%02x\n", meta->fr_flags[0], meta->fr_flags[1], meta->fr_flags[2], meta->fr_flags[3]);
            fprintf(log, "  Symbol Size  : %u bytes\n", meta->symbol_size);
            fprintf(log, "  Symbol Name  : %s\n", strlen(symbol_name) > 0 ? symbol_name : "<None>");
            fprintf(log, "----------------------------------------\n");

            entry_count++;
            embeded_data_size += meta->frag_size;
            
            if (meta->frag_size != 0 && strcmp(symbol_name, "null") != 0) {
                data_block++;
            }
        }
    }

    // --- 统计计算与打印 ---
    char ratio_str[64];
    if (text_size > 0) {
        double ratio_val = (double)embeded_data_size / text_size;
        double ratio_pct = ratio_val * 100.0;

        if (ratio_pct == 0.0) {
            snprintf(ratio_str, sizeof(ratio_str), "0.00%%");
        } else if (ratio_pct < 0.0001) {
            snprintf(ratio_str, sizeof(ratio_str), "%.2e%%", ratio_pct);
        } else if (ratio_pct < 0.01) {
            snprintf(ratio_str, sizeof(ratio_str), "%.6f%%", ratio_pct);
        } else if (ratio_pct < 1.0) {
            snprintf(ratio_str, sizeof(ratio_str), "%.4f%%", ratio_pct);
        } else {
            snprintf(ratio_str, sizeof(ratio_str), "%.2f%%", ratio_pct);
        }
    } else {
        snprintf(ratio_str, sizeof(ratio_str), "N/A (Size is 0)");
    }

    double average_size = 0;
    if (data_block > 0) {
        average_size = (double)embeded_data_size / data_block;
    }

    uint32_t total_code_block = total_hardcode_block - data_block;
    uint32_t total_indirect_code_num = total_code_block - total_direct_code_num - total_fall_through_num;
    uint32_t total_indirect_data_num = data_block - total_direct_data_num;

    fprintf(log, "\n=== ✅ 解析完成，共成功解析 %u 个元数据条目。 ===\n", entry_count);
    fclose(log);

    const char *border_line = "+--------------------------------+-----------------------------------+";
    printf("\n%s\n", border_line);
    printf("| %-30s | %-33s |\n", "METRIC CATEGORY", "VALUE");
    printf("%s\n", border_line);

    char val_buf[64];
    
    snprintf(val_buf, sizeof(val_buf), "%u", data_block);
    printf("| %-30s | %-33s |\n", "Total Embedded Data Count", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%lu", embeded_data_size);
    printf("| %-30s | %-33s |\n", "Total Embedded Data Size", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%lu", text_size);
    printf("| %-30s | %-33s |\n", "Actual Text Size", val_buf);
    printf("%s\n", border_line);

    printf("| %-30s | %-33s |\n", "Embedded / Actual Size Ratio", ratio_str);
    
    snprintf(val_buf, sizeof(val_buf), "%g", average_size); // 自动处理格式
    printf("| %-30s | %-33s |\n", "Average Embedded Data Size", val_buf);
    printf("%s\n", border_line);

    snprintf(val_buf, sizeof(val_buf), "%u", total_hardcode_size);
    printf("| %-30s | %-33s |\n", "Total Hardcode Size", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%u", total_hardcode_block);
    printf("| %-30s | %-33s |\n", "Total Hardcode Block", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%u", total_direct_code_num);
    printf("| %-30s | %-33s |\n", "Total Direct code Num", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%u", total_direct_data_num);
    printf("| %-30s | %-33s |\n", "Total Direct Data Num", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%u", total_fall_through_num);
    printf("| %-30s | %-33s |\n", "Total Fall Through Num", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%u", total_indirect_code_num);
    printf("| %-30s | %-33s |\n", "Total Indirect Code Num", val_buf);
    
    snprintf(val_buf, sizeof(val_buf), "%u", total_indirect_data_num);
    printf("| %-30s | %-33s |\n", "Total Indirect Data Num", val_buf);
    
    printf("%s\n\n", border_line);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <elf_file> [--section <name>] [--ldlog <file>] [--asonly <yes/no>]\n", argv[0]);
        return 1;
    }

    const char *elf_path = NULL;
    const char *section_name = ".my_section";
    const char *ldlog_path = "xom_parse.log"; // 默认log名称
    const char *asonly = "no";

    // 简单解析参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--section") == 0 && i + 1 < argc) {
            section_name = argv[++i];
        } else if (strcmp(argv[i], "--ldlog") == 0 && i + 1 < argc) {
            ldlog_path = argv[++i];
        } else if (strcmp(argv[i], "--asonly") == 0 && i + 1 < argc) {
            asonly = argv[++i];
        } else if (argv[i][0] != '-') {
            elf_path = argv[i];
        }
    }

    if (!elf_path) {
        fprintf(stderr, "[!] 缺少目标 ELF 文件路径\n");
        return 1;
    }

    printf("read log file: %s\n", ldlog_path);
    printf("read elf file: %s\n", elf_path);

    int fd = open(elf_path, O_RDONLY);
    if (fd < 0) {
        perror("[!] Error opening file");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("[!] Error getting file size");
        close(fd);
        return 1;
    }

    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("[!] Error mapping file");
        close(fd);
        return 1;
    }

    // 检查 ELF Magic (\x7fELF)
    if (memcmp(map_start, ELFMAG, SELFMAG) != 0) {
        printf("Skip: %s -> is a Wrapper bash! (或非 ELF 文件)\n", elf_path);
        munmap(map_start, st.st_size);
        close(fd);
        return 0;
    }

    // 假定为 64 位 ELF (针对 x86_64 架构)
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)map_start;
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "[!] 目前该代码仅支持解析 64 位 ELF 文件。\n");
        munmap(map_start, st.st_size);
        close(fd);
        return 1;
    }

    Elf64_Shdr *shdrs = (Elf64_Shdr *)((uint8_t *)map_start + ehdr->e_shoff);
    Elf64_Shdr *sh_strtab = &shdrs[ehdr->e_shstrndx];
    const char *sh_strtab_p = (const char *)map_start + sh_strtab->sh_offset;

    uint64_t total_exec_size = 0;
    const uint8_t *target_section_data = NULL;
    uint64_t target_section_size = 0;

    // 遍历段表，查找总 Executable 大小以及指定段的数据
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdrs[i].sh_flags & SHF_EXECINSTR) {
            total_exec_size += shdrs[i].sh_size;
        }

        const char *name = sh_strtab_p + shdrs[i].sh_name;
        if (strcmp(name, section_name) == 0) {
            target_section_data = (uint8_t *)map_start + shdrs[i].sh_offset;
            target_section_size = shdrs[i].sh_size;
        }
    }

    if (target_section_data) {
        parse_xom(target_section_data, target_section_size, total_exec_size, ldlog_path);
    } else {
        fprintf(stderr, "[!] Section '%s' not found in %s\n", section_name, elf_path);
    }

    // 资源清理
    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}