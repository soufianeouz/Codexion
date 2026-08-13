#include "codexion.h"

int main(int argc, char **argv)
{
    t_config config;

    if (parse_arguments(argc, argv, &config) == 0)
        return (1);
    

    
    return 0;
}