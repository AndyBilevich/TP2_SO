#include <b.h>

void start_b(void){
    for (size_t i = 0; i < 3; i++){
        wait(1);
        printf("B is counting: %d\n", i);
    }
}