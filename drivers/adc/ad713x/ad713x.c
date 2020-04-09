/***************************************************************************//**
 *   @file   ad713x.c
 *   @brief  Implementation of ad713x Driver.
 *   @author SPopa (stefan.popa@analog.com)
 *   @author Andrei Drimbarean (andrei.drimbarean@analog.com)
********************************************************************************
 * Copyright 2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdlib.h>
#include "ad713x.h"
#include "delay.h"
#include "error.h"

/******************************************************************************/
/***************************** Variable definition ****************************/
/******************************************************************************/

static const int ad713x_output_data_frame[3][9][2] = {
	{
		{ADC_16_BIT_DATA, CRC_6},
		{ADC_24_BIT_DATA, CRC_6},
		{ADC_32_BIT_DATA, NO_CRC},
		{ADC_32_BIT_DATA, CRC_6},
		{ADC_16_BIT_DATA, NO_CRC},
		{ADC_24_BIT_DATA, NO_CRC},
		{ADC_24_BIT_DATA, CRC_8},
		{ADC_32_BIT_DATA, CRC_8},
		{INVALID}
	},
	{
		{ADC_16_BIT_DATA, NO_CRC},
		{ADC_16_BIT_DATA, CRC_6},
		{ADC_24_BIT_DATA, NO_CRC},
		{ADC_24_BIT_DATA, CRC_6},
		{ADC_16_BIT_DATA, CRC_8},
		{ADC_24_BIT_DATA, CRC_8},
		{INVALID}
	},
	{
		{ADC_16_BIT_DATA, NO_CRC},
		{ADC_16_BIT_DATA, CRC_6},
		{ADC_16_BIT_DATA, CRC_8},
		{INVALID}
	},
};

/******************************************************************************/
/************************** Functions Implementation **************************/
/******************************************************************************/

/**
 * @brief Read from device.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The register data.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_spi_reg_read(struct ad713x_dev *dev,
			    uint8_t reg_addr,
			    uint8_t *reg_data)
{
	int32_t ret;
	uint8_t buf[2];

	buf[0] = AD713X_REG_READ(reg_addr);
	buf[1] = 0x00;

	ret = spi_write_and_read(dev->spi_desc, buf, 2);
	if(IS_ERR_VALUE(ret))
		return FAILURE;
	*reg_data = buf[1];

	return SUCCESS;
}

/**
 * @brief Write to device.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The register data.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_spi_reg_write(struct ad713x_dev *dev,
			     uint8_t reg_addr,
			     uint8_t reg_data)
{
	uint8_t buf[2];

	buf[0] = reg_addr;
	buf[1] = reg_data;

	return spi_write_and_read(dev->spi_desc, buf, 2);
}

/**
 * @brief SPI write to device using a mask.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param mask - The mask.
 * @param data - The register data.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_spi_write_mask(struct ad713x_dev *dev,
			      uint8_t reg_addr,
			      uint32_t mask,
			      uint8_t data)
{
	uint8_t reg_data;
	int32_t ret;

	ret = ad713x_spi_reg_read(dev, reg_addr, &reg_data);
	if(IS_ERR_VALUE(ret))
		return FAILURE;
	reg_data &= ~mask;
	reg_data |= data;

	return ad713x_spi_reg_write(dev, reg_addr, reg_data);
}

/**
 * @brief Device power mode control.
 * @param dev - The device structure.
 * @param mode - Type of power mode
 * 			Accepted values: LOW_POWER
 * 					 HIGH_POWER
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_set_power_mode(struct ad713x_dev *dev,
			      enum ad713x_power_mode mode)
{
	if (mode == LOW_POWER)
		return ad713x_spi_write_mask(dev, AD713X_REG_DEVICE_CONFIG,
					     AD713X_DEV_CONFIG_PWR_MODE_MSK, 0);
	else if (mode == HIGH_POWER)
		return ad713x_spi_write_mask(dev, AD713X_REG_DEVICE_CONFIG,
					     AD713X_DEV_CONFIG_PWR_MODE_MSK,
					     1);

	return FAILURE;
}

/**
 * @brief ADC conversion data output frame control.
 * @param dev - The device structure.
 * @param adc_data_len - Data conversion length
 * 				Accepted values: ADC_16_BIT_DATA
 * 						 ADC_24_BIT_DATA
 * 						 ADC_32_BIT_DATA
 * @param crc_header - CRC header
 * 				Accepted values: NO_CRC
 * 						 CRC_6
 * 						 CRC_8
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_set_out_data_frame(struct ad713x_dev *dev,
				  enum ad713x_adc_data_len adc_data_len,
				  enum ad713x_crc_header crc_header)
{
	uint8_t id;
	uint8_t i = 0;

	id = dev->dev_id;

	while (ad713x_output_data_frame[id][i][0] != INVALID) {
		if((adc_data_len == ad713x_output_data_frame[id][i][0]) &&
		    (crc_header == ad713x_output_data_frame[id][i][1])) {
			return ad713x_spi_write_mask(dev,
						     AD713X_REG_DATA_PACKET_CONFIG,
						     AD713X_DATA_PACKET_CONFIG_FRAME_MSK,
						     AD713X_DATA_PACKET_CONFIG_FRAME_MODE(i));
		}
		i++;
	}

	return FAILURE;
}

/**
 * @brief DOUTx output format configuration.
 * @param dev - The device structure.
 * @param format - Single channel daisy chain mode. Dual channel daisy chain mode.
 * 		   Quad channel parallel output mode. Channel data averaging mode.
 * 			Accepted values: SINGLE_CH_DC
 * 					 DUAL_CH_DC
 * 					 QUAD_CH_PO
 * 					 CH_AVG_MODE
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_dout_format_config(struct ad713x_dev *dev,
				  enum ad713x_doutx_format format)
{
	return ad713x_spi_write_mask(dev, AD713X_REG_DIGITAL_INTERFACE_CONFIG,
				     AD713X_DIG_INT_CONFIG_FORMAT_MSK,
				     AD713X_DIG_INT_CONFIG_FORMAT_MODE(format));
}

/**
 * @brief Magnitude and phase matching calibration clock delay enable for all
 *        channels at 2 clock delay.
 *        This function is kept for backwards compatibility with the current
 *        application source, but it is deprecated. Use
 *        ad713x_mag_phase_clk_delay_chan().
 * @param dev - The device structure.
 * @param clk_delay_en - Enable or disable Mag/Phase clock delay.
 * 				Accepted values: true
 * 						         false
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_mag_phase_clk_delay(struct ad713x_dev *dev,
				   bool clk_delay_en)
{
	int32_t ret;
	int8_t i;
	int8_t temp_clk_delay;

	if (clk_delay_en)
		temp_clk_delay = DELAY_2_CLOCKS;
	else
		temp_clk_delay = DELAY_NONE;

	for (i = CH3; i >= 0; i--) {
		ret = ad713x_spi_write_mask(dev, AD713X_REG_MPC_CONFIG,
					    AD713X_MPC_CLKDEL_EN_CH_MSK(i),
					    AD713X_MPC_CLKDEL_EN_CH_MODE(temp_clk_delay, i));
		if (IS_ERR_VALUE(ret))
			return FAILURE;
	}

	return SUCCESS;
}

/**
 * @brief Change magnitude and phase calibration clock delay mode for a specific
 *        channel.
 * @param dev - The device structure.
 * @param chan - ID of the channel to be changed.
 * 				Accepted values: CH0, CH1, CH2, CH3
 * @param mode - Delay in clock periods.
 * 				Accepted values: DELAY_NONE,
 * 						 DELAY_1_CLOCKS,
 *						 DELAY_2_CLOCKS
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_mag_phase_clk_delay_chan(struct ad713x_dev *dev,
					enum ad713x_channels chan,
					enum ad717x_mpc_clkdel mode)
{
	return ad713x_spi_write_mask(dev, AD713X_REG_MPC_CONFIG,
				     AD713X_MPC_CLKDEL_EN_CH_MSK(chan),
				     AD713X_MPC_CLKDEL_EN_CH_MODE(mode, chan));
}

/**
 * @brief Digital filter type selection for each channel
 * @param dev - The device structure.
 * @param filter - Type of filter: Wideband, Sinc6, Sinc3,
 * 				   Sinc3 filter with simultaneous 50Hz and 60Hz rejection.
 * 				   	Accepted values: FIR
 * 							 SINC6
 * 							 SINC3
 * 							 SINC3_50_60_REJ
 * @param ch - Channel to apply the filter to
 * 					Accepted values: CH0
 * 							 CH1
 * 							 CH2
 * 							 CH3
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_dig_filter_sel_ch(struct ad713x_dev *dev,
				 enum ad713x_dig_filter_sel filter,
				 enum ad713x_channels ch)
{
	return ad713x_spi_write_mask(dev, AD713X_REG_CHAN_DIG_FILTER_SEL,
				     AD713X_DIGFILTER_SEL_CH_MSK(ch),
				     AD713X_DIGFILTER_SEL_CH_MODE(filter, ch));
}

/**
 * @brief Enable/Disable CLKOUT output.
 * @param [in] dev - The device structure.
 * @param [in] enable - true to enable the clkout output;
 *                      false to disable the clkout output.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_clkout_output_en(struct ad713x_dev *dev, bool enable)
{
	return ad713x_spi_write_mask(dev, AD713X_REG_DEVICE_CONFIG1,
				     AD713X_DEV_CONFIG1_CLKOUT_EN_MSK,
				     enable ? AD713X_DEV_CONFIG1_CLKOUT_EN_MSK : 0);
}

/**
 * @brief Enable/Disable reference gain correction.
 * @param [in] dev - The device structure.
 * @param [in] enable - true to enable the reference gain correction;
 *                      false to disable the reference gain correction.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_ref_gain_correction_en(struct ad713x_dev *dev, bool enable)
{
	return ad713x_spi_write_mask(dev, AD713X_REG_DEVICE_CONFIG1,
				     AD713X_DEV_CONFIG1_REF_GAIN_CORR_EN_MSK,
				     enable ? AD713X_DEV_CONFIG1_REF_GAIN_CORR_EN_MSK : 0);
}

/**
 * @brief Select the wideband filter bandwidth for a channel.
 *        The option is relative to ODR, so it's a fraction of it.
 * @param [in] dev - The device structure.
 * @param [in] ch - Number of the channel to which to set the wideband filter
 *                  option.
 * @param [in] wb_opt - Option to set the wideband filter:
 *                      Values are:
 *                          0 - bandwidth of 0.443 * ODR;
 *                          1 - bandwidth of 0.10825 * ODR.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_wideband_bw_sel(struct ad713x_dev *dev,
			       enum ad713x_channels ch, uint8_t wb_opt)
{
	return ad713x_spi_write_mask(dev, AD713X_REG_FIR_BW_SEL,
				     AD713X_FIR_BW_SEL_CH_MSK(ch),
				     wb_opt ? AD713X_FIR_BW_SEL_CH_MSK(ch) : 0);
}

/**
 * @brief Initialize GPIO driver handlers for the GPIOs in the system.
 *        ad713x_init() helper function.
 * @param [out] dev - AD713X device handler.
 * @param [in] init_param - Pointer to the initialization structure.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
static int32_t ad713x_init_gpio(struct ad713x_dev *dev,
				struct ad713x_init_param *init_param)
{

	int32_t ret;

	ret = gpio_get(&dev->gpio_mode, &init_param->gpio_mode);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	ret = gpio_get(&dev->gpio_dclkmode, &init_param->gpio_dclkmode);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	ret = gpio_get(&dev->gpio_dclkio, &init_param->gpio_dclkio);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	ret = gpio_get(&dev->gpio_resetn, &init_param->gpio_resetn);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	ret = gpio_get(&dev->gpio_pnd, &init_param->gpio_pnd);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	/** Tie this pin to IOVDD for master mode operation, tie this pin to IOGND
	 *  for slave mode operation. */
	ret = gpio_direction_output(dev->gpio_mode, init_param->mode_master_nslave);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	/* Tie this pin low to ground to make DLCK operating in gated mode */
	ret = gpio_direction_output(dev->gpio_dclkmode,
				    init_param->dclkmode_free_ngated);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	/** Tie this pin high to make DCLK an output, tie this pin low to make DLCK
	 *  an input. */
	ret = gpio_direction_output(dev->gpio_dclkio, init_param->dclkio_out_nin);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	/** Get the ADCs out of power down state */
	ret = gpio_direction_output(dev->gpio_pnd, init_param->pnd);
	if (IS_ERR_VALUE(ret))
		return FAILURE;

	/** Reset to configure pins */
	ret = gpio_direction_output(dev->gpio_resetn, false);
	if (IS_ERR_VALUE(ret))
		return FAILURE;
	mdelay(100);
	ret = gpio_set_value(dev->gpio_resetn, true);
	if (IS_ERR_VALUE(ret))
		return FAILURE;
	mdelay(100);

	return SUCCESS;
}

/**
 * @brief Free the resources allocated by ad713x_init_gpio().
 * @param dev - The device structure.
 * @return SUCCESS in case of success, negative error code otherwise.
 */
static int32_t ad713x_remove_gpio(struct ad713x_dev *dev)
{
	int32_t ret;

	if (dev->gpio_dclkio) {
		ret = gpio_remove(dev->gpio_dclkio);
		if(IS_ERR_VALUE(ret))
			return FAILURE;
	}
	if (dev->gpio_dclkio) {
		ret = gpio_remove(dev->gpio_dclkmode);
		if(IS_ERR_VALUE(ret))
			return FAILURE;
	}
	if (dev->gpio_mode) {
		ret = gpio_remove(dev->gpio_mode);
		if(IS_ERR_VALUE(ret))
			return FAILURE;
	}
	if (dev->gpio_pnd) {
		ret = gpio_remove(dev->gpio_pnd);
		if(IS_ERR_VALUE(ret))
			return FAILURE;
	}
	if (dev->gpio_resetn) {
		ret = gpio_remove(dev->gpio_resetn);
		if(IS_ERR_VALUE(ret))
			return FAILURE;
	}

	return SUCCESS;
}

/**
 * @brief Initialize the wideband filter bandwidth for every channel.
 *        ad713x_init() helper function.
 * @param [in] dev - AD713X device handler.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
static int32_t ad713x_init_chan_bw(struct ad713x_dev *dev)
{
	int8_t i;
	int32_t ret;

	for (i = CH3; i >= 0; i--) {
		ret = ad713x_wideband_bw_sel(dev, i, 0);
		if (IS_ERR_VALUE(ret))
			return FAILURE;
	}

	return SUCCESS;
}

/**
 * @brief Initialize the device.
 * @param device - The device structure.
 * @param init_param - The structure that contains the device initial
 *                     parameters.
 * @return \ref SUCCESS in case of success, \ref FAILURE otherwise.
 */
int32_t ad713x_init(struct ad713x_dev **device,
		    struct ad713x_init_param *init_param)
{
	struct ad713x_dev *dev;
	int32_t ret;
	uint8_t data;

	dev = (struct ad713x_dev *)calloc(1, sizeof(*dev));
	if (!dev)
		return FAILURE;

	if (!init_param->spi_common_dev) {
		ret = spi_init(&dev->spi_desc, &init_param->spi_init_prm);
		if (IS_ERR_VALUE(ret))
			goto error_dev;
	} else {
		dev->spi_desc = calloc(1, sizeof *dev->spi_desc);
		dev->spi_desc->chip_select = init_param->spi_init_prm.chip_select;
		dev->spi_desc->extra = init_param->spi_common_dev->extra;
		dev->spi_desc->max_speed_hz = init_param->spi_init_prm.max_speed_hz;
		dev->spi_desc->mode = init_param->spi_init_prm.mode;
	}

	ret = ad713x_init_gpio(dev, init_param);
	if(IS_ERR_VALUE(ret))
		goto error_gpio;

	dev->dev_id = init_param->dev_id;

	ret = ad713x_spi_reg_read(dev, AD713X_REG_DEVICE_CONFIG, &data);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;
	data |= AD713X_DEV_CONFIG_PWR_MODE_MSK;
	ret = ad713x_spi_reg_write(dev, AD713X_REG_DEVICE_CONFIG, data);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;

	ret = ad713x_clkout_output_en(dev, true);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;

	ret = ad713x_ref_gain_correction_en(dev, true);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;

	ret = ad713x_set_out_data_frame(dev, init_param->adc_data_len,
					init_param->crc_header);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;

	ret = ad713x_dout_format_config(dev, init_param->format);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;

	ret = ad713x_mag_phase_clk_delay(dev, init_param->clk_delay_en);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;

	ret = ad713x_init_chan_bw(dev);
	if (IS_ERR_VALUE(ret))
		goto error_gpio;

	*device = dev;

	return SUCCESS;

error_gpio:
	ad713x_remove_gpio(dev);
error_dev:
	ad713x_remove(dev);

	return FAILURE;
}

/**
 * @brief Free the resources allocated by ad713x_init().
 * @param dev - The device structure.
 * @return SUCCESS in case of success, negative error code otherwise.
 */
int32_t ad713x_remove(struct ad713x_dev *dev)
{
	int32_t ret;

	if(!dev)
		return FAILURE;

	ret = spi_remove(dev->spi_desc);
	if(IS_ERR_VALUE(ret))
		return FAILURE;

	ret = ad713x_remove_gpio(dev);
	if(IS_ERR_VALUE(ret))
		return FAILURE;

	free(dev);

	return SUCCESS;
}