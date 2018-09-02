#include "Exhi.h"

int main(void)
{
	int ** arr;
	arr = new int*[2];
	for (int i = 0; i < 2; ++i)
	{
		arr[i] = new int[3];
	}
	arr[0][0] = 1;
	arr[0][1] = 2;
	arr[0][2] = 3;
	arr[1][0] = 4;
	arr[1][1] = 5;
	arr[1][2] = 6;

	int ** transarr;

	transarr = MatrixTranspose(arr,3,2);
	ShowArray(arr, 3, 2);
	ShowArray(transarr, 2, 3);

	return 0;
}