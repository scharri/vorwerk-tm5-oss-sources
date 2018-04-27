
#include "string.h"     //for memset
#include "stdio.h"      //for NULL (and printf for debugging)

char *m2mitoa(int input, char *output, int size)
{
	const char digits[] = "0123456789";
	int i, j, abs_input;
	char tmp;

	//handle negative value
	if (input < 0)
	{
		abs_input = -input;
		size--;//we'll need space for the sign
	}
	else
	{
		abs_input = input;
	}

	//create string in reverse order
	i = 0;
	do
	{
		//check for possible overrun 
		if(i >= (size - 1))//one byte for terminator
        {
            memset(output, 0, size);
            return NULL;
        }
		
		output[i] = digits[abs_input % 10];
		i++;
	}
	while ((abs_input /= 10) > 0);

	//add sign
	if (input < 0)
	{
		output[i] = '-';
		i++;
	}

	//terminage string
	output[i] = '\0';

	//flip string
	j = 0;
	i --;//do NOT flip the terminating zero!
	while(j < i)
	{
		tmp = output[j];
		output[j] = output[i];
		output[i] = tmp;
		j++;
		i--;
	}

	return output;
}


