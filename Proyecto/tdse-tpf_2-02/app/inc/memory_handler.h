/*
 *
 * @file   : memory_handler.h
 * @date   : Aug 10, 2025
 *
 */

#ifndef MEMORY_HANDLER_H
#define MEMORY_HANDLER_H

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** data types *******************************************/
typedef enum {
    MEM_WRITE_PWD,
    MEM_CLEAR,
    MEM_WRITE_TIME,
    MEM_NO_WRITE
} MEM_WriteType_t;

/********************** external data declaration ****************************/
extern MEM_WriteType_t MEM_WriteType;

/********************** external functions declaration ***********************/
extern void handle_memory(uint32_t tick, MEM_WriteType_t type, char pwd[], uint8_t idx, char time_str[]);

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif // MEMORY_HANDLER_H

/********************** end of file ******************************************/
