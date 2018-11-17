#include <stdio.h>


int main()
{
    int res, x, y;
    char buf;

    /* for (;;) { */
        printf("enter x, y, char...");
        res = scanf("%d%d %c", &x, &y, &buf);
        printf("\n\nres = %d\n(x, y) = (%d, %d)\nbuf = %c\n",
                res, x, y, buf);
    /* } */

    return 0;
}
