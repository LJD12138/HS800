/**
 * @file test.c
 *
 * Unit tests for the lwrb library
 *
 * @author Tofik Sonono (tofik@sonono.me)
 *
 */

/*======= Includes ==========================================================*/
#include "lwrb.h"
#include "main.h"

void
basic_read_and_write(lwrb_t* buff, uint8_t* data_to_write, size_t data_size) {
    uint8_t ret;
    size_t buffer_size = (sizeof(uint8_t) * data_size) + 1;
    uint8_t* buff_data = malloc(buffer_size);

    ret = lwrb_init(buff, buff_data, buffer_size);

    ret = lwrb_is_ready(buff);

    size_t n_written = lwrb_write(buff, data_to_write, data_size);

    size_t n_bytes_in_queue = lwrb_get_full(buff);

    uint8_t read_buffer[data_size];
    size_t n_read = lwrb_read(buff, read_buffer, 2);
	n_bytes_in_queue = lwrb_get_full(buff);
	n_read = lwrb_read(buff, read_buffer, n_bytes_in_queue);
	
    free(buff_data);
}

void
testAddElementsToQueueAndReadAndVerifyEmpty_should_succeed(void) {
    uint8_t data_to_write[] = {0, 1, 2, 3, 4, 5, 6, 7};
    lwrb_t buff = {0};
    basic_read_and_write(&buff, data_to_write, sizeof(data_to_write));

    size_t n_free_bytes = lwrb_get_free(&buff);
}


static u8 uca_data[10] = {0};
static lwrb_t t_buff_handler;
void Buff_Test(void)
{
	lwrb_init(&t_buff_handler, uca_data, sizeof(uca_data));
	
	u8 data[10]={1,2,3,4,5,6,7,8,9,10};
	u8 data3[4]={11,12,13,14};
	u8 data2[10]={0};
	u8 data4[2] = {5,6};
	lwrb_sz_t temp = 0;
	vu8 temp2 = 0;
	
	lwrb_write(&t_buff_handler, data, sizeof(data));
	temp2 = lwrb_get_full(&t_buff_handler);

	memset(data2, 0, sizeof(data2));
	temp2 = lwrb_find(&t_buff_handler, data4, sizeof(data4), 0, &temp);
	temp2 = lwrb_get_full(&t_buff_handler);
	
	lwrb_write(&t_buff_handler, data3, sizeof(data3));
	temp2 = lwrb_get_full(&t_buff_handler);
	
	memset(data2, 0, sizeof(data2));
	lwrb_read(&t_buff_handler, data2, 10);
	
	lwrb_write(&t_buff_handler, data, sizeof(data));
	
	memset(data2, 0, sizeof(data2));
	lwrb_read(&t_buff_handler, data2, 5);
	temp2 = lwrb_get_full(&t_buff_handler);
	
	lwrb_write(&t_buff_handler, data3, sizeof(data3));
	temp2 = lwrb_get_full(&t_buff_handler);
	
	memset(data2, 0, sizeof(data2));
	data4[0] = 12;
	data4[1] = 13;
	temp2 = lwrb_find(&t_buff_handler, data4, sizeof(data4), 0, &temp);
	temp2 = lwrb_get_full(&t_buff_handler);
}

/*======= Main ===============================================================*/

int
lwrb_test(void) {
//	testAddElementsToQueueAndReadAndVerifyEmpty_should_succeed();
	Buff_Test();
    return 0;
}

/*======= Local function implementations =====================================*/

