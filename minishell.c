#include <stdio.h>
#include <string.h>
#define INP_SIZE_MAX    1024
int main(void)
{
    char inp[INP_SIZE_MAX];
    memset(inp,0,sizeof(inp));    
	while(1)    {
        printf("msh>");
        fgets(inp, INP_SIZE_MAX, stdin);
        printf("%s",inp);
    }
	return 0;
}
