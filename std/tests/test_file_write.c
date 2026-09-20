//~ ak: Includes
//=============================================================================

//- ak: headers
#include "../base/base_include.h"
#include "../os/os_include.h"

//- ak: implementation
#include "../base/base_include.c"
#include "../os/os_include.c"

void base_main(void)
{
    Os_File h_file = os_file_open(str8("./build/test_file_write.txt"), Os_AccessFlag_Write);
    fmt_fprintfln(h_file, "//- GENERATED CODE\n");
    fmt_fprintfln(h_file, "#ifndef _META_H");
    os_file_close(h_file);
}
