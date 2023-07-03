#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
// Driver Code
int main()
{
    // Substitute the full file path
    // for the string file_path
    FILE* fp = fopen("iframe.csv", "r");
 
    if (!fp)
        printf("Can't open file\n");
 
    else {
        // Here we have taken size of
        // array 1024 you can modify it
 
        int row = 0;
        int column = 0;

        char *line = NULL;
        int header_max_length = 0;
        int header_num = 0;
        char *headers_pointer[100];
        {
            size_t len = 0;
            size_t read;

            read = getline(&line, &len, fp);

            if (read == -1) {
                exit(EXIT_FAILURE);
            }

            printf("Retrieved line of length %zu :\n", read);
            printf("%s\n\n", line);

            char* value = strtok(line, ",");
            printf(": %s\n", value);
            while (value) {
                printf(": %s\n", value);
                if (header_max_length < strlen(value)) {
                    header_max_length = strlen(value) + 1;
                }
                headers_pointer[header_num] = value;
                ++header_num;
                value = strtok(NULL, ",");
            }
        }
        printf("header_max_length %d\n", header_max_length);
        printf("header_num %d\n", header_num);

        char header[header_num][header_max_length];
        memset(header, 0, header_num * header_max_length);
        {
            for (int i = 0; i < header_num; ++i) {
                int copy_nchars = strlen(headers_pointer[i]);
                if (headers_pointer[i][copy_nchars - 1] == '\n') {
                    --copy_nchars;
                }
                strncpy(header[i], headers_pointer[i], copy_nchars);
                printf("~%10d\t\t%s~\n", i, header[i]);
            }
        }

        int columns_max_length[header_num];
        {

        }

        exit(EXIT_SUCCESS);

        // Close the file
        fclose(fp);
    }
    return 0;
}