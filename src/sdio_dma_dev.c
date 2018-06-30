/* Copyright 2011-2018 Tyler Gilbert;
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

#include <mcu/sdio.h>
#include "sdio_local.h"

#if MCU_SDIO_PORTS > 0


static sdio_dma_local_t sdio_local[MCU_SDIO_PORTS] MCU_SYS_MEM;


DEVFS_MCU_DRIVER_IOCTL_FUNCTION(sdio_dma, SDIO_VERSION, SDIO_IOC_IDENT_CHAR, I_MCU_TOTAL + I_SDIO_TOTAL, mcu_sdio_dma_getcid, mcu_sdio_dma_getcsd, mcu_sdio_dma_getstatus)

int mcu_sdio_dma_open(const devfs_handle_t * handle){
    return sdio_local_open(&sdio_local[handle->port].sdio, handle);
}

int mcu_sdio_dma_close(const devfs_handle_t * handle){

    //do the opposite of mcu_sdio_dma_open() -- ref_count is zero -- turn off interrupt
    return 0;
}

int mcu_sdio_dma_getinfo(const devfs_handle_t * handle, void * ctl){
    return sdio_local_getinfo(&sdio_local[handle->port].sdio, handle, ctl);
}

int mcu_sdio_dma_setattr(const devfs_handle_t * handle, void * ctl){

    int result;
    const sdio_attr_t * attr = mcu_select_attr(handle, ctl);
    if( attr == 0 ){ return SYSFS_SET_RETURN(EINVAL); }

    u32 o_flags = attr->o_flags;
    sdio_dma_local_t * sdio = sdio_local + handle->port;

    result = sdio_local_setattr(&sdio->sdio, handle, ctl);
    if( (result < 0) || (o_flags & SDIO_FLAG_GET_CARD_STATE) ){
        return result;
    }

    if( o_flags & SDIO_FLAG_SET_INTERFACE ){

        const stm32_sdio_dma_config_t * config = handle->config;
        if( config == 0 ){ return SYSFS_SET_RETURN(ENOSYS); }

        stm32_dma_set_handle(&sdio->dma_rx_channel, config->dma_config.rx.dma_number, config->dma_config.rx.stream_number);
        sdio->dma_rx_channel.handle.Instance = stm32_dma_get_stream_instance(config->dma_config.rx.dma_number, config->dma_config.rx.stream_number);
        sdio->dma_rx_channel.handle.Init.Channel = stm32_dma_decode_channel(config->dma_config.rx.channel_number);
        sdio->dma_rx_channel.handle.Init.Mode = DMA_PFCTRL;
        sdio->dma_rx_channel.handle.Init.Direction = DMA_PERIPH_TO_MEMORY; //read is always periph to memory
        sdio->dma_rx_channel.handle.Init.PeriphInc = DMA_PINC_DISABLE; //don't inc peripheral
        sdio->dma_rx_channel.handle.Init.MemInc = DMA_MINC_ENABLE; //do inc the memory

        sdio->dma_rx_channel.handle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        sdio->dma_rx_channel.handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        sdio->dma_rx_channel.handle.Init.MemBurst = DMA_MBURST_INC4;
        sdio->dma_rx_channel.handle.Init.PeriphBurst = DMA_PBURST_INC4;

        sdio->dma_rx_channel.handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        sdio->dma_rx_channel.handle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;

        sdio->dma_rx_channel.handle.Init.Priority = stm32_dma_decode_priority(config->dma_config.rx.priority);

        if (HAL_DMA_Init(&sdio->dma_rx_channel.handle) != HAL_OK){ return SYSFS_SET_RETURN(EIO); }

        __HAL_LINKDMA((&sdio->sdio.hal_handle), hdmarx, sdio->dma_rx_channel.handle);

        //setup the DMA for transmitting
        stm32_dma_set_handle(&sdio->dma_tx_channel, config->dma_config.tx.dma_number, config->dma_config.tx.stream_number);
        sdio->dma_tx_channel.handle.Instance = stm32_dma_get_stream_instance(config->dma_config.tx.dma_number, config->dma_config.tx.stream_number);
        sdio->dma_tx_channel.handle.Init.Channel = stm32_dma_decode_channel(config->dma_config.tx.channel_number);
        sdio->dma_tx_channel.handle.Init.Mode = DMA_PFCTRL;
        sdio->dma_tx_channel.handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        sdio->dma_tx_channel.handle.Init.PeriphInc = DMA_PINC_DISABLE; //don't inc peripheral
        sdio->dma_tx_channel.handle.Init.MemInc = DMA_MINC_ENABLE; //do inc the memory

        sdio->dma_tx_channel.handle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        sdio->dma_tx_channel.handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        sdio->dma_tx_channel.handle.Init.MemBurst = DMA_MBURST_INC4;
        sdio->dma_tx_channel.handle.Init.PeriphBurst = DMA_PBURST_INC4;

        sdio->dma_tx_channel.handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        sdio->dma_tx_channel.handle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;

        sdio->dma_tx_channel.handle.Init.Priority = stm32_dma_decode_priority(config->dma_config.tx.priority);

        if (HAL_DMA_Init(&sdio->dma_tx_channel.handle) != HAL_OK){
          return SYSFS_SET_RETURN(EIO);
        }

        __HAL_LINKDMA((&sdio->sdio.hal_handle), hdmatx, sdio->dma_tx_channel.handle);


        if( HAL_SD_Init(&sdio->sdio.hal_handle) != HAL_OK ){
            return SYSFS_SET_RETURN(EIO);
        }

        //SDIO_BUS_WIDE_1B -- set as default for initialziation
        //SDIO_BUS_WIDE_4B
        //SDIO_BUS_WIDE_8B -- not compatible with SDIO
        if( o_flags & SDIO_FLAG_IS_BUS_WIDTH_4 ){
            HAL_SD_ConfigWideBusOperation(&sdio->sdio.hal_handle, SDIO_BUS_WIDE_4B);
        }
    }


    return SYSFS_RETURN_SUCCESS;
}


int mcu_sdio_dma_setaction(const devfs_handle_t * handle, void * ctl){
    mcu_action_t * action = ctl;
    u32 port = handle->port;

    if( action->handler.callback == 0 ){
        if( action->o_events & MCU_EVENT_FLAG_DATA_READY ){
            //HAL_SD_Abort_IT(&sdio_local[port].hal_handle);
            mcu_execute_read_handler_with_flags(&sdio_local[port].sdio.transfer_handler, 0, SYSFS_SET_RETURN(EIO), MCU_EVENT_FLAG_CANCELED);
        }

        if( action->o_events & MCU_EVENT_FLAG_WRITE_COMPLETE ){
            //HAL_SD_Abort_IT(&sdio_local[port].hal_handle);
            mcu_execute_write_handler_with_flags(&sdio_local[port].sdio.transfer_handler, 0, SYSFS_SET_RETURN(EIO), MCU_EVENT_FLAG_CANCELED);
        }
    }

    cortexm_set_irq_priority(sdio_irqs[port], action->prio, action->o_events);
    return 0;
}

int mcu_sdio_dma_getcid(const devfs_handle_t * handle, void * ctl){
    return sdio_local_getcid(&sdio_local[handle->port].sdio, handle, ctl);
}

int mcu_sdio_dma_getcsd(const devfs_handle_t * handle, void * ctl){
    return sdio_local_getcsd(&sdio_local[handle->port].sdio, handle, ctl);
}

int mcu_sdio_dma_getstatus(const devfs_handle_t * handle, void * ctl){
    return sdio_local_getstatus(&sdio_local[handle->port].sdio, handle, ctl);
}

int mcu_sdio_dma_write(const devfs_handle_t * handle, devfs_async_t * async){
    int port = handle->port;
    DEVFS_DRIVER_IS_BUSY(sdio_local[port].sdio.transfer_handler.write, async);

    sdio_local[port].sdio.start_time = TIM2->CNT;

    mcu_debug_root_printf("WState:%d %d %d %d\n", HAL_SD_GetCardState(&sdio_local[port].sdio.hal_handle),
                          async->loc,
                          async->nbyte,
                          sdio_local[port].sdio.hal_handle.Instance->DTIMER
                          );

    sdio_local[port].sdio.hal_handle.TxXferSize = async->nbyte; //used by the callback but not set by HAL_SD_WriteBlocks_DMA
    if( (HAL_SD_WriteBlocks_DMA(&sdio_local[port].sdio.hal_handle, async->buf, async->loc, async->nbyte / BLOCKSIZE)) == HAL_OK ){
        return 0;
    }

    sdio_local[port].sdio.transfer_handler.write = 0;
    return SYSFS_SET_RETURN(EIO);
}

int mcu_sdio_dma_read(const devfs_handle_t * handle, devfs_async_t * async){
    int port = handle->port;
    int hal_result;
    DEVFS_DRIVER_IS_BUSY(sdio_local[port].sdio.transfer_handler.read, async);

    sdio_local[port].sdio.start_time = TIM2->CNT;

    mcu_debug_root_printf("RState:%d\n", HAL_SD_GetCardState(&sdio_local[port].sdio.hal_handle));

    sdio_local[port].sdio.hal_handle.RxXferSize = async->nbyte; //used by the callback but not set by HAL_SD_ReadBlocks_DMA
    if( (hal_result = HAL_SD_ReadBlocks_DMA(&sdio_local[port].sdio.hal_handle, async->buf, async->loc, async->nbyte / BLOCKSIZE)) == HAL_OK ){
        return 0;
    }

    //mcu_debug_root_printf("R:HAL Not OK %d %d %d 0x%lX\n", async->loc, async->nbyte, hal_result, sdio_local[port].hal_handle.ErrorCode);
    //mcu_debug_root_printf("State: %d\n", HAL_SD_GetCardState(&sdio_local[port].hal_handle));
    sdio_local[port].sdio.transfer_handler.read = 0;
    return SYSFS_SET_RETURN(EIO);
}


#endif
