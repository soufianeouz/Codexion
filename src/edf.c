// #include "codexion.h"

// void edf_request(t_dongle *dongle, t_coder *coder){
//     int i;
//     long long deadline1;
//     long long deadline2;

// 	i = 0;
//     while (i < 2)
//     {
//         if (dongle->queue[i] == coder)
//             return;
//         i++;
//     }
//     if (dongle->queue[0] == NULL){
//         dongle->queue[0] = coder;
//     }
//     else
//     {
//         deadline1 = dongle->queue[0]->last_compile_start + dongle->queue[0]->config->time_to_burnout;
//         deadline2 = coder->last_compile_start + coder->config->time_to_burnout;
//         if (deadline2 < deadline1){
//             dongle->queue[1] = dongle->queue[0];
//             dongle->queue[0] = coder;
//         }else
//             dongle->queue[1] = coder;
//     }
    
// }