#include "../include/fin.h"
#include <stdio.h>

int main(int argc, const char* argv[]) {
    for (int i=0; i<argc; i++)
        printf("%s\n", argv[i]);
    fin_ctx* ctx = fin_ctx_create_default();
    if (argc == 1)
        fin_ctx_eval_str(ctx, "void Main() { io.WriteLine(\"Hello, world!\"); }");
    else
        fin_ctx_eval_file(ctx, argv[1]);
    fin_ctx_destroy(ctx);
}
