/**
 *  \brief Definition of the data cluster data type.
 *
 *  \author António Rui Borges - 2012-2015
 *  \author Artur Pereira - 2016
 */

#ifndef __SOFS16_CLUSTER__
#define __SOFS16_CLUSTER__

#include <stdint.h>

#include "rawdisk.h"

/** \brief number references per data cluster */
#define RPB (BLOCK_SIZE / sizeof (uint32_t))

#endif				/* __SOFS16_CLUSTER__ */
