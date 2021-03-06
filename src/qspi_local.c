/* Copyright 2011-2016 Tyler Gilbert;
 * This file is part of Stratify OS.
 *
 * Stratify OS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Stratify OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Stratify OS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#include <fcntl.h>
#include "stm32_local.h"
#include "cortexm/cortexm.h"
#include "mcu/qspi.h"
#include "mcu/pio.h"
#include "mcu/debug.h"
#include "mcu/core.h"
#include <sched.h>

#if MCU_QSPI_PORTS > 0

#include "qspi_local.h"

qspi_local_t m_qspi_local[MCU_QSPI_PORTS] MCU_SYS_MEM;
QUADSPI_TypeDef * const qspi_regs_table[MCU_QSPI_PORTS] = MCU_QSPI_REGS;
u8 const qspi_irqs[MCU_QSPI_PORTS] = MCU_QSPI_IRQS;

int qspi_local_open(const devfs_handle_t * handle){
	DEVFS_DRIVER_DECLARE_LOCAL(qspi, MCU_QSPI_PORTS);

	if ( local->ref_count == 0 ){
		memset(&local->hal_handle,0,sizeof(QSPI_HandleTypeDef));
		local->hal_handle.Instance = qspi_regs_table[port];

		switch(port){
			case 0:
				__HAL_RCC_QSPI_CLK_ENABLE();
				break;
		}
		//reset HAL UART
		cortexm_enable_irq(qspi_irqs[port]);
	}
	local->ref_count++;

	return 0;
}

int qspi_local_close(const devfs_handle_t * handle){
	DEVFS_DRIVER_DECLARE_LOCAL(qspi, MCU_QSPI_PORTS);
	if ( local->ref_count > 0 ){
		if ( local->ref_count == 1 ){
			cortexm_disable_irq(qspi_irqs[port]);
			switch(port){
				case 0:
					__HAL_RCC_QSPI_CLK_DISABLE();
					break;
			}
			local->hal_handle.Instance = 0;
		}
		local->ref_count--;
	}
	return 0;
}


int qspi_local_setattr(const devfs_handle_t * handle, void * ctl){
	u32 o_flags;
	qspi_attr_t * attr;
	DEVFS_DRIVER_DECLARE_LOCAL(qspi, MCU_QSPI_PORTS);
	attr = (qspi_attr_t *)mcu_select_attr(handle, ctl);
	if( attr == 0 ){
		return SYSFS_SET_RETURN(EINVAL);
	}
	o_flags = attr->o_flags;
	local->state = o_flags;
	if( o_flags & QSPI_FLAG_SET_MASTER ){
		uint32_t flash_size = 24;

		__HAL_RCC_QSPI_FORCE_RESET();
		__HAL_RCC_QSPI_RELEASE_RESET();

		if( mcu_set_pin_assignment(
				 &(attr->pin_assignment),
				 MCU_CONFIG_PIN_ASSIGNMENT(qspi_config_t, handle),
				 MCU_PIN_ASSIGNMENT_COUNT(qspi_pin_assignment_t),
				 CORE_PERIPH_QSPI, port, 0, 0, 0) < 0 ){
			return SYSFS_SET_RETURN(EINVAL);
		}

		//prescalar can be between 0 and 255
		u32 prescalar;
		if( attr->freq ){
			prescalar = mcu_board_config.core_cpu_freq / attr->freq;
			if( prescalar > 255 ){
				prescalar = 255;
			}
		} else {
			prescalar = 0;
		}

		local->hal_handle.Init.ClockPrescaler = prescalar; //need to calculate
		local->hal_handle.Init.FifoThreshold = 1; //interrupt fires when FIFO is half full
		local->hal_handle.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
		local->hal_handle.Init.FlashSize = 31; /*attribute size 2^size-1*/
		local->hal_handle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
		if(o_flags & QSPI_FLAG_IS_CLK_HIGH_WHILE_CS ){
			local->hal_handle.Init.ClockMode = QSPI_CLOCK_MODE_3;
		}else{
			/*default*/
			local->hal_handle.Init.ClockMode = QSPI_CLOCK_MODE_0;
		}
		//Clock mode QSPI_CLOCK_MODE_3 is double data rate
		if(o_flags & QSPI_FLAG_IS_FLASH_ID_2){
			local->hal_handle.Init.FlashID = QSPI_FLASH_ID_2;
			local->hal_handle.Init.DualFlash = QSPI_DUALFLASH_ENABLE;
		} else {
			/*by default*/
			local->hal_handle.Init.FlashID = QSPI_FLASH_ID_1;
			local->hal_handle.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
		}
		if( HAL_QSPI_Init(&local->hal_handle) != HAL_OK ){
			return SYSFS_SET_RETURN(EIO);
		}

	}


	return 0;
}

int qspi_local_setaction(const devfs_handle_t * handle, void * ctl){
	int port = handle->port;
	mcu_action_t * action = ctl;
	if( action->handler.callback != 0 ){
		return SYSFS_SET_RETURN(ENOTSUP);
	}
	cortexm_set_irq_priority(qspi_irqs[port], action->prio, action->o_events);
	return 0;
}

int qspi_local_execcommand(const devfs_handle_t * handle, void * ctl){
	DEVFS_DRIVER_DECLARE_LOCAL(qspi, MCU_QSPI_PORTS);

	qspi_command_t * qspi_command = ctl;
	u32 o_flags = qspi_command->o_flags;

	QSPI_CommandTypeDef command;
	command.Instruction = qspi_command->opcode;
	command.NbData = qspi_command->data_size;

	command.Instruction = qspi_command->opcode;
	command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	if( o_flags & QSPI_FLAG_IS_COMMAND_DUAL ){
		command.InstructionMode = QSPI_INSTRUCTION_2_LINES;
	} else if( o_flags & QSPI_FLAG_IS_COMMAND_QUAD ){
		command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
	}

	command.Address = qspi_command->address;
	command.AddressMode = QSPI_ADDRESS_NONE;
#if 0
	if( o_flags & QSPI_FLAG_IS_ADDRESS_8_BITS ){
		command.AddressMode = QSPI_ADDRESS_8_BITS;
	} else if( o_flags & QSPI_FLAG_IS_ADDRESS_16_BITS ){
		command.AddressMode = QSPI_ADDRESS_16_BITS;
	} else if( o_flags & QSPI_FLAG_IS_ADDRESS_24_BITS ){
		command.AddressMode = QSPI_ADDRESS_24_BITS;
	} else if( o_flags & QSPI_FLAG_IS_ADDRESS_32_BITS ){
		command.AddressMode = QSPI_ADDRESS_32_BITS;
	}
#endif

	command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	command.AlternateBytes = qspi_command->address; //up to 32 bits can be sent
	command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_24_BITS; //8,16,24 or 32 bits

	//setup the data mode
	command.DataMode = QSPI_DATA_NONE;
	if( qspi_command->data_size > 0 ){
		command.DataMode = QSPI_DATA_1_LINE;
		if( o_flags & QSPI_FLAG_IS_DATA_DUAL ){
			command.DataMode = QSPI_DATA_2_LINES;
		} else if( o_flags & QSPI_FLAG_IS_DATA_QUAD ){
			command.DataMode = QSPI_DATA_4_LINES;
		}
	}

	command.DummyCycles = qspi_command->dummy_size;

	//no double data rate
	command.DdrMode = QSPI_DDR_MODE_DISABLE;
	command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	//mcu_debug_printf("%s():%d 0x%X 0x%X\n", __FUNCTION__, __LINE__, command.Instruction, local->hal_handle.Instance->SR);

	if( HAL_QSPI_Command(&local->hal_handle, &command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK ){
		local->transfer_handler.write = 0;
		return SYSFS_SET_RETURN(EIO);
	}

	if( command.NbData > 0 ){
		if( o_flags & QSPI_FLAG_IS_COMMAND_READ ){
			if (HAL_QSPI_Receive(&local->hal_handle, qspi_command->data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){
				local->transfer_handler.read = 0;
				return SYSFS_SET_RETURN(EIO);
			}
		} else if( o_flags & QSPI_FLAG_IS_COMMAND_WRITE ){
			if(HAL_QSPI_Transmit(&local->hal_handle, qspi_command->data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)!=HAL_OK){
				return SYSFS_SET_RETURN(EIO);
			}
		}
	}

	return 0;
}

void HAL_QSPI_ErrorCallback(QSPI_HandleTypeDef *hqspi){
	mcu_debug_printf("error 0x%X\n", hqspi->ErrorCode);
}

void HAL_QSPI_AbortCpltCallback(QSPI_HandleTypeDef *hqspi){
	mcu_debug_printf("abort\n");

}

void HAL_QSPI_FifoThresholdCallback(QSPI_HandleTypeDef *hqspi){

}

void HAL_QSPI_CmdCpltCallback(QSPI_HandleTypeDef *hqspi){
	int ret;
	qspi_local_t * local =  (qspi_local_t *)hqspi;
	if( local->transfer_handler.read ){
		//command is the start of a read operation -- complete the read
		//ret = HAL_QSPI_Receive_IT(hqspi, local->transfer_handler.read->buf);
	} else if( local->transfer_handler.write ){
		//ret = HAL_QSPI_Transmit_IT(hqspi, local->transfer_handler.write->buf);
	}

	if( ret != HAL_OK ){
		//there was an error -- execute the callback
	}

}

void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi){
	qspi_local_t * local =  (qspi_local_t *)hqspi;
	//mcu_debug_printf("RX complete %d\n", hqspi->RxXferCount);
	if( local->hal_handle.hdma != 0 && local->transfer_handler.read ){
		//pull in values from memory to cache if using DMA
		mcu_core_invalidate_data_cache_block(
					local->transfer_handler.read->buf,
					local->transfer_handler.read->nbyte);
	}
	devfs_execute_read_handler(&local->transfer_handler, 0, hqspi->RxXferCount, MCU_EVENT_FLAG_DATA_READY);

}

void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi){
	qspi_local_t * qspi =  (qspi_local_t *)hqspi;
	//mcu_debug_printf("TX complete\n");
	devfs_execute_write_handler(&qspi->transfer_handler, 0, hqspi->TxXferCount, MCU_EVENT_FLAG_WRITE_COMPLETE);
}

void HAL_QSPI_RxHalfCpltCallback(QSPI_HandleTypeDef *hqspi){
	//this is for DMA only
}

void HAL_QSPI_TxHalfCpltCallback(QSPI_HandleTypeDef *hqspi){
	//this is for DMA only
}

void mcu_core_quadspi_isr(){
	//mcu_debug_printf("QSPI handler %d\n", m_qspi_local[0].hal_handle.RxXferCount);
	HAL_QSPI_IRQHandler(&m_qspi_local[0].hal_handle);
}


#endif
