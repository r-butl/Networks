#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

#define BATCH_SIZE 150

int main(int argc, char *argv[]){
    int input_file_desc;
    int output_file_desc;
    char buffer[BATCH_SIZE];
    int bytes_read;
    int bytes_written;
    const char* output_file = "upper_file.txt";
    
    if( argc != 2){
        perror("USAGE: ./main <file>\n");
        return -1;
    }

    // Open the input file
    printf("Opening file '%s'\n", argv[1]); 
    input_file_desc = open(argv[1], O_RDONLY);

    if( input_file_desc == -1){
        perror("Error opening file...");
        return -1;
    }   

    // Open the output file descriptor
    printf("Opening output file '%s'\n", output_file); 
    output_file_desc = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if( output_file_desc == -1){
        perror("Error opening file...");
        return -1;
    } 

    while(( bytes_read = read(input_file_desc, buffer, BATCH_SIZE)) > 0){
        printf("%d bytes read: \n", bytes_read);

        // Adjust the characters in the buffer
        for( int i = 0; i < bytes_read; i++){            
            printf("%c", buffer[i]);

	    // Convert to upper case
            if( buffer[i] > 96 && buffer[i] < 123){
                buffer[i] = buffer[i] - 32;
            }
        }
        printf("\n\n");

        // Write the bytes to the output file
        if( (bytes_written = write(output_file_desc, buffer, BATCH_SIZE) ) < 1 ){
            perror("Error writing data...\n");
            return -1;
        }
    }

    close(input_file_desc);
    close(output_file_desc);
    printf("done\n");

    return 0;
}
