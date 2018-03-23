//*****************************************************************************
// CC3200AUDBOOST.c
//
// Driver for TI TLV320AIC3254 CODEC
//
// Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup audio_app
//! @{
//
//*****************************************************************************
/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* Other Includes */
#include "CC3200AUDBOOST.h"
#include "HRDWCFG.h"
#include "BTPSKRNL.h"

//******************************************************************************
//
// Writes to specified register
// ucRegAddr - 8 bit Register Address
// ucRegValue - 8 bit Register Value
//
//******************************************************************************
unsigned long CodecRegWrite(unsigned char ucRegAddr,unsigned char ucRegValue)
{
    /* Make sure the last transaction has been completely sent out.     */
    while(I2C_masterIsStopSent(HRDWCFG_I2C_MODULE) == EUSCI_B_I2C_SENDING_STOP)
       ;

    /* Send twice the first data byte to clean out whatever is in       */
    /* the buffer from a previous send.                                 */
    I2C_masterSendMultiByteStart(HRDWCFG_I2C_MODULE, ucRegAddr);
    I2C_masterSendMultiByteNext(HRDWCFG_I2C_MODULE, ucRegAddr);
    I2C_masterSendMultiByteFinish(HRDWCFG_I2C_MODULE, ucRegValue);
    
    /* Wait until the transaction completes.                            */
    while(I2C_isBusBusy(HRDWCFG_I2C_MODULE) == UCBBUSY)
       ;

    /* Delay for a short amount of time, I2C hang ups were seen when    */
    /* this delay was not included.  Note that this time could probably */
    /* be made much shorter if it every becomes an issue, originally the*/
    /* delay was 10s of microseconds.                                   */
    BTPS_Delay(20);

    return(0);
}

//******************************************************************************
//
// Selects the Page for reading/writing
//
//******************************************************************************
unsigned long CodecPageSelect(unsigned char ucPageAddress)
{
    return CodecRegWrite(PAGE_CTRL_REG,ucPageAddress);
}

//******************************************************************************
//
// Codec Soft Reset
//
//******************************************************************************
void CodecReset(void)
{
    /* Select page 0	*/
    CodecPageSelect(0);

    /* Soft RESET	*/
    CodecRegWrite(CODEC_REG_SOFT_RESET,1);
}

//******************************************************************************
//
// Codec Configure
//
//******************************************************************************
void CodecInit(unsigned char ucInputLine, unsigned char ucOutputLine)
{

	/*	Initialization	*/
    CodecReset();

    /*	Digital Configuration
     * 		-> PLL_CLK = (PLL_CLKIN x R x J.D)/P
     * 			-> CODEC_CLKIN = PLL_CLK
     * 				-> DAC_Fs = CODEC_CLK_IN / (NDAC.MDAC.DOSR)
     * 				-> ADC_Fs = CODEC_CLK_IN / (NADC.MADC.AOSR)
     * 	Options:
     * 		-> 84.672MHz = (3.528MHz  x 1 x 24.0)/1 --> For Fs = 44.1KHz
     * 		-> 92.160MHz = (3.840MHz  x 1 x 24.0)/1 --> For Fs = 48KHz
     * 				-> 44.1KHz = 84.672MHz / (3.5.128)
     * 				-> 48KHz = 92.160MHz / (3.5.128)
     * 	Considerations:
     * 		-> MDAC * xOSR >= ResourceClass * 32
     *			- 5 * 128 (640) >= 8 * 32 (256)
     *		-> 2.8MHz < xOSR x ADC_Fs < 6.758MHz
     *			- 128 * 44,100 = 5.644MHz
     *			- 128 * 48,000 = 6.144MHz
     *		-> xOSR must be a multiple of 8 (48KHz High-Performance)
     *			-> xOSR = 64, for Low-Power Mode
     *			-> xOSR = 128, for High-Performance Mode
     */
    CodecPageSelect(0);
    // P0, R4, b6.			PLL Range = High (1)
    // P0, R4, b3-2.       	PLL_CLKIN = BCLK (01)
    // P0, R4, b1-0.       	CODEC_CLKIN = PLL_CLK (11)
    CodecRegWrite(CODEC_REG_CLK_MUX, 0x47);
    // P0, R5, b7.         	PLL = Power Up (1)
    // P0, R5, b6-4.       	PLL Divider P = 1 (001)
    // P0, R5, b3-0.       	PLL Divider R = 1 (0001)
    CodecRegWrite(CODEC_REG_PLL_P_R, 0x91);
    // P0, R6, b5-0.       	PLL Divider J = 24 (11000)
    CodecRegWrite(CODEC_REG_PLL_J, 0x18);
    // P0, R7, b5-0. (MSB) 	PLL Divider D = 0
    // P0, R8, b7-0. (LSB)
    CodecRegWrite(CODEC_REG_PLL_D_MSB, 0x00);
    CodecRegWrite(CODEC_REG_PLL_D_LSB, 0x00);
    // P0, R11, b7.        	NDAC = Power Up
    // P0, R11, b6-0.      	NDAC = 3
    CodecRegWrite(CODEC_REG_NDAC, 0x83);
    // P0, R12, b7.        	MDAC = Power Up
    // P0, R12, b6-0.      	MDAC = 5
    CodecRegWrite(CODEC_REG_MDAC, 0x85);
    // P0, R13, b1-0.	  	DOSR_MSB (00)
    // P0, R13, b7-0.	  	DOSR_LSB (0x80) --> DOSR = 128
    CodecRegWrite(CODEC_REG_DOSR_MSB, 0x00);
    CodecRegWrite(CODEC_REG_DOSR_LSB, 0x80);
    // P0, R18, b7.        	NADC = Power Up
    // P0, R18, b6-0.      	NADC = 3
    CodecRegWrite(CODEC_REG_NADC, 0x83);
    // P0, R19, b7.        	MADC = Power Up
    // P0, R19, b6-0.      	MADC = 5
    CodecRegWrite(CODEC_REG_MADC, 0x85);
    // P0, R20, b7-0.	  	AOSR = 128
    CodecRegWrite(CODEC_REG_AOSR, 0x80);
    // P0, R27, b7-6.      	Interface Mode = DSP Mode (01)
    // P0, R27, b5-4.      	Data Length = 16bits (00)
    // P0, R27, b3.        	BCLK = Input (0)
    // P0, R27, b2.        	WCLK = Input (0)
    // P0, R27, b0.        	DOUT = Output (0)
    CodecRegWrite(CODEC_REG_AUD_IF_CTRL_1, 0x40);
    // P0, R28, b7-0.      	Data Offset = 1
    CodecRegWrite(CODEC_REG_AUD_IF_CTRL_2, 0x01);
    // P0, R60, b4-0.     	DAC Processing Block: PRB_R1
    CodecRegWrite(CODEC_REG_DAC_PRB, 0x01);
    // P0, R61, b4-0.     	ADC Processing Block: PRB_R1
    CodecRegWrite(CODEC_REG_ADC_PRB, 0x01);

    /*	Analog Configuration	*/
    CodecPageSelect(1);
    // P1, R2, b7-6.   		DVDD LDO = 1.72V (00)
    // P1, R2, b5-4.   		AVDD LDO = 1.72V (00)
    // P1, R2, b3.     		Analog Blocks = Disabled (1)
    // P1, R2, b0.     		AVDD LDO = Power Up (1)
    CodecRegWrite(CODEC_REG_LDO_CTRL, 0x09);
    // P1, R1, b3.     		Weak Connection AVDD-DVDD = Disabled (1)
    CodecRegWrite(CODEC_REG_PWR_CFG, 0x08);
    // P1, R2, b7-6.   		DVDD LDO = 1.72V (00)
    // P1, R2, b5-4.   		AVDD LDO = 1.72V (00)
    // P1, R2, b3.     		Analog Blocks = Enabled (0)
    // P1, R2, b0.     		AVDD LDO = Power Up (1)
    CodecRegWrite(CODEC_REG_LDO_CTRL, 0x01);
    // P1, R10, b6.			Full Chip Common Mode = 0.9V (0)
    // P1, R10, b5-4.		HPL/HPR Common Mode = 1.65V - b6=0 (11)
    // P1, R10, b3.			LOL/LOR Common Mode = Full Chip CM (0)
    // P1, R10, b1.			HPL/HPR Power = LDOIN Supply (1)
    // P1, R10, b0.			LDOIN PWR = 1.8V-3.6V (1)
    CodecRegWrite(CODEC_REG_COMMON_MODE, 0x33);
    // P1, R71, b5-0.   	Analog Input Pwr Up = 6.4ms (11 0010)
    CodecRegWrite(CODEC_REG_ANA_IN_CHARGE, 0x32);
    // P1, R123, b2-0.  	Reference will power up in 40ms
    //						when analog blocks are powered up (001)
    CodecRegWrite(CODEC_REG_REF_PWR_CFG, 0x01);
    // P1, R20, b7-6.		Soft routing step time = 0 ms (00)
    // P1, R20, b2-5.		Ramp-Up time = 5.0 Time Constants (1001)
    // P1, R20, b1-0.		6k resistance (01)
    CodecRegWrite(CODEC_REG_HP_DRV_STUP, 0x25);

    switch(ucInputLine)
    {
    	case NO_INPUT:
    		break;

    	case CODEC_ONBOARD_MIC:
			// Select Page 1
			CodecPageSelect(1);
			// P1, R51, b6			MICBIAS = Power Up (1)
			// P1, R51, b5-4		MICBIAS = 1.25V (CM = 0.9V) (00)
			// P1, R51, b3      	MICBIAS PWR = LDOIN (1)
			CodecRegWrite(CODEC_REG_MCBIAS_CFG, 0x48);
			// P1, R52, b7-6		IN1L-Left_MICPGA = Not routed (00)
			// P1, R52, b5-4		IN2L-Left_MICPGA = Not routed (00)
			// P1, R52, b3-2      	IN3L-Left_MICPGA = Not routed (00)
			// P1, R52, b1-0      	IN1R-Left_MICPGA = Not routed (00)
			CodecRegWrite(CODEC_REG_LEFT_MICPGA_POS, 0x00);
			// P1, R54, b7-6		CM-Left_MICPGA (CM1L) = Not routed (00)
			// P1, R54, b5-4		IN2R-Left_MICPGA = Not routed (00)
			// P1, R54, b3-2      	IN3R-Left_MICPGA = Not routed (00)
			// P1, R54, b1-0      	CM-Left_MICPGA (CM2L) = Not routed (00)
			CodecRegWrite(CODEC_REG_LEFT_MICPGA_NEG, 0x00);
			// P1, R55, b7-6		IN1R-Right_MICPGA = Not routed (00)
			// P1, R55, b5-4		IN2R-Right_MICPGA = Not routed (00)
			// P1, R55, b3-2      	IN3R-Right_MICPGA = Routed w/10K (01)
			// P1, R55, b1-0      	IN2L-Right_MICPGA = Not routed (00)
			CodecRegWrite(CODEC_REG_RIGHT_MICPGA_POS, 0x04);
			// P1, R57, b7-6		CM-Right_MICPGA (CM1L) = Routed w/10K (01)
			// P1, R57, b5-4		IN1L-Right_MICPGA = Not routed (00)
			// P1, R57, b3-2      	IN3L-Right_MICPGA = Not routed (00)
			// P1, R57, b1-0      	CM-Right_MICPGA (CM2L) = Not routed (00)
			CodecRegWrite(CODEC_REG_RIGHT_MICPGA_NEG, 0x40);
			// P1, R58, b7			IN1L = Not Connected to CM (0)
			// P1, R58, b6			IN1R = Not Connected to CM (0)
			// P1, R58, b5      	IN2L = Not Connected to CM (0)
			// P1, R58, b4      	IN2R = Not Connected to CM (0)
			// P1, R58, b3      	IN3L = Weakly Connected to CM (1)
			// P1, R58, b2      	IN3R = Not Connected to CM (0)
			CodecRegWrite(CODEC_REG_FLOAT_INPUT_CFG, 0x08);
			// P1, R59, b7        	Left_MICPGA is enabled (0)
			// P1, R59, b6-0      	Left_MICPGA Gain = +0dB (0)
			CodecRegWrite(CODEC_REG_LEFT_MICPGA_VOL, 0x0);
			// P1, R60, b7        	Right_MICPGA is enabled (0)
			// P1, R60, b6-0      	Right_MICPGA Gain = +40dB (80)
			CodecRegWrite(CODEC_REG_RIGHT_MICPGA_VOL, 0x50);

			// Select Page 1
			CodecPageSelect(0);
			// P0, R81, b7			Left_ADC = Power Down (0)
			// P0, R81, b6			Right_ADC = Power Up (1)
			// P0, R81, b5-4      	DIG_MIC_IN = GPIO (0)
			// P0, R81, b3      	Left_ADC = Not DIG_MIC (0)
			// P0, R81, b2      	Right_ADC = Not DIG_MIC (0)
			// P0, R81, b1-0      	ADC_Soft-Stepping = 1 step/ FSYNC (00)
			CodecRegWrite(CODEC_REG_ADC_CTRL, 0x40);
			// P0, R82, b7			Left_ADC = Muted (1)
			// P0, R82, b6-4		Left_ADC Fine Gain = 0dB (000)
			// P0, R82, b3      	Right_ADC = Un-muted (0)
			// P0, R82, b2-0      	Right_ADC Fine Gain = 0dB (000)
			CodecRegWrite(CODEC_REG_ADC_FINE_GAIN, 0x80);
			break;

    	case CODEC_LINE_IN:
			// Select Page 1
			CodecPageSelect(1);
			// P1, R51, b6			MICBIAS = Power down (0)
			// P1, R51, b5-4		MICBIAS = 1.25V (CM = 0.9V) (00)
			// P1, R51, b3      	MICBIAS PWR = LDOIN (1)
			CodecRegWrite(CODEC_REG_MCBIAS_CFG, 0x08);
			// P1, R52, b7-6		IN1L-Left_MICPGA = Routed w/10K (01)
			// P1, R52, b5-4		IN2L-Left_MICPGA = Not routed (00)
			// P1, R52, b3-2      	IN3L-Left_MICPGA = Not routed (00)
			// P1, R52, b1-0      	IN1R-Left_MICPGA = Not routed (00)
			CodecRegWrite(CODEC_REG_LEFT_MICPGA_POS, 0x40);
			// P1, R54, b7-6		CM-Left_MICPGA (CM1L) = Routed w/10K (01)
			// P1, R54, b5-4		IN2R-Left_MICPGA = Not routed (00)
			// P1, R54, b3-2      	IN3R-Left_MICPGA = Not routed (00)
			// P1, R54, b1-0      	CM-Left_MICPGA (CM2L) = Not routed (00)
			CodecRegWrite(CODEC_REG_LEFT_MICPGA_NEG, 0x40);
			// P1, R55, b7-6		IN1R-Right_MICPGA = Routed w/10K (01)
			// P1, R55, b5-4		IN2R-Right_MICPGA = Not routed (00)
			// P1, R55, b3-2      	IN3R-Right_MICPGA = Not routed (00)
			// P1, R55, b1-0      	IN2L-Right_MICPGA = Not routed (00)
			CodecRegWrite(CODEC_REG_RIGHT_MICPGA_POS, 0x40);
			// P1, R57, b7-6		CM-Right_MICPGA (CM1L) = Routed w/10K (01)
			// P1, R57, b5-4		IN1L-Right_MICPGA = Not routed (00)
			// P1, R57, b3-2      	IN3L-Right_MICPGA = Not routed (00)
			// P1, R57, b1-0      	CM-Right_MICPGA (CM2L) = Not routed (00)
			CodecRegWrite(CODEC_REG_RIGHT_MICPGA_NEG, 0x40);
			// P1, R58, b7			IN1L = Not Connected to CM (0)
			// P1, R58, b6			IN1R = Not Connected to CM (0)
			// P1, R58, b5      	IN2L = Not Connected to CM (0)
			// P1, R58, b4      	IN2R = Not Connected to CM (0)
			// P1, R58, b3      	IN3L = Not Connected to CM (0)
			// P1, R58, b2      	IN3R = Not Connected to CM (0)
			CodecRegWrite(CODEC_REG_FLOAT_INPUT_CFG, 0x00);
			// P1, R59, b7        	Left_MICPGA is enabled (0)
			// P1, R59, b6-0      	Left_MICPGA Gain = +0dB (0)
			CodecRegWrite(CODEC_REG_LEFT_MICPGA_VOL, 0x00);
			// P1, R60, b7        	Right_MICPGA is enabled (0)
			// P1, R60, b6-0      	Right_MICPGA Gain = +0dB (0)
			CodecRegWrite(CODEC_REG_RIGHT_MICPGA_VOL, 0x00);

			// Select Page 1
			CodecPageSelect(0);
			// P0, R81, b7			Left_ADC = Power Up (1)
			// P0, R81, b6			Right_ADC = Power Up (1)
			// P0, R81, b5-4      	DIG_MIC_IN = GPIO (0)
			// P0, R81, b3      	Left_ADC = Not DIG_MIC (0)
			// P0, R81, b2      	Right_ADC = Not DIG_MIC (0)
			// P0, R81, b1-0      	ADC_Soft-Stepping = 1 step/ FSYNC (00)
			CodecRegWrite(CODEC_REG_ADC_CTRL, 0xC0);
			// P0, R82, b7			Left_ADC = Un-muted (0)
			// P0, R82, b6-4		Left_ADC Fine Gain = 0dB (000)
			// P0, R82, b3      	Right_ADC = Un-muted (0)
			// P0, R82, b2-0      	Right_ADC Fine Gain = 0dB (000)
			CodecRegWrite(CODEC_REG_ADC_FINE_GAIN, 0x00);
			break;

    	default:
    		break;
    }

    switch(ucOutputLine)
    {
		case NO_INPUT:
					break;

		case CODEC_LINE_OUT:
       		// Select Page 1
       		CodecPageSelect(1);
			// P1, R12, b3.			Left_DAC -> HPL (1)
			CodecRegWrite(CODEC_REG_HPL_SEL, 0x08);
			// P1, R13, b3.			Right_DAC -> HPR (1)
			CodecRegWrite(CODEC_REG_HPR_SEL, 0x08);
			// P1, R3, b7-6.		Left_DAC-HPL -> Class-AB Driver (00)
			// P1, R3, b4-2.		Left_DAC PTM Control = PTM_P3 (000)
			CodecRegWrite(CODEC_REG_PYBCK_CTRL_1, 0x00);
			// P1, R4, b7-6.		Right_DAC-HPL -> Class-AB Driver (00)
			// P1, R4, b4-2.		Right_DAC PTM Control = PTM_P3 (000)
			CodecRegWrite(CODEC_REG_PYBCK_CTRL_2, 0x00);
			// P1, R16, b6.			HPL Driver = Not Muted (0)
			// P1, R16, b5-0.		HPL Driver Gain = 0dB
			CodecRegWrite(CODEC_REG_HPL_DRV_GAIN, 0x00);
			// P1, R17, b6.			HPR Driver = Not Muted (0)
			// P1, R17, b5-0.		HPR Driver Gain = 0dB
			CodecRegWrite(CODEC_REG_HPR_DRV_GAIN, 0x00);
			// P1, R9, b5.			HPL Driver = Power Up (1)
			// P1, R9, b4.			HPR Driver = Power Up (1)
			CodecRegWrite(CODEC_REG_OUT_DRV_CTRL ,0x30);

			// Select Page 0
			CodecPageSelect(0);
			// P0, R65, b7-0.		Left_DAC Volume = 0dB
			CodecRegWrite(CODEC_REG_LDAC_VOL, 0x00);
			// P0, R66, b7-0.		Right_DAC Volume = 0dB
			CodecRegWrite(CODEC_REG_RDAC_VOL, 0x00);
			// P0, R63, b7.			Right_DAC = Power Up (1)
			// P0, R63, b6.			Right_DAC = Power Up (1)
			// P0, R63, b5-4.		Left_DAC  = Left Channel (01)
			// P0, R63, b3-2.		Right_DAC  = Right Channel (01)
			// P0, R63, b1-0.		Soft-Stepping = Disabled (10)
			CodecRegWrite(CODEC_REG_DAC_CTRL_1, 0xD6);
			// P0, R64, b7.			Right_DAC_Off = Zero_Data (0)
			// P0, R64, b6-4.		DAC_Auto_Mute = Disabled (000)
			// P0, R64, b3.			Left_DAC  = Not muted (0)
			// P0, R64, b2.			Right_DAC  = Not muted (0)
			// P0, R64, b1-0.		DAC_Master_Vol = Independent L/R (00)
			CodecRegWrite(CODEC_REG_DAC_CTRL_2, 0x00);
			break;

       	default:
       		break;
    }
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
