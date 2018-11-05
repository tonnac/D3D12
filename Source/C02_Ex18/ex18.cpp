#include <iostream>
using namespace std;

int ** MatrixTranspose(int**ptr, int row, int col);
void MatrixShow(int ** ptr, int row, int col);

int main()
{
	int ** ptr;
	ptr = new int*[3];
	for (int i = 0; i < 3; ++i)
	{
		ptr[i] = new int[2];
	}

	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			ptr[i][j] = (j + 1) + (i * 2);
		}
	}

	MatrixShow(ptr, 3, 2);

	int ** transptr = MatrixTranspose(ptr, 3, 2);

	MatrixShow(transptr, 2, 3);

	for (int i = 0; i < 3; ++i)
	{
		delete[] ptr[i];
	}
	delete[] ptr;

	for (int i = 0; i < 2; ++i)
	{
		delete[] transptr[i];
	}
	delete[] transptr;
	return 0;
}


int ** MatrixTranspose(int**ptr, int row, int col)
{
	int ** retptr = new int*[col];
	for (int i = 0; i < col; ++i)
	{
		retptr[i] = new int[row];
	}

	for (int i = 0; i < row; ++i)
	{
		for (int j = 0; j < col; ++j)
		{
			retptr[j][i] = ptr[i][j];
		}
	}
	return retptr;
}

void MatrixShow(int ** ptr, int row, int col)
{
	for (int i = 0; i < row; ++i)
	{
		for (int j = 0; j < col; ++j)
		{
			if (j != 0)
			{
				cout << ' ';
			}
			cout << ptr[i][j];
		}
		cout << endl;
	}
}