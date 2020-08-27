/*---------------------------------------------------------------------------------
  Oximeter - hardware for SpO2 and PPG

  support both AFE4490 and MAX3010x, but only turn on one of them 
---------------------------------------------------------------------------------*/
#include "firmware.h"

extern  portMUX_TYPE oximeterMux;

bool      ppgBufferReady      = false;
uint8_t   ppg_data_buff[PPG_BUFFER_SIZE];

class SPO2 spo2;

void IRAM_ATTR oximeter_interrupt_handler()
{
  portENTER_CRITICAL_ISR(&oximeterMux);
  spo2.interrupt_flag = true;
  portEXIT_CRITICAL_ISR (&oximeterMux);  
}
 

void SPO2::init()
{
  ppg_data_cnt    = 0;
  interrupt_flag  = false;
  
  digitalWrite(SPO2_START_PIN, LOW);
  delay(500);
  digitalWrite(SPO2_START_PIN, HIGH);
  delay(500);

  #if   (SPO2_TYPE==OXI_AFE4490)
    afe4490.init();             // SPI controls ADS1292R and AFE4490,
    attachInterrupt(digitalPinToInterrupt(OXIMETER_INT_PIN), 
                    oximeter_interrupt_handler, RISING ); 
  #elif (SPO2_TYPE==OXI_MAX30102)
    initMax3010xSpo2();
    attachInterrupt(digitalPinToInterrupt(OXIMETER_INT_PIN), 
                    oximeter_interrupt_handler, FALLING ); 
  #elif (SPO2_TYPE==OXI_NULL)
    // do nothing
  #else 
    #error define here!
  #endif 
}
 
void SPO2::handleData()
{
  #if   (SPO2_TYPE==OXI_AFE4490)
    afe4490.getData();          // handle SpO2 and PPG 
  #elif (SPO2_TYPE==OXI_MAX30102)
    handleMax3010xSpo2();       // handel SpO2 and PPG
  #elif (SPO2_TYPE==OXI_NULL)
    // do nothing  
  #else 
    #error define here!
  #endif   
}

void SPO2::save_to_ppg_buffer(uint8_t i)
{//FIXME MAX3010x implementation
    ppg_data_buff[ppg_data_cnt++] = i;
    if(ppg_data_cnt >=PPG_BUFFER_SIZE)
    {
        ppgBufferReady = true;
        ppg_data_cnt = 0;
    }
}

void SPO2::clear_interrupt()
{
  portENTER_CRITICAL_ISR(&oximeterMux);
  spo2.interrupt_flag = false;
  portEXIT_CRITICAL_ISR (&oximeterMux);  
}
