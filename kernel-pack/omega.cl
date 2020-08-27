/*
* omega.cl
*
*  date    : 01.08.2020
*  author  : M. Khaled
*/

#define CFG_MSG @pfaces-configValueString:"project_name"
#define PARAM_MSG @@param_message@@


kernel void exampleKernelFunction(global char* data_in){
    printf("Hello World from the device side!\n");
    printf("The value of the config item (config_message) is: %s\n", CFG_MSG);
    printf("The value of the kernel param (param_message) is: %s\n", PARAM_MSG);
}
