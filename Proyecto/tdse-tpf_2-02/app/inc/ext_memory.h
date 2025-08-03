/*
 * @file   : ext_memory.h
 * @date   : Jul 10, 2025
 */

#ifndef EXT_MEMORY_H
#define EXT_MEMORY_H

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** defines **********************************************/
#define MEMORY_CONNECTED                       (0)

/********************** data types *******************************************/
typedef struct {
	char mem_status[8];
	bool system_status;
	bool alarm_status;
	char password[6];
	bool ldr_mode;
	uint8_t ldr_adj;
} memory_t;

/********************** external data declaration ****************************/
extern memory_t ext_mem;

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_INC_TASK_ACTUATOR_H_ */

/********************** end of file ******************************************/
