#pragma once

/* This file contains useful Functions */

//New Functions
int chrlen(const char* array)
{
	int i = 0;
	while (array[i] != '\n')
	{
		i++;
	}
	return i;
}