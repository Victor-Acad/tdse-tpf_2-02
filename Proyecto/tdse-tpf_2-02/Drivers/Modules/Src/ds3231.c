#include "ds3231.h"

uint8_t data_tx[8];
uint8_t data_rx[2];

char dw[7][11] = {"Domingo","Lunes","Martes","Miercoles","Jueves","Viernes","Sabado"};

void DS3231_Set_Date_Time(uint8_t dy, uint8_t mth, uint8_t yr, uint8_t dw, uint8_t hr, uint8_t mn, uint8_t sc)
{
	sc &= 0x7F;
	hr &= 0x3F;
	data_tx[0] = 0x00;
	data_tx[1] = DS3231_Bin_Bcd(sc);
	data_tx[2] = DS3231_Bin_Bcd(mn);
	data_tx[3] = DS3231_Bin_Bcd(hr);
	data_tx[4] = DS3231_Bin_Bcd(dw);
	data_tx[5] = DS3231_Bin_Bcd(dy);
	data_tx[6] = DS3231_Bin_Bcd(mth);
	data_tx[7] = DS3231_Bin_Bcd(yr);
	HAL_I2C_Master_Transmit(&hi2c2, (uint16_t)DS3231_ADDRESS, data_tx, 8, 100);
}

void DS3231_Get_Date(uint8_t *day, uint8_t *mth, uint8_t *year, uint8_t *dow)
{
    *dow = DS3231_Bcd_Bin(DS3231_Read(DS3231_DAY) & 0x7F);
    *day = DS3231_Bcd_Bin(DS3231_Read(DS3231_DATE) & 0x3F);
    *mth = DS3231_Bcd_Bin(DS3231_Read(DS3231_MONTH) & 0x1F);
    *year = DS3231_Bcd_Bin(DS3231_Read(DS3231_YEAR));
}

void DS3231_Get_Time(uint8_t *hr, uint8_t *min, uint8_t *sec)
{
    *sec = DS3231_Bcd_Bin(DS3231_Read(DS3231_SEC) & 0x7F);
    *min = DS3231_Bcd_Bin(DS3231_Read(DS3231_MIN) & 0x7F);
    *hr = DS3231_Bcd_Bin(DS3231_Read(DS3231_HOUR) & 0x3F);
}

void DS3231_Get_DayOfWeek(char* str)
{
    uint8_t lday;
    uint8_t lmonth;
    uint8_t lyr;
    uint8_t ldow;
    uint8_t chr = 0;
    DS3231_Get_Date(&lday, &lmonth, &lyr, &ldow);
	do{
		str[chr] = dw[ldow][chr];
	} while(dw[ldow][chr++] != '\0');
	str[chr] = '\0';
}

uint8_t DS3231_Read(uint8_t reg)
{
	data_tx[0] = reg;
	HAL_I2C_Master_Transmit(&hi2c2, (uint16_t)DS3231_ADDRESS, &data_tx[0], 1, 100);
	HAL_I2C_Master_Receive(&hi2c2, (uint16_t)DS3231_ADDRESS, data_rx, 1, 100);
    return data_rx[0];
}

uint8_t DS3231_Bin_Bcd(uint8_t binary_value)
{
	uint8_t temp;
    uint8_t retval;
    temp = binary_value;
    retval = 0;
    while(1)
    {
        if(temp >= 10){
            temp -= 10;
            retval += 0x10;
        }else{
            retval += temp;
            break;
        }
    }
    return(retval);
}

uint8_t DS3231_Bcd_Bin(uint8_t bcd_value)
{
    uint8_t temp;
    temp = bcd_value;
    temp >>= 1;
    temp &= 0x78;
    return(temp + (temp >> 2) + (bcd_value & 0x0f));
}
