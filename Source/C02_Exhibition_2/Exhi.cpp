#include "Exhi.h"

template <class K = int>
K Matrix<K>::Determinant(fDMatrix<K> m)
{
	K ** ptr = new K*[4];
	for (int i = 0; i < 4; ++i)
	{
		ptr[i] = new K[4];
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ptr[i][j] = m[i][j];
		}
	}
	K pe = computeDeterminant(ptr, 4);
	for (int i = 0; i < 4; ++i)
	{
		delete[] ptr[i];
	}
	delete[] ptr;
	return pe;
}

template <class K = int>
K** Matrix<K>::Inverse(fDMatrix<K> m)
{
	K Deter = Determinant(m);
	K temp[4][4];
	int x1 = 0, y1 = 0;

	if (Deter == 0)
	{
		return nullptr;
	}

	int x = 0, y = -1;
	K ** arr = nullptr;
	int k = 0;
	int z = 0;

	arr = new K*[3];
	for (size_t p = 0; p < 3; ++p)
	{
		arr[p] = new K[3];
	}
	for (size_t a = 0; a < 4; ++a)
	{
		for (size_t i = 0; i < 4; ++i)
		{
			y = 0;
			for (size_t j = 0; j < 4; ++j)
			{
				x = 0;
				for (size_t d = 0; d < 4; ++d)
				{
					if (d == i || j == a)
					{
						continue;
					}
					arr[y][x++] = m[j][d];
				}
				if (j != a)	++y;
			}
			(i + a) % 2 == 0 ? k = 0 : k = 1;
			temp[y1][x1] = static_cast<float>(pow(-1, k)) * computeDeterminant(arr, 3);
			if (x1 + 1 >= 4)
			{
				if (y1 + 1 >= 4)
				{
					y1 = 0;
				}
				else
					++y1;
				x1 = 0;
			}
			else
				++x1;
		}
	}
	for (size_t p = 0; p < 3; ++p)
	{
		delete[] arr[p];
	}
	delete[] arr;

	K ** ptr;
	ptr = new K*[4];
	for (int i = 0; i < 4; ++i)
	{
		ptr[i] = new K[4];
	}
	
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (i == j)
			{
				ptr[i][j] = temp[i][j];
				ptr[i][j] /= Deter;
				continue;
			}
			ptr[i][j] = temp[j][i];
			ptr[i][j] /= Deter;
		}
	}
	return ptr;
}

template <class K = int>
K Matrix<K>::computeDeterminant(K ** ptr, const int& value)
{
	int x = 0, y = 0;
	K ** arr = nullptr;
	int k = 0;
	K TotalDeterminant = 0;
	K Determinant = 0;

	if (value - 1 == 0)
	{
		return **ptr;
	}

	arr = new K*[value - 1];
	for (size_t p = 0; p < value - 1; ++p)
	{
		arr[p] = new K[value - 1];
	}
	for (size_t i = 0; i < value; ++i)
	{
		y = 0;
		for (size_t j = 0; j < value; ++j)
		{
			x = 0;
			for (size_t d = 0; d < value; ++d)
			{
				if (d == i || j == 0)
				{
					continue;
				}
				arr[y][x++] = ptr[j][d];
			}
			if(j != 0)	
				++y;
		}
		Determinant = ptr[0][i] * static_cast<float>(pow(-1, k)) * computeDeterminant(arr, value - 1);
		TotalDeterminant += Determinant;
		++k;
	}

	for (size_t p = 0; p < value - 1; ++p)
	{
		delete[] arr[p];
	}
	delete[] arr;
	return TotalDeterminant;
}