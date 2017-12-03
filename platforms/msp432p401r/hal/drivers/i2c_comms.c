#include "driverlib.h"
#include "i2c_comms.h"
#include "XBD_DeviceDependent.h"

//#define I2C_DEBUG 1
#ifdef I2C_DEBUG
#define DEBUG_I2C(x) printf_xbd(x)
#else
#define DEBUG_I2C(x)
#endif

//SCL - P1.7
//SDA - P1.6
//TF - P3.0

volatile uint_fast8_t tx_flag = 0;
uint8_t i2c_buf[180] = {0};
uint16_t i2c_size_read = 0;
uint16_t i2c_size_sent = 0;

static void (*i2cSlaveReceive)(uint8_t receiveDataLength, uint8_t* recieveData);
static uint8_t (*i2cSlaveTransmit)(uint8_t transmitDataLengthMax, uint8_t* transmitData);

// Set the XBD function that handles receive
void i2c_set_rx(void (*i2cSlaveRx_func)(uint8_t
            receiveDataLength, uint8_t* recieveData)) {
	i2cSlaveReceive = i2cSlaveRx_func;
}

// Set the XBD function that handles transmit
void i2c_set_tx(uint8_t (*i2cSlaveTx_func)(uint8_t
            transmitDataLengthMax, uint8_t* transmitData)) {
	i2cSlaveTransmit = i2cSlaveTx_func;
}

// Initialize i2c on EUSCI in slave mode
void i2c_init(void) {

	/* Select Port 1 for I2C - Set Pin 6, 7 to input Primary Module Function,
	*   (UCB0SIMO/UCB0SDA, UCB0SOMI/UCB0SCL).
	*/
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
	  GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

	/* eUSCI I2C Slave Configuration */
	MAP_I2C_initSlave(EUSCI_B0_BASE, SLAVE_ADDRESS, EUSCI_B_I2C_OWN_ADDRESS_OFFSET0,
	  EUSCI_B_I2C_OWN_ADDRESS_ENABLE);

	/* Enable the module and enable interrupts */
	MAP_I2C_enableModule(EUSCI_B0_BASE);

	MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0);
	MAP_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0);

	MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0);
	MAP_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0);

	MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, EUSCI_B_I2C_STOP_INTERRUPT);
	MAP_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_STOP_INTERRUPT);

	MAP_Interrupt_enableMaster();
}

typedef enum {
	STATE_START,
	STATE_RX, 
	STATE_TX
} state_t;

state_t state = STATE_START;

// Function called by XBD_serveCommunication to start processing of data
void i2c_rx(void)
{
	uint_fast16_t status;

	status = MAP_I2C_getEnabledInterruptStatus(EUSCI_B0_BASE);

	switch (state) {
	case STATE_START:
		if (status & EUSCI_B_I2C_RECEIVE_INTERRUPT0) {
			state = STATE_RX;
		} else if (status & EUSCI_B_I2C_TRANSMIT_INTERRUPT0) {
			state = STATE_TX;
		}
		break;
	case STATE_RX:
		// Receive interrupt
		if (status & EUSCI_B_I2C_RECEIVE_INTERRUPT0)
		{
			// Receive the byte from i2c
			i2c_buf[i2c_size_read] = MAP_I2C_slaveGetData(EUSCI_B0_BASE);
			i2c_size_read++;
			DEBUG_I2C("XBH sending\n");
		
		}
		//Received stop
		if (status & EUSCI_B_I2C_STOP_INTERRUPT) {
			MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, EUSCI_B_I2C_STOP_INTERRUPT);
			DEBUG_I2C("Received Stop\n");
			DEBUG_I2C((char *)i2c_buf);
			i2cSlaveReceive(i2c_size_read, i2c_buf);
			i2c_size_read = 0;
			state = STATE_START;
		}
		break;
	case STATE_TX:
		// Transmit interrupt
		if (status & EUSCI_B_I2C_TRANSMIT_INTERRUPT0)
		{
			if (tx_flag == 0) {
				i2cSlaveTransmit(160, i2c_buf);
				DEBUG_I2C(i2c_buf);
				tx_flag = 1;
			}
			// Transmit the byte over i2c
			MAP_I2C_slavePutData(EUSCI_B0_BASE, i2c_buf[i2c_size_sent]);
			i2c_size_sent++;
			DEBUG_I2C("XBH receiving\n");
		}
		if (status & EUSCI_B_I2C_STOP_INTERRUPT) {
			MAP_I2C_clearInterruptFlag(EUSCI_B0_BASE, EUSCI_B_I2C_STOP_INTERRUPT);
			DEBUG_I2C("Received Stop\n");
			DEBUG_I2C((char *)i2c_buf);
			i2c_size_sent = 0;
			tx_flag = 0;
			state = STATE_START;
		}
		break;	
	}
}

