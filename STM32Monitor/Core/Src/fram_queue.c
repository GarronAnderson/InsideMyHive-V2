/*
 * fram_queue.c
 *
 *  Created on: Sep 25, 2026
 *      Author: mfdal
 */

#include "fram.h"
#include "fram_queue.h"
#include "hive_packet.h"

#define TAIL_ADDR       0
#define HEAD_ADDR       2
#define COUNT_ADDR      4
#define PACKETS_START   6

#define PACKET_SIZE     sizeof(HivePacket)
#define MAX_PACKETS     1440


void FRAM_QueueInit(void)
{
    uint16_t zero = 0;

    FRAM_Write(TAIL_ADDR,  (uint8_t *)&zero, sizeof(zero));
    FRAM_Write(HEAD_ADDR,  (uint8_t *)&zero, sizeof(zero));
    FRAM_Write(COUNT_ADDR, (uint8_t *)&zero, sizeof(zero));
}


HAL_StatusTypeDef FRAM_Push(const HivePacket *packet)
{
    uint16_t head;
    uint16_t count;

    if (packet == NULL)
        return HAL_ERROR;

    /* Read queue state */
    if (FRAM_Read(HEAD_ADDR,
                  (uint8_t *)&head,
                  sizeof(head)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (FRAM_Read(COUNT_ADDR,
                  (uint8_t *)&count,
                  sizeof(count)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Queue is full */
    if (count >= MAX_PACKETS)
        return HAL_BUSY;

    /* Calculate address of next free packet slot */
    uint32_t address =
        (PACKET_SIZE * head) + PACKETS_START;

    /* Write packet */
    if (FRAM_Write(address,
                   (const uint8_t *)packet,
                   PACKET_SIZE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Advance head */
    head++;

    if (head >= MAX_PACKETS)
        head = 0;

    /* Increment count */
    count++;

    /* Save queue state */
    if (FRAM_Write(HEAD_ADDR,
                   (uint8_t *)&head,
                   sizeof(head)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (FRAM_Write(COUNT_ADDR,
                   (uint8_t *)&count,
                   sizeof(count)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


HAL_StatusTypeDef FRAM_Peek(HivePacket *packet)
{
    uint16_t tail;
    uint16_t count;

    if (packet == NULL)
        return HAL_ERROR;

    /* Read queue state */
    if (FRAM_Read(TAIL_ADDR,
                  (uint8_t *)&tail,
                  sizeof(tail)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (FRAM_Read(COUNT_ADDR,
                  (uint8_t *)&count,
                  sizeof(count)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Queue is empty */
    if (count == 0)
        return HAL_OK;

    /* Calculate address of oldest packet */
    uint32_t address =
        (PACKET_SIZE * tail) + PACKETS_START;

    return FRAM_Read(address,
                    (uint8_t *)packet,
                    PACKET_SIZE);
}


HAL_StatusTypeDef FRAM_Pop(void)
{
    uint16_t tail;
    uint16_t count;

    /* Read queue state */
    if (FRAM_Read(TAIL_ADDR,
                  (uint8_t *)&tail,
                  sizeof(tail)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (FRAM_Read(COUNT_ADDR,
                  (uint8_t *)&count,
                  sizeof(count)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Advance tail */
    tail++;

    if (tail >= MAX_PACKETS)
        tail = 0;

    /* Decrement count */
    count--;

    /* Save queue state */
    if (FRAM_Write(TAIL_ADDR,
                   (uint8_t *)&tail,
                   sizeof(tail)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (FRAM_Write(COUNT_ADDR,
                   (uint8_t *)&count,
                   sizeof(count)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint16_t FRAM_GetCount(void)
{
    uint16_t count;

    FRAM_Read(COUNT_ADDR,
              (uint8_t *)&count,
              sizeof(count));

    return count;
}
