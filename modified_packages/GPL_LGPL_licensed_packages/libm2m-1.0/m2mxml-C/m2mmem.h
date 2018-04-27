/* ============================================================================
 *                   GNU Lesser General Public License
 * ============================================================================
 *
 * Copyright (C) 2005 SensorLogic, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _M2MMEM_H_
#define _M2MMEM_H_

/* BUFFER DEFINITION */

typedef struct {
	char *pStart;
	char *pNext;
	int usedCount;
	int maxSize;
} M2MMEM_BUFFER;

/* EXTERNAL API */

void M2MMEM_initializeBuffer(M2MMEM_BUFFER *pBuffer,char *pStart,int maxSize);
void *M2MMEM_allocateSpace(M2MMEM_BUFFER *pBuffer,int size);
const char **M2MMEM_allocateProperties(M2MMEM_BUFFER *pBuffer,const char **ppSource);
int M2MMEM_allocateProperty(M2MMEM_BUFFER *pBuffer,const char **ppProperties,const char *pKey,const char *pValue,int index);
const char **M2MMEM_allocatePropertySet(M2MMEM_BUFFER *pBuffer,int propertyCount);
const char *M2MMEM_allocateString(M2MMEM_BUFFER *pBuffer,const char *pSource);
const char *M2MMEM_appendString(M2MMEM_BUFFER *pBuffer,const char *pSource);

#endif
