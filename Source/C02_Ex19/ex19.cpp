#include "Ex19.h"

template <typename X>
void Matrix::CofactorMatrix(const fdMatrix<X>& matrix, fdMatrix<X> desc)
{
	int a = 0, b = 0;
	X mat[4][4] = { 0, };
	int q = 0;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			for (int k = 0; k < 4; ++k)
			{
				for (int u = 0; u < 4; ++u)
				{
					if (u == j || k == i) continue;
					mat[a][b] = matrix[k][u];
					b = (b + 1) % 3;
				}
				if (k == i) continue;
				a = (a + 1) % 3;
			}
			desc[i][j] = (int)pow(-1, q++) * ComputeDeterminant<X>(mat, 3);
		}
	}
}
template <typename X>
void Matrix::InverseMatrix(const fdMatrix<X>& matrix, fdMatrix<float> desc)
{
	X deter = ComputeDeterminant(matrix);
	if (abs(deter - 0.0f) < 0.01f)
	{
		return;
	}
	X Cofac[4][4];
	X temp[4][4];
	CofactorMatrix(matrix, Cofac);
	memcpy(temp, Cofac, sizeof(X) * 16);
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			Cofac[j][i] = temp[i][j];
		}
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			desc[i][j] = Cofac[i][j] / (float)deter;
		}
	}
}
template <typename X>
X Matrix::ComputeDeterminant(const fdMatrix<X>& matrix, int order)
{
	X deter = 0;
	int a = 0, b = 0;
	switch (order)
	{
		case 4:
		case 3:
		{
			X mat[4][4] = { 0, };
			for (int i = 0; i < order; ++i)
			{
				for (int k = 1; k < order; ++k)
				{
					for (int j = 0; j < order; ++j)
					{
						if (j == i) continue;
						mat[a][b] = matrix[k][j];
						b = (b + 1) % (order - 1);
					}
					a = (a + 1) % (order - 1);
				}
				deter += matrix[0][i] * (int)pow(-1, i) * ComputeDeterminant<X>(mat, order - 1);
			}
		}break;
		case 2:
		{
			for (int i = 0; i < order; ++i)
			{
				deter += (int)pow(-1, i) * matrix[0][i] * matrix[1][1 - i];
			}
		}break;
	}
	return deter;
}