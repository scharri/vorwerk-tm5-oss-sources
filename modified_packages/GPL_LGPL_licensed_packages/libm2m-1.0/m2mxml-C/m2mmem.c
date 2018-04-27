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

#include <stdlib.h>
#include <string.h>
#include "m2mmem.h"

/* EXTERNAL API */

void M2MMEM_initializeBuffer(M2MMEM_BUFFER *pBuffer,char *pStart,int maxSize) {
	pBuffer->pStart = pStart;
	pBuffer->pNext = pStart;
	pBuffer->usedCount = 0;
	pBuffer->maxSize = maxSize;
}

void *M2MMEM_allocateSpace(M2MMEM_BUFFER *pBuffer,int size) {
	void *pResult;
	
    //force 4-byte alignment of allocated memory
    //while(size % 4)
    //     size++;
	
	if (pBuffer->usedCount + size > pBuffer->maxSize)
		return NULL;
	pResult = pBuffer->pNext;
	pBuffer->pNext += size;
	pBuffer->usedCount += size;
	memset(pResult,0,size);
	return pResult;
}

const char *M2MMEM_allocateString(M2MMEM_BUFFER *pBuffer,const char *pSource) {
	int length;
	char *pResult;
	
	if (pSource == NULL)
		return NULL;
	length = strlen(pSource);
	pResult = (char*)M2MMEM_allocateSpace(pBuffer,length + 1);
	return pResult ? strcpy(pResult,pSource) : NULL;
}

const char *M2MMEM_appendString(M2MMEM_BUFFER *pBuffer,const char *pSource) {
	int length;
	char *pResult;
	
	if (pSource == NULL)
		return NULL;
	length = strlen(pSource);
	if (pBuffer->usedCount + length + 1 > pBuffer->maxSize)
		return NULL;
	pResult = (char*)M2MMEM_allocateSpace(pBuffer,length);
	return pResult ? strcpy(pResult,pSource) : NULL;
}

const char **M2MMEM_allocateProperties(M2MMEM_BUFFER *pBuffer,const char **ppSource) {
	int pointerCount;
	const char **ppResult;
	const char **ppDestination;
	
	if (ppSource == NULL)
		return NULL;
	pointerCount = 0;
	while (ppSource[pointerCount++])
		;
	if ((ppResult = (const char**)M2MMEM_allocateSpace(pBuffer,pointerCount * sizeof(const char *))) == NULL)
		return NULL;
	ppDestination = ppResult;
	for (; *ppSource != NULL; ppDestination++, ppSource++)
		if ((*ppDestination = M2MMEM_allocateString(pBuffer,*ppSource)) == NULL && *ppSource != NULL)
			return NULL;
	return ppResult;
}

int M2MMEM_allocateProperty(M2MMEM_BUFFER *pBuffer,const char **ppProperties,const char *pKey,const char *pValue,int index) {
	if (pKey == NULL || pValue == NULL || index < 0)
		return index;
	if (ppProperties == NULL ||
		(ppProperties[index++] = M2MMEM_allocateString(pBuffer,pKey)) == NULL ||
		(ppProperties[index++] = M2MMEM_allocateString(pBuffer,pValue)) == NULL)
		return -1;
	return index;
}

const char **M2MMEM_allocatePropertySet(M2MMEM_BUFFER *pBuffer,int propertyCount) {
	return (const char **)M2MMEM_allocateSpace(pBuffer,sizeof(const char *) * (propertyCount * 2 + 1));
}
