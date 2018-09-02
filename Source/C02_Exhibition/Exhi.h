#pragma once
#include <iostream>

int** MatrixTranspose(int ** arr, int row, int column)
{
	int ** ptr = new int*[row];
	for (size_t i = 0; i < row; ++i)
	{
		ptr[i] = new int[column];
		memset(ptr[i], 0, sizeof(int) * column);
	}
	for (size_t iColumn = 0; iColumn < column; ++iColumn)
	{
		for (size_t iRow = 0; iRow < row; ++iRow)
		{
			if (iRow == iColumn)
			{
				ptr[iRow][iColumn] = arr[iColumn][iRow];
			}
			ptr[iRow][iColumn] = arr[iColumn][iRow];
		}
	}
	return ptr;
}
void ShowArray(int ** arr, int row, int column)
{
	for (size_t i = 0; i < column; ++i)
	{
		for (size_t j = 0; j < row; ++j)
		{
			if (j != 0)
			{
				std::cout << " ";
			}
			std::cout << arr[i][j];
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}