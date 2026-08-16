/* patch.ld */
SECTIONS
{
    /* 定义自定义段，强制 8 字节对齐 */
    .xom : ALIGN(8)
    {
        /* 创建一个符号记录该段的起始地址（可选） */
        __xom_section_start = .;
        
        /* * KEEP 关键字防止开启 --gc-sections 时被删除
         * *(.my_section) 捕获所有 .o 文件中的 .my_section
         * *(.my_meta)    捕获我们在之前 GAS 实验中生成的元数据
         */
        /* KEEP(*(.my_section)) */
        KEEP(*(.xom))
        
        __xom_section_end = .;
    }
}

/* * 关键指令：将上述段插入到 .bss 段之后。
 * 在 ELF 文件结构中，这实际上就是 .symtab 之前的位置。
 */
INSERT AFTER .bss;