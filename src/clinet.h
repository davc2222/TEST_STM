#ifndef CLINET_H
#define CLINET_H

#  include <stdint.h>
#define BIT_PATTERN_LENGTH 8
typedef   struct 
{

 uint32_t test_id;          // Command ID
 uint8_t tested_Peripheral; // 
 uint8_t iterations_num; // Number of iterations
uint8_t	bit_pattern_length ;
 char   bit_pattern[BIT_PATTERN_LENGTH];
uint8_t cmd_crc;



} cmd_to_stm_t;




#endif