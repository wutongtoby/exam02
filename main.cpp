#include "mbed.h"

#include "fsl_port.h"

#include "fsl_gpio.h"

#define UINT14_MAX        16383

// FXOS8700CQ I2C address

#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0

#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0

#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1

#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1

// FXOS8700CQ internal register addresses

#define FXOS8700Q_STATUS 0x00

#define FXOS8700Q_OUT_X_MSB 0x01

#define FXOS8700Q_OUT_Y_MSB 0x03

#define FXOS8700Q_OUT_Z_MSB 0x05

#define FXOS8700Q_M_OUT_X_MSB 0x33

#define FXOS8700Q_M_OUT_Y_MSB 0x35

#define FXOS8700Q_M_OUT_Z_MSB 0x37

#define FXOS8700Q_WHOAMI 0x0D

#define FXOS8700Q_XYZ_DATA_CFG 0x0E

#define FXOS8700Q_CTRL_REG1 0x2A

#define FXOS8700Q_M_CTRL_REG1 0x5B

#define FXOS8700Q_M_CTRL_REG2 0x5C

#define FXOS8700Q_WHOAMI_VAL 0xC7


I2C i2c( PTD9,PTD8);

Serial pc(USBTX, USBRX);

EventQueue queue;
DigitalOut led(LED1);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;
InterruptIn sw(SW3);

int id;
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
Timer my_time;
void FXOS8700CQ_writeRegs(uint8_t * data, int len);

uint8_t who_am_i, data[2], res[6];

int16_t acc16;

float t[3];
float x[100];
float y[100];
float z[100];
bool geq5[100];
int times = 0;

void logger(void)
{
      FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);


      acc16 = (res[0] << 6) | (res[1] >> 2);

      if (acc16 > UINT14_MAX/2)

         acc16 -= UINT14_MAX;

      t[0] = ((float)acc16) / 4096.0f;


      acc16 = (res[2] << 6) | (res[3] >> 2);

      if (acc16 > UINT14_MAX/2)

         acc16 -= UINT14_MAX;

      t[1] = ((float)acc16) / 4096.0f;


      acc16 = (res[4] << 6) | (res[5] >> 2);

      if (acc16 > UINT14_MAX/2)

         acc16 -= UINT14_MAX;

      t[2] = ((float)acc16) / 4096.0f;

   x[times] = t[0];
   y[times] = t[1];
   z[times] = t[2];
   
   //printf("%1.3f\r\n%1.3f\r\n%1.3f\r\n", t[0], t[1], t[2]);
    // print x y z repevtively
   //printf("%d\r\n", times);
   led = !led;

   // if (horizontal q) * 9.5 * 0.1^2 *0.5 > 5 
   if (sqrt(t[0] * t[0] + t[1] * t[1]) * 9.8 * 0.1 * 0.1 *0.5  > 5)
      geq5[times] = 1;
   else
      geq5[times] = 0; 
   times++;
}
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {

   char t = addr;

   i2c.write(m_addr, &t, 1, true);

   i2c.read(m_addr, (char *)data, len);

}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {

   i2c.write(m_addr, (char *)data, len);

}

void start(void)
{
   id = queue.call_every(100, logger);
   my_time.start();
}

int main(void) {
   led = 1;
   pc.baud(115200);

   // Enable the FXOS8700Q
   FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);

   data[1] |= 0x01;

   data[0] = FXOS8700Q_CTRL_REG1;

   FXOS8700CQ_writeRegs(data, 2);

   // Get the slave address

   FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);
   ///////////////////////////////////////////////////////////////////
   Thread t;//(osPriorityLow);

   t.start(callback(&queue, &EventQueue::dispatch_forever)); 
   sw.rise(&start);
   while(1) {
      if (times == 100 || my_time >= 10) {
         queue.cancel(id);
         break;
      }
   }
   for (int i = 0; i < times; i++) {
      printf("%1.3f\r\n", x[i]);
      printf("%1.3f\r\n", y[i]);
      printf("%1.3f\r\n", z[i]);
      printf("%d\r\n", geq5[i]);
   }
}
