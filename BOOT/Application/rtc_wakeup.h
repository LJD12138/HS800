#ifndef RTC_WAKEUP_H_
#define RTC_WAKEUP_H_

#include "board_config.h"

#if(boardLOW_POWER)

#include "main.h"

void rtc_configuration( u8 num );
void rtc_close(void);

#endif  //boardLOW_POWER

#endif //RTC_WAKEUP_H_


