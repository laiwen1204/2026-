#include "HeaderFiles.h"

int main(void)
{
    SCB->VTOR = 0x08011000;
    __DSB();
    __ISB();

    system_init();     
    scheduler_init();  

    while (1)
    {
        scheduler_run();
    }
}
